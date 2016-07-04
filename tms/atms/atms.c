#include <assert.h>

#include "tv.h"
#include "defs.h"
#include "trie.h"
#include "atms.h"
#include "qsort.h"

/* Static function prototypes: */
static void propagate(atms, atms_justification, atms_node, trie);
static void update(atms, trie, atms_node);
static trie weave(atms, atms_node, trie, array);
static void nogood(atms, trie);

static void atms_node_free(atms_node);
static void atms_justification_free(atms_justification);
static array get_environment_key(array);

/* Data structures. */
atms atms_new()
{
    atms tms = (atms)malloc(sizeof(struct str_atms));
    if (NULL == tms) {
        return tms;
    }

    tms->nodes = array_new((array_element_destroy_func_t)atms_node_free, NULL);
    tms->justifications = array_new((array_element_destroy_func_t)atms_justification_free, NULL);

/* The "F" node is special. An environment of the "F" node is a "nogood". */
    atms_add_node(tms, 0, 1);

    return tms;
}

void atms_free(atms tms)
{
    array_free(tms->justifications);
    array_free(tms->nodes);

    free(tms);
}

static atms_justification atms_justification_new()
{
    atms_justification justification = (atms_justification)malloc(sizeof(struct str_atms_justification));
    if (NULL == justification) {
        return justification;
    }
    memset(justification, 0, sizeof(struct str_atms_justification));

    if (NULL == (justification->antecedents = array_new(NULL, NULL))) {
        free(justification);
        return NULL;
    }

    return justification;
}

static void atms_justification_free(atms_justification justification)
{
    array_free(justification->antecedents);

    free(justification);
}

static void atms_node_free(atms_node node)
{
    array_free(node->consequences);

    trie_free(node->label);

    free(node);
}

/* Interface. */
int atms_add_node(atms tms, const signed char assumption,
                  const signed char contradiction)
{
    atms_node node = (atms_node)malloc(sizeof(struct str_atms_node));

    if (NULL == node) {
        return -1;
    }
    memset(node, 0, sizeof(struct str_atms_node));

    node->index = tms->nodes->sz;
    node->assumption = assumption;
    node->contradiction = contradiction;

    node->consequences = array_new(NULL, NULL);

    node->label = trie_new((trie_node_destroy_func_t)array_free,
                           (trie_node_clone_func_t)array_copy);

    array_append(tms->nodes, node);

    if (assumption) {
/* The label of this node is a singleton environment with this node only. */
        array env = array_append(array_new(NULL, NULL), node);
        array key = get_environment_key(env);
        trie_add(node->label, key, env);
        array_free(key);
    }

    return node->index - 1;
}

int atms_add_justification(atms tms, const_material_implication cl)
{
    register unsigned int ix;

    atms_justification justification;

    trie I;

    array key;

    if (NULL == (justification = atms_justification_new())) {
        return 0;
    }

    for (ix = 0; ix < cl->antecedents->sz; ix++) {
        atms_node node = tms->nodes->arr[cl->antecedents->arr[ix] + 1];
        array_append(justification->antecedents, node);

        array_append(node->consequences, justification);
    }
    if (cl->consequent == -1) {
        justification->consequent = tms->nodes->arr[0];
    } else {
        justification->consequent = tms->nodes->arr[cl->consequent + 1];
    }

    array_append(tms->justifications, justification);

    I = trie_new((trie_node_destroy_func_t)array_free,
                 (trie_node_clone_func_t)array_copy);
/* Append the empty environment. */
    key = array_new(NULL, NULL);
    trie_add(I, key, array_new(NULL, NULL));
    array_free(key);
    propagate(tms, justification, NULL, I);
    trie_free(I);

    return 1;
}

static void propagate(atms tms, atms_justification J, atms_node a, trie I)
{
    trie L = weave(tms, a, I, J->antecedents);

    if (!trie_is_empty(L)) {
        update(tms, L, J->consequent);
    }
    trie_free(L);
}

static void update(atms tms, trie L, atms_node n)
{
    register unsigned int ix;

    atms_justification J;

    trie_remove_subsumed_trie(L, n->label);
    trie_remove_subsumed_trie(n->label, L);
    trie_add_trie(n->label, L);

    if (n->index == 0) { /* This is a nogood. */
        nogood(tms, L);
        return;
    }

    for (ix = 0; ix < n->consequences->sz; ix++) {
        J = (atms_justification)n->consequences->arr[ix];
        propagate(tms, J, n, L);

        trie_remove_not_subsumed_trie(L, n->label);
        if (trie_is_empty(L)) {
            break;
        }
    }
}

static int cmp_nodes(const void *a, const void *b)
{
    const atms_node *na = (const atms_node *)a;
    const atms_node *nb = (const atms_node *)b;

    return (*na)->index - (*nb)->index;
}

/* Make a new environment by taking the union of two environments. */
static void *unify_environments(void *lhs, void *rhs)
{
    register unsigned int ix;

    array result = array_concat(array_copy(lhs), array_copy(rhs));

    lydia_qsort(result->arr, result->sz, sizeof(result->arr[0]), cmp_nodes);

/* Remove duplicate nodes. */
    for (ix = result->sz - 1; ix < result->sz; ix--) {
        if (ix > 0) {
            atms_node na = result->arr[ix];
            atms_node nb = result->arr[ix - 1];
            if (nb->index == na->index) {
                array_delete(result, ix);
            }
        }
    }

    return result;
}

static array get_environment_key(array env)
{
    register unsigned int ix;

    array result = array_new(NULL, NULL);

    for (ix = 0; ix < env->sz; ix++) {
        atms_node node = (atms_node)env->arr[ix];
        array_append(result, i2p(node->index));
    }

    return result;
}

static trie weave(atms tms, atms_node a, trie I, array X)
{
    register unsigned int ix;

    atms_node F;
    atms_node h;

    trie result = trie_copy(I);

    for (ix = 0; ix < X->sz; ix++) {
        h = (atms_node)X->arr[ix];
        if (h != a) {
            trie_multiply_trie(result, h->label, unify_environments);
        }
    }

    F = (atms_node)tms->nodes->arr[0];

    trie_remove_subsumed_trie(result, F->label);

    return result;
}

static void nogood(atms tms, trie F)
{
    register unsigned int ix;

    for (ix = 1; ix < tms->nodes->sz; ix++) {
        atms_node node = (atms_node)tms->nodes->arr[ix];

        trie_remove_subsumed_trie(node->label, F);
    }
}

array atms_get_labels(atms tms, int node_idx)
{
    atms_node node = (atms_node)tms->nodes->arr[node_idx + 1];

    return trie_get_all_values(node->label);
}

array atms_get_nogoods(atms tms)
{
    atms_node F = (atms_node)tms->nodes->arr[0];

    return trie_get_all_values(F->label);
}
