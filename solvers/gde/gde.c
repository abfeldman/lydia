#include "card_sort_terms.h"
#include "sorted_int_list.h"
#include "pp_variable.h"
#include "config.h"
#include "queue.h"
#include "stat.h"
#include "defs.h"
#include "diag.h"
#include "atms.h"
#include "gde.h"
#include "tv.h"

#include <assert.h>

extern void atms_print(FILE *, atms, const_variable_list);

atms tms;

static int option_mincard = 0;

typedef struct str_mhs_node *mhs_node;
typedef const struct str_mhs_node *const_mhs_node;

struct str_mhs_node
{
    array elements;
};

static mhs_node mhs_node_new()
{
    mhs_node result = (mhs_node)malloc(sizeof(struct str_mhs_node));
    if (NULL == result) {
        return result;
    }

    result->elements = array_new(NULL, NULL);

    return result;
}

static mhs_node mhs_node_copy(const_mhs_node rhs)
{
    mhs_node result = (mhs_node)malloc(sizeof(struct str_mhs_node));
    if (NULL == result) {
        return result;
    }
    result->elements = array_copy(rhs->elements);

    return result;
}

static void mhs_node_free(mhs_node node)
{
    array_free(node->elements);

    free(node);
}

static void push_children(queue q, mhs_node node, array universe)
{
    register unsigned int ix, iy = 0;

    mhs_node child;

    if (node->elements->sz == universe->sz) {
        return;
    }

    if (node->elements->sz > 0) {
        iy = (unsigned int)(node->elements->arr[node->elements->sz - 1]) + 1;
    }
    for (ix = iy; ix < universe->sz; ix++) {
        child = mhs_node_copy(node);
        array_append(child->elements, (void *)ix);
        queue_push(q, child);
    }
}
/*
static void mhs_node_print(mhs_node node)
{
    register unsigned int ix;

    for (ix = 0; ix < node->elements->sz; ix++) {
        if (ix != 0) {
            printf(", ");
        }
        printf("%d", (int)node->elements->arr[ix]);
    }
    printf("\n");
}
*/
static int intersect(array nogood, array candidate, array universe)
{
    register unsigned int ix, iy;

    for (ix = 0; ix < candidate->sz; ix++) {
        for (iy = 0; iy < nogood->sz; iy++) {
            atms_node node = (atms_node)nogood->arr[iy];
            if (node->index - 1 == (int)universe->arr[(int)candidate->arr[ix]]) {
                return 1;
            }
        }
    }
    return 0;
}

static int is_hs(array nogoods, array candidate, array universe)
{
    register unsigned int ix;

    for (ix = 0; ix < nogoods->sz; ix++) {
        if (!intersect(nogoods->arr[ix], candidate, universe)) {
            return 0;
        }
    }
    return 1;
}

void bfs(diagnostic_problem problem, array nogoods, array universe)
{
    register unsigned int ix;
    FILE* outfile = stdout;
    int terminate = 0;
    int current_min_cardinality = problem->tv_cache->assumables;
    queue q = queue_new(NULL, NULL);

    queue_push(q, mhs_node_new());
    while (!terminate && queue_size(q) != 0) {
        mhs_node n = queue_pop(q);

        /* Prune nodes over current mincardinality */
        if(option_mincard && (current_min_cardinality<n->elements->sz))
        	break;

        candidate_found(n->elements->sz);
        if (is_terminate()) {
            break;
        }

        if (is_hs(nogoods, n->elements, universe)) {
            tv_term term = new_tv_term(new_int_list(), new_int_list());
            for (ix = 0; ix < n->elements->sz; ix++) {
                append_int_list(term->neg, (int)universe->arr[(int)n->elements->arr[ix]]);
            }

            if (!add_diagnosis_from_tv_term(problem,
                                            term,
                                            &terminate,
                                            1)) {
                break;
            }
            rfre_tv_term(term);
        } else {
            push_children(q, n, universe);
        }
        mhs_node_free(n);
    }

    queue_free(q);
}

array unify_nogoods(array nogoods, const_variable_list variables)
{
    register unsigned int ix, iy;

    signed char *buf;
    array result;

    buf = (signed char *)malloc(sizeof(signed char) * variables->sz);
    if (NULL == buf) {
        return NULL;
    }
    memset(buf, 0, sizeof(signed char) * variables->sz);

    result = array_new(NULL, NULL);

    for (ix = 0; ix < nogoods->sz; ix++) {
        array nogood = nogoods->arr[ix];
        for (iy = 0; iy < nogood->sz; iy++) {
            atms_node node = nogood->arr[iy];
            assert(node->index > 0);
            assert(node->index <= (int)variables->sz);
            if (!buf[node->index - 1]) {
                array_append(result, (void *)(node->index - 1));
            }
            buf[node->index - 1] = 1;
        }
    }
    array_int_sort(result);
/*
    for (ix = 0; ix < result->sz; ix++) {
        if (ix != 0) {
            printf(", ");
        }
        printf("%d", (int)result->arr[ix]);
    }
    printf("\n");
*/
    free(buf);

    return result;
}

int gde_diag(diagnostic_problem problem, const_tv_term alpha)
{
    register unsigned int ix;
    unsigned int iy;
    FILE* outfile = stdout;
    material_implication premise;

    array nogoods;
    array suspect_components;

    int rc = 0;

    diagnostic_problem_reset(problem);
    start_stopwatch("search time");

    tms = atms_new();

    start_stopwatch("ATMS time");
    for (ix = 0; ix < problem->variables->sz; ix++) {
        atms_add_node(tms, is_health(problem->variables->arr[ix]), 0);
    }
    for (ix = 0; ix < problem->u.horn_sd->clauses->sz; ix++) {
        atms_add_justification(tms, problem->u.horn_sd->clauses->arr[ix]);
    }

    for (ix = 0; ix < alpha->neg->sz; ix++) {
        variable nvar = rdup_variable(problem->variables->arr[alpha->neg->arr[ix]]);

        char *buf = (char *)malloc(strlen(nvar->name->name->name) + 20);
        if (NULL == buf) {
            rfre_variable(nvar);

            goto exit;
        }
        buf[0] = '~';
        strncpy(buf + 1, nvar->name->name->name, strlen(nvar->name->name->name) + 1);
        nvar->name->name = add_lydia_symbol(buf);
        free(buf);

        if (!search_variable_list(problem->variables, nvar->name, &iy)) {
            rfre_variable(nvar);

            goto exit;
        }
        rfre_variable(nvar);

        premise = new_material_implication(new_int_list(), iy);
        atms_add_justification(tms, premise);
        rfre_material_implication(premise);
    }
    for (ix = 0; ix < alpha->pos->sz; ix++) {
        premise = new_material_implication(new_int_list(), alpha->pos->arr[ix]);
        atms_add_justification(tms, premise);
        rfre_material_implication(premise);
    }
    stop_stopwatch("ATMS time");
/*
    atms_print(stdout, tms, problem->variables);
*/
    nogoods = atms_get_nogoods(tms);
    suspect_components = unify_nogoods(nogoods, problem->variables);
    bfs(problem, nogoods, suspect_components);
    array_free(suspect_components);
    array_free(nogoods);

    rc = 1;

exit:
    atms_free(tms);
    /* TODO: Check if SD was inconsistent with OBS in the first place */
    /* TODO: Currently assumes that the only reason for not finding diagnoses is a timeout */
    if(problem->diagnoses->sz==0)
    	fprintf(outfile, "GDE: Terminated by timeout @ ");
    return rc;
}

static tv_term environment_to_tv_term(array env)
{
    register unsigned int ix;

    tv_term result = new_tv_term(new_int_list(), new_int_list());

    for (ix = 0; ix < env->sz; ix++) {
        atms_node node = (atms_node)env->arr[ix];

        assert(node->index != 0);

        append_int_list(result->pos, node->index - 1);
    }

    return result;
}

int gde_conflicts(FILE *outfile,
                  diagnostic_problem problem,
                  const_tv_term alpha)
{
    register unsigned int ix;
    unsigned int iy;

    array nogoods;

    int rc = 0;

    material_implication premise;

    diagnostic_problem_reset(problem);

    tms = atms_new();

    start_stopwatch("ATMS time");
    for (ix = 0; ix < problem->variables->sz; ix++) {
        atms_add_node(tms, is_health(problem->variables->arr[ix]), 0);
    }
    for (ix = 0; ix < problem->u.horn_sd->clauses->sz; ix++) {
        atms_add_justification(tms, problem->u.horn_sd->clauses->arr[ix]);
    }

    for (ix = 0; ix < alpha->neg->sz; ix++) {
        variable nvar = rdup_variable(problem->variables->arr[alpha->neg->arr[ix]]);

        char *buf = (char *)malloc(strlen(nvar->name->name->name) + 20);
        if (NULL == buf) {
            rfre_variable(nvar);

            goto exit;
        }
        buf[0] = '~';
        strncpy(buf + 1, nvar->name->name->name, strlen(nvar->name->name->name) + 1);
        nvar->name->name = add_lydia_symbol(buf);
        free(buf);

        if (!search_variable_list(problem->variables, nvar->name, &iy)) {
            rfre_variable(nvar);

            goto exit;
        }
        rfre_variable(nvar);

        premise = new_material_implication(new_int_list(), iy);
        atms_add_justification(tms, premise);
        rfre_material_implication(premise);
    }
    for (ix = 0; ix < alpha->pos->sz; ix++) {
        premise = new_material_implication(new_int_list(), alpha->pos->arr[ix]);
        atms_add_justification(tms, premise);
        rfre_material_implication(premise);
    }
    stop_stopwatch("ATMS time");

    nogoods = atms_get_nogoods(tms);
    for (ix = 0; ix < nogoods->sz; ix++) {
        tv_term nogood = environment_to_tv_term(nogoods->arr[ix]);

        pp_tv_term_short(outfile, problem->variables, nogood);

        fprintf(outfile, "\n");

        rfre_tv_term(nogood);
    }
    array_free(nogoods);

    rc = 1;

exit:
	stop_stopwatch("search time");
    atms_free(tms);

    return rc;
}

int gde_init(diagnostic_problem UNUSED(problem),
        const int mincard)
{
    stat_init();

    option_mincard = mincard;

    init_stopwatch("ATMS time", "ATMS time: %d s %d.%d ms", "dynamics");
    init_stopwatch("search time", "search time: %d s %d.%d ms", "dynamics");

    return 1;
}

void gde_destroy()
{
    stat_destroy();
}
