#include "sorted_int_list.h"
#include "priority_queue.h"
#include "variable.h"
#include "dnf_tree.h"
#include "gotcha.h"
#include "inline.h"
#include "config.h"
#include "qsort.h"
#include "cones.h"
#include "diag.h"
#include "stat.h"
#include "util.h"
#include "defs.h"
#include "mv.h"
#include "tv.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

static signed char *gotcha_var_buffer;

static cones_context cones;

static gotcha_node gotcha_node_new(const unsigned int systems)
{
    gotcha_node result = (gotcha_node)malloc(sizeof(struct str_gotcha_node));
    if (NULL == result) {
        return result;
    }

    result->offsets = (unsigned short int *)malloc(sizeof(unsigned short int) * systems);
    memset(result->offsets, 0, sizeof(unsigned short int) * systems);
    result->depth = 0;
    result->cardinality = 0;

    return result;
}

static gotcha_node gotcha_node_copy(const_gotcha_node s,
                                    const unsigned int systems)
{
    gotcha_node result = (gotcha_node)malloc(sizeof(struct str_gotcha_node));
    if (NULL == result) {
        return result;
    }

    result->offsets = (unsigned short int *)malloc(sizeof(unsigned short int) * systems);
    if (NULL == result->offsets) {
        free(result);
        return NULL;
    }
    memcpy(result->offsets, s->offsets, sizeof(unsigned short int) * systems);
    result->depth = s->depth;

    return result;
}

static void gotcha_node_free(gotcha_node s)
{
    free(s->offsets);
    free(s);
}

static int get_systems(dnf_tree node, tv_term_list_list systems)
{
    register unsigned int ix;

    if (node->terms->sz != 1 ||
        node->terms->arr[0]->pos->sz > 0 ||
        node->terms->arr[0]->neg->sz > 0) {
        append_tv_term_list_list(systems, node->terms);
    }
    for (ix = 0; ix < node->kids->sz; ix++) {
        if (!get_systems(node->kids->arr[ix], systems)) {
            return 0;
        }
    }
    return 1;
}

static int cmp_offsets(unsigned short int *a,
                       unsigned short int *b,
                       unsigned int systems)
{
    register unsigned int ix;

    for (ix = 0; ix < systems; ix++) {
        if (a[ix] > b[ix]) {
            return 1;
        }
        if (a[ix] < b[ix]) {
            return -1;
        }
    }
    return 0;
}

static int cmp_nodes(void *context, void *itema, void *itemb)
{
    unsigned int systems = (unsigned int)context;

    const_gotcha_node gotcha_nodea = (const_gotcha_node)itema;
    const_gotcha_node gotcha_nodeb = (const_gotcha_node)itemb;

    unsigned int ca = gotcha_nodea->cardinality;
    unsigned int cb = gotcha_nodeb->cardinality;

    return ca > cb ? -1 : (ca < cb ? 1 : cmp_offsets(gotcha_nodea->offsets, gotcha_nodeb->offsets, systems));
}

static int is_diagnosis(const_gotcha_node current, tv_term_list_list systems)
{
    return current->depth == systems->sz;
}

static tv_term build_term(const_gotcha_node current,
                          const_tv_term_list_list systems)
{
    register unsigned int ix;
    register unsigned int iy;

    int_list pos;
    int_list neg;

    tv_term result;

    if (systems->sz == 0) {
        return new_tv_term(new_int_list(), new_int_list());
    }

    if (NULL == (result = rdup_tv_term(systems->arr[0]->arr[current->offsets[0]]))) {
        return NULL;
    }
    for (ix = 1; ix < current->depth; ix++) {
        iy = current->offsets[ix];
        if (NULL == (pos = merge_sorted_int_list(result->pos,
                                                 systems->arr[ix]->arr[iy]->pos))) {
            rfre_tv_term(result);
            return NULL;
        }
        if (NULL == (neg = merge_sorted_int_list(result->neg,
                                                 systems->arr[ix]->arr[iy]->neg))) {
            rfre_tv_term(result);
            return NULL;
        }
        rfre_int_list(result->pos);
        rfre_int_list(result->neg);
        result->pos = pos;
        result->neg = neg;
    }
    return result;
}

static int check_and_add_term(const_tv_term term)
{
    register unsigned int iz;

    const_int_list neg = term->neg;
    const_int_list pos = term->pos;

    for (iz = 0; iz < neg->sz; iz++) {
        if (1 == gotcha_var_buffer[neg->arr[iz]]) {
            return 0;
        }
        gotcha_var_buffer[neg->arr[iz]] = -1;
    }
    for (iz = 0; iz < pos->sz; iz++) {
        if (-1 == gotcha_var_buffer[pos->arr[iz]]) {
            return 0;
        }
        gotcha_var_buffer[pos->arr[iz]] = 1;
    }
    return 1;
}

#if 0
static int check_term(const_tv_term term)
{
    register unsigned int iz;

    const_int_list neg = term->neg;
    const_int_list pos = term->pos;

    for (iz = 0; iz < neg->sz; iz++) {
        if (1 == gotcha_var_buffer[neg->arr[iz]]) {
            return 0;
        }
    }
    for (iz = 0; iz < pos->sz; iz++) {
        if (-1 == gotcha_var_buffer[pos->arr[iz]]) {
            return 0;
        }
    }
    return 1;
}
#endif

static int is_consistent(diagnostic_problem problem,
                         const_gotcha_node node,
                         const_tv_term_list_list systems)
{
    register unsigned int ix;
#if 0
    register unsigned int iy;
#endif

    memset(gotcha_var_buffer, 0, sizeof(signed char) * problem->variables->sz);
/*
 * Move backwards in the loop below, as it is more likely recently
 * added terms to conflict.
 */
    for (ix = node->depth - 1; ix < node->depth; ix--) {
        if (!check_and_add_term(systems->arr[ix]->arr[node->offsets[ix]])) {
            return 0;
        }
    }
#if 0
/* Check forward consistency with unit systems. */
    for (ix = node->depth; ix < systems->sz; ix++) {
        const_tv_term_list system = systems->arr[ix];
        unsigned int consistent_terms = 0;
        unsigned int last_consistent = 0;

        for (iy = 0; iy < system->sz; iy++) {
            if (check_term(system->arr[iy])) {
                consistent_terms += 1;
                last_consistent = iy;
            }
        }
        if (0 == consistent_terms) {
            increase_int_counter("forward inconsistent");
            return 0;
        }
        if (1 == consistent_terms) {
            if (!check_and_add_term(system->arr[last_consistent])) {
                increase_int_counter("forward inconsistent");
                return 0;
            }
        }
    }
#endif
    return 1;
}

static void update_cardinality(diagnostic_problem problem, gotcha_node current)
{
    register unsigned int ix;

    current->cardinality = 0;
    for (ix = 0; ix < problem->tv_cache->assumables; ix++) {
        if (gotcha_var_buffer[problem->tv_cache->a_to_v[ix]] == 1) {
            if (!(problem->tv_cache->health_states[ix] & 2)) {
                current->cardinality += 1;
            }
            continue;
        }
        if (gotcha_var_buffer[problem->tv_cache->a_to_v[ix]] == -1) {
            if (!(problem->tv_cache->health_states[ix] & 1)) {
                current->cardinality += 1;
            }
        }
    }
}

static int enqueue_sibling(diagnostic_problem problem,
                           priority_queue opened,
                           gotcha_node current,
                           const_tv_term_list_list systems)
{
    register unsigned int ix;
    gotcha_node sibling;

    if (current->depth == 0) {
        return 1;
    }
    ix = current->depth - 1;
    if (NULL == (sibling = gotcha_node_copy(current, systems->sz))) {
        return 0;
    }
    while (sibling->offsets[ix] < systems->arr[ix]->sz - 1) {
        sibling->offsets[ix] += 1;
        if (is_consistent(problem, sibling, systems)) {
            update_cardinality(problem, sibling);
            if (!priority_queue_push(opened, sibling)) {
                gotcha_node_free(sibling);
                return 0;
            }
            increase_int_counter("pushed");
            maximize_int_counter("max", priority_queue_size(opened));
            return 1;
        }
        increase_int_counter("inconsistent");
    }
    gotcha_node_free(sibling);
    return 1;
}

static int enqueue_next_best_state(diagnostic_problem problem,
                                   priority_queue opened,
                                   gotcha_node current,
                                   const_tv_term_list_list systems)
{
    register unsigned int ix;

    current->depth += 1;

    ix = current->depth - 1;
    while (current->offsets[ix] < systems->arr[ix]->sz) {
        if (is_consistent(problem, current, systems)) {
            update_cardinality(problem, current);
            if (!priority_queue_push(opened, current)) {
                return 0;
            }
            increase_int_counter("pushed");
            maximize_int_counter("max", priority_queue_size(opened));
            return 1;
        }
        current->offsets[ix] += 1;
        increase_int_counter("inconsistent");
    }
    gotcha_node_free(current);
    return 1;
}

int gotcha_diag(diagnostic_problem problem, const_tv_term alpha)
{
    priority_queue opened;
    gotcha_node initial;

    dnf_tree consistent_input;
    tv_term_list_list systems;

    unsigned int old_diagnoses;

    int terminate = 0;
    int result = 1;

    diagnostic_problem_reset(problem);

    sort_int_list(alpha->neg);
    sort_int_list(alpha->pos);

    start_stopwatch("observation filtering time");
    consistent_input = dnf_tree_filter(dnf_tree_copy(problem->u.tv_tdnf_sd.model),
                                       alpha);
    stop_stopwatch("observation filtering time");
    systems = new_tv_term_list_list();

    get_systems(consistent_input, systems);

/* Start the timer. */
    start_stopwatch("search time");

    opened = priority_queue_new(cmp_nodes,
                                (priority_queue_element_destroy_func_t)gotcha_node_free,
                                (void *)systems->sz);
    initial = gotcha_node_new(systems->sz);
    priority_queue_push(opened, initial);
    increase_int_counter("pushed");
    maximize_int_counter("max", 1);

    while (!terminate && !priority_queue_empty(opened)) {
        gotcha_node current = priority_queue_pop(opened);
        candidate_found(current->cardinality);
        if (is_terminate()) {
            gotcha_node_free(current);
            break;
        }

        if (current->depth != 0) {
/* The root node has no siblings. */
            if (!enqueue_sibling(problem, opened, current, systems)) {
                result = 0;

                gotcha_node_free(current);
                break;
            }
        }
        if (is_diagnosis(current, systems)) {
            tv_term diagnosis = build_term(current, systems);
            if (NULL == diagnosis) {
                result = 0;

                gotcha_node_free(current);
                break;
            }
            old_diagnoses = problem->diagnoses->sz;
            if (!add_diagnosis_from_tv_term(problem,
                                            diagnosis,
                                            &terminate,
                                            1)) {
                result = 0;
                rfre_tv_term(diagnosis);
                gotcha_node_free(current);
                break;
            }


            if (cones != NULL && problem->diagnoses->sz - 1 == old_diagnoses) {
                assert(problem->diagnoses->sz > 0);
/* @todo: Fix the termination here. */
                expand_cone_exhaustive(cones,
                                       alpha,
                                       problem->diagnoses->arr[problem->diagnoses->sz - 1]);
            }

            rfre_tv_term(diagnosis);
            gotcha_node_free(current);
        } else {
            if (!enqueue_next_best_state(problem, opened, current, systems)) {
                result = 0;

                gotcha_node_free(current);
                break;
            }
        }
    }

    priority_queue_free(opened);
    fre_tv_term_list_list(systems);

/* Stop the timer. */
    stop_stopwatch("search time");

    dnf_tree_free(consistent_input);

    return result;
}

void gotcha_destroy_input(diagnostic_problem problem)
{
    if (NULL != problem->encoded_variables) {
        rfre_variable_list(problem->encoded_variables);
    }
    if (NULL != problem->variables) {
        rfre_variable_list(problem->variables);
    }
    if (NULL != problem->domains) {
        rfre_values_set_list(problem->domains);
    }

    if (NULL != problem->u.tv_tdnf_sd.model) {
        dnf_tree_free(problem->u.tv_tdnf_sd.model);
    }

    free(gotcha_var_buffer);
}

int gotcha_set_input(diagnostic_problem problem,
                     const_tv_dnf_hierarchy input,
                     const_node root)
{
    dnf_tree sd;
    int_list_list variable_mappings;

    unsigned int ix = 0;

    start_stopwatch("preprocessing time");

    if (int_list_listNIL == (variable_mappings = new_int_list_list())) {
        return 0;
    }

    if (values_set_listNIL == (problem->domains = new_values_set_list())) {
        rfre_int_list_list(variable_mappings);
    }
    if (variable_listNIL == (problem->variables = new_variable_list())) {
        rfre_int_list_list(variable_mappings);
        rfre_values_set_list(problem->domains);
    }
    if (variable_listNIL == (problem->encoded_variables = new_variable_list())) {
        rfre_int_list_list(variable_mappings);
        rfre_values_set_list(problem->domains);
        rfre_variable_list(problem->encoded_variables);
    }

    inline_variables(to_hierarchy(input),
                     root,
                     mapping_listNIL,
                     problem->domains,
                     problem->variables,
                     problem->encoded_variables,
                     constant_listNIL,
                     variable_mappings,
                     int_list_listNIL,
                     NULL);

    sd = dnf_tree_make(to_const_hierarchy(input), root, variable_mappings, &ix);

    problem->u.tv_tdnf_sd.model = sd;

    rfre_int_list_list(variable_mappings);

    problem->encoding = root->constraints->encoding;

    gotcha_var_buffer = (signed char *)malloc(problem->variables->sz * sizeof(signed char));
    if (NULL == gotcha_var_buffer) {
        return 0;
    }

    problem->tv_cache = initialize_tv_variables_cache(problem->variables);
    problem->mv_cache = initialize_mv_variables_cache(problem->encoded_variables,
                                                      problem->domains);

    dnf_tree_sort(sd, sd, problem->tv_cache, problem->mv_cache, problem->encoding);

    stop_stopwatch("preprocessing time");

    return 1;
}

void gotcha_init(diagnostic_problem problem,
                 const_tv_dnf_hierarchy input,
                 const_node root)
{
    stat_init();

    gotcha_set_input(problem, input, root);

    init_int_counter("inconsistent", "inconsistent states: %d", "search tree");
    init_int_counter("forward inconsistent", "unit check inconsistent states: %d", "search tree");
    init_int_counter("pushed", "nodes pushed: %d", "opened queue");
    init_int_counter("max", "maximum size: %d nodes", "opened queue");

    init_stopwatch("search time", "search time: %d s %d.%d ms", "dynamics");
    init_stopwatch("preprocessing time", "preprocessing time: %d s %d.%d ms", "dynamics");
    init_stopwatch("observation filtering time", "observation filtering time: %d s %d.%d ms", "dynamics");
}

void gotcha_set_cones(cones_context context)
{
    cones = context;
}

void gotcha_destroy(diagnostic_problem problem)
{
    gotcha_destroy_input(problem);

    stat_destroy();
}
