#include "defs.h"
#include "ds.h"
#include "tv.h"

#include <stdlib.h>
#include <math.h>

static int cmp_assignments(array a, array b)
{
    register unsigned int ix;

    if (a->sz != b->sz) {
        return a->sz - b->sz;
    }

    for (ix = 0; ix < a->sz; ix++) {
        cdas_literal la = (cdas_literal)a->arr[ix];
        cdas_literal lb = (cdas_literal)a->arr[ix];
        if (la->var != lb->var) {
            return la->var - lb->var;
        }
        if (la->sign != lb->sign) {
            return la->sign - lb->sign;
        }
    }

    return 0;
}

static int cmp_kernels(void *UNUSED(context), void *a, void *b)
{
    const cdas_node ka = (const cdas_node)a;
    const cdas_node kb = (const cdas_node)b;

    if (fabs(ka->cost - kb->cost) < EPSILON) {
        return cmp_assignments(ka->assignments, kb->assignments);
    }
    if (ka->cost > kb->cost) {
        return -1;
    }
    if (ka->cost < kb->cost) {
        return 1;
    }
    return 0;
}

cdas_literal cdas_literal_new(const int var, const signed char sign)
{
    cdas_literal literal = (cdas_literal)malloc(sizeof(struct str_cdas_literal));
    if (NULL == literal) {
        return literal;
    }

    literal->var = var;
    literal->sign = sign;

    return literal;
}

cdas_literal cdas_literal_copy(cdas_literal literal)
{
    cdas_literal result = (cdas_literal)malloc(sizeof(struct str_cdas_literal));
    if (NULL == result) {
        return result;
    }

    result->var = literal->var;
    result->sign = literal->sign;

    return result;
}

void cdas_literal_free(cdas_literal literal)
{
    free(literal);
}

cdas_node cdas_node_new()
{
    cdas_node node = (cdas_node)malloc(sizeof(struct str_cdas_node));
    if (NULL == node) {
        return node;
    }

    node->assignments = NULL;
    node->cost = 0.0;
    node->conflict = NULL;
    node->kernel = 0;
    node->cardinality = 0;

    return node;
}

cdas_node cdas_node_copy(const cdas_node node)
{
    cdas_node result = (cdas_node)malloc(sizeof(struct str_cdas_node));
    if (NULL == result) {
        return result;
    }

    result->assignments = array_copy(node->assignments);
    result->cost = node->cost;
    result->conflict = node->conflict;
    result->kernel = node->kernel;

    result->cardinality = node->cardinality;

    return result;
}

void cdas_node_free(cdas_node node)
{
    if (NULL == node) {
        return;
    }

    if (NULL != node->assignments) {
        array_free(node->assignments);
    }

    free(node);
}

cdas_context cdas_context_new()
{
    cdas_context context = (cdas_context)malloc(sizeof(struct str_cdas_context));
    if (NULL == context) {
        return context;
    }
    context->nodes = priority_queue_new(cmp_kernels,
                                        (priority_queue_element_destroy_func_t)cdas_node_free,
                                        NULL);
    context->constituent_kernels = array_new((array_element_destroy_func_t)array_free,
                                             (array_element_clone_func_t)array_copy);
    context->constituent_kernels_trie = trie_new(NULL, NULL);

    return context;
}

void cdas_context_free(cdas_context context)
{
    trie_free(context->constituent_kernels_trie);

    array_free(context->constituent_kernels);
    priority_queue_free(context->nodes);

    free(context);
}
