#include "sorted_int_list.h"
#include "priority_queue.h"
#include "consistency.h"
#include "lreadline.h"
#include "variable.h"
#include "rand_nr.h"
#include "config.h"
#include "debug.h"
#include "qsort.h"
#include "util.h"
#include "cdas.h"
#include "defs.h"
#include "lsss.h"
#include "stat.h"
#include "ltms.h"
#include "diag.h"
#include "math.h"
#include "tv.h"

#include <assert.h>
#ifndef WIN32
# include <sys/time.h>
#else
# include <Winsock2.h>
#endif

static signed char *cdas_var_buffer;

static int option_conflicts = 0;
static int option_mincard = 0;
static int option_direct = 0;
static int option_dpll = 0;

static ltms tms = NULL;

ltms cdas_get_tms()
{
    return tms;
}

static void cdas_node_update_cost(cdas_node node,
                                  const_tv_variables_cache tv_cache)
{
    register unsigned int ix;
    register unsigned int cardinality = 0;
    signed char fg = 1;
    double g = 0.0;
    double h = 0.0;

    memset(cdas_var_buffer, 0, tv_cache->assumables * sizeof(signed char));
    for (ix = 0; ix < node->assignments->sz; ix++) {
        cdas_literal lit = (cdas_literal)node->assignments->arr[ix];
        assert(lit->var < (int)tv_cache->assumables);
        if (lit->sign) {
            if ((tv_cache->health_states[lit->var] & 2) == 0) {
                cardinality += 1;
            }
        } else {
            if ((tv_cache->health_states[lit->var] & 1) == 0) {
                cardinality += 1;
            }
        }
        cdas_var_buffer[lit->var] = 1;
        if (fg) {
            g = 1 / (lit->sign ?
                         tv_cache->p_true[lit->var] :
                         tv_cache->p_false[lit->var]);
            fg = 0;
            continue;
        }
        g *= 1 / (lit->sign ?
                      tv_cache->p_true[lit->var] :
                      tv_cache->p_false[lit->var]);
    }

    fg = 1;
    for (ix = 0; ix < tv_cache->assumables; ix++) {
        if (cdas_var_buffer[ix]) {
            continue;
        }
        if (fg) {
            h = 1 / tv_cache->p_max[ix];
            fg = 0;
            continue;
        }
        h *= 1 / tv_cache->p_max[ix];
    }

    node->cost = g + h;
    node->cardinality = cardinality;
}

static const_tv_variables_cache tv_cache = NULL;

static int better_constituent_kernel(const void *a, const void *b)
{
    const cdas_literal la = *((const cdas_literal *)a);
    const cdas_literal lb = *((const cdas_literal *)b);

    double ca = 1 / (la->sign ? tv_cache->p_true[la->var] : tv_cache->p_false[la->var]);
    double cb = 1 / (lb->sign ? tv_cache->p_true[lb->var] : tv_cache->p_false[lb->var]);

    if (fabs(ca - cb) < EPSILON) {
        if (la->var > lb->var) {
            return 1;
        }
        if (la->var < lb->var) {
            return -1;
        }
    }
    if (ca > cb) {
        return 1;
    }
    if (ca < cb) {
        return -1;
    }

    return 0;
}

static int kgp_node_contains_kernel(array constituent_kernels)
{
    register unsigned int ix;

    for (ix = 0; ix < constituent_kernels->sz; ix++) {
        cdas_literal kernel = constituent_kernels->arr[ix];
        if (cdas_var_buffer[kernel->var] == kernel->sign) {
            return 1;
        }
    }

    return 0;
}

static array goal_test_kernel(cdas_context kgp)
{
    register unsigned int ix;

    for (ix = 0; ix < kgp->constituent_kernels->sz; ix++) {
        array k_gamma = (array)kgp->constituent_kernels->arr[ix];

        if (!kgp_node_contains_kernel(k_gamma)) {
            return k_gamma;
        }
    }

    return NULL;
}

static int next_variable(diagnostic_problem problem,
                         cdas_context kgp,
                         cdas_node node,
                         int *var,
                         signed char *sign)
{
    register unsigned int ix;

    array k_gamma;

    memset(cdas_var_buffer, -1, sizeof(signed char) * problem->tv_cache->assumables);

    for (ix = 0; ix < node->assignments->sz; ix++) {
        cdas_literal lit = (cdas_literal)node->assignments->arr[ix];
        if (lit->sign == CDAS_LITERAL_FALSE) {
            cdas_var_buffer[lit->var] = CDAS_LITERAL_FALSE;
        } else if (lit->sign == CDAS_LITERAL_TRUE) {
            cdas_var_buffer[lit->var] = CDAS_LITERAL_TRUE;
        } else {
            assert(0);
            abort();
        }
    }

    k_gamma = goal_test_kernel(kgp);
    if (NULL != k_gamma) {
        for (ix = 0; ix < k_gamma->sz; ix++) {
            cdas_literal lit = (cdas_literal)k_gamma->arr[ix];
            if (cdas_var_buffer[lit->var] == -1) {
                *var = lit->var;
                *sign = lit->sign;
                return 1;
            }
        }
    }

    for (ix = 0; ix < problem->tv_cache->assumables; ix++) {
        if (cdas_var_buffer[ix] == -1) {
            break;
        }
    }
    assert(ix < problem->tv_cache->assumables);

    *var = (int)ix;
    *sign = -1;

    return 1;
}

static array cdas_constituent_kernels_new(diagnostic_problem problem, ltms tms)
{
    register unsigned int ix, iy;

    array constituent_kernels = array_new((array_element_destroy_func_t)cdas_literal_free,
                                          (array_element_clone_func_t)cdas_literal_copy);
    array conflict = ltms_get_conflict_assumptions(tms);
    for (ix = 0; ix < conflict->sz; ix++) {
        ltms_literal lit = (ltms_literal)conflict->arr[ix];
        ltms_node node = lit->proposition;
        iy = problem->tv_cache->v_to_a[node->index];
        array_append(constituent_kernels, cdas_literal_new(iy, lit->sign));
    }
    array_free(conflict);

    tv_cache = problem->tv_cache;

    lydia_qsort(constituent_kernels->arr,
                constituent_kernels->sz,
                sizeof(constituent_kernels->arr[0]),
                better_constituent_kernel);

    return constituent_kernels;
}

static tv_term assignments_to_tv_term(diagnostic_problem problem,
                                      array assignments)
{
    register unsigned int ix;

    tv_term result = new_tv_term(new_int_list(), new_int_list());

    for (ix = 0; ix < assignments->sz; ix++) {
        cdas_literal lit = (cdas_literal)assignments->arr[ix];
        append_int_list(lit->sign ? result->pos : result->neg,
                        problem->tv_cache->a_to_v[lit->var]);
    }

    return result;
}

static int is_diagnosis(diagnostic_problem problem, cdas_node node)
{
    return node->assignments->sz == problem->tv_cache->assumables;
}

static cdas_node expand_child_best_state(diagnostic_problem problem,
                                         array result,
                                         cdas_node node)
{
    register unsigned int iy = 0;

    cdas_node child;
    cdas_literal lit;

    if (node->assignments->sz > 0) {
        lit = ((cdas_literal)node->assignments->arr[node->assignments->sz - 1]);
        iy = lit->var + 1;
    }
    if (iy == problem->tv_cache->assumables) {
        return NULL;
    }

    child = cdas_node_copy(node);

    lit = cdas_literal_new(iy, problem->tv_cache->p_true[iy] > 0.5 ? CDAS_LITERAL_TRUE : CDAS_LITERAL_FALSE);
    array_append(child->assignments, lit);

    cdas_node_update_cost(child, problem->tv_cache);

    array_append(result, child);

    return child;
}

static int expand_sibling(diagnostic_problem problem,
                          array result,
                          cdas_node node)
{
    int var;
    signed char sign;
    cdas_node sibling;
    cdas_literal literal;

    if (node->assignments->sz == 0) {
        return 0;
    }

    literal = ((cdas_literal)node->assignments->arr[node->assignments->sz - 1]);
    var = literal->var;
    sign = literal->sign;

    if (problem->tv_cache->p_true[var] > 0.5) {
        if (!sign) {
            return 0;
        }
        sibling = cdas_node_copy(node);
        ((cdas_literal)sibling->assignments->arr[sibling->assignments->sz - 1])->sign = CDAS_LITERAL_FALSE;
    } else {
        if (sign) {
            return 0;
        }
        sibling = cdas_node_copy(node);
        ((cdas_literal)sibling->assignments->arr[sibling->assignments->sz - 1])->sign = CDAS_LITERAL_TRUE;
    }
    cdas_node_update_cost(sibling, problem->tv_cache);

    array_append(result, sibling);

    return 1;
}

static cdas_node get_parent(cdas_node node)
{
    cdas_node parent;

    if (node->assignments->sz == 0) {
        return NULL;
    }
    parent = cdas_node_copy(node);
    array_delete(parent->assignments, node->assignments->sz - 1);

    return parent;
}

void expand_ancestors_siblings(diagnostic_problem problem,
                               array result,
                               cdas_node node)
{
    node = cdas_node_copy(node);
    while (expand_sibling(problem, result, node)) {
        cdas_node parent = get_parent(node);
        if (NULL == parent) {
            break;
        }
        cdas_node_free(node);
        node = parent;
    }
    if (NULL != node) {
        cdas_node_free(node);
    }
}

array expand_variable(diagnostic_problem problem, cdas_node node)
{
    array result = array_new(NULL, NULL);

    cdas_node child = expand_child_best_state(problem, result, node);

    if (NULL != child) {
        expand_sibling(problem, result, child);
    }
/*
    if (NULL != child && is_diagnosis(problem, child)) {
        expand_ancestors_siblings(problem, result, child);
    }
*/

    return result;
}

static void enqueue_root(cdas_context ocsp)
{
/* Enqueue the root node. */
    cdas_node root = cdas_node_new(NULL);
    root->assignments = array_new((array_element_destroy_func_t)cdas_literal_free,
                                  (array_element_clone_func_t)cdas_literal_copy);
    priority_queue_push(ocsp->nodes, root);
    increase_int_counter("pushed");
    maximize_int_counter("max", priority_queue_size(ocsp->nodes));
}

int cbas_search(diagnostic_problem problem, const_tv_term obs)
{
    register unsigned int ix;
    FILE* outfile = stdout;
    int terminate = 0;
    array new_nodes;

    cdas_context ocsp;
    cdas_node node;

    tv_clause_list alpha = term_to_tv_clause_list(obs);
    tv_term candidate;

    ocsp = cdas_context_new();

    enqueue_root(ocsp);

    while (!terminate && !priority_queue_empty(ocsp->nodes)) {
        node = priority_queue_pop(ocsp->nodes);
        candidate_found(node->cardinality);
        if (is_terminate()) {
        	if(is_timeout()){
				fprintf(outfile, "CBAS: Terminated by timeout @ ");
				print_stopwatch(outfile, "search time");
				fprintf(outfile, "\n");
        	}
            cdas_node_free(node);
            break;
        }

        new_nodes = expand_variable(problem, node);
        for (ix = 0; ix < new_nodes->sz; ix++) {
            priority_queue_push(ocsp->nodes, new_nodes->arr[ix]);
            increase_int_counter("pushed");
            maximize_int_counter("max", priority_queue_size(ocsp->nodes));
        }
        array_free(new_nodes);

        if (is_diagnosis(problem, node)) {
            increase_int_counter("ltms_calls");
            if (cdas_assume_health(tms,
                                   problem->tv_cache,
                                   node->assignments)) {
                candidate = assignments_to_tv_term(problem, node->assignments);
                if (!option_dpll || cdas_is_sat(problem->u.tv_cnf_sd,
                                                alpha,
                                                candidate)) {
                    add_diagnosis_from_tv_term(problem, candidate, &terminate, 1);
                }
                rfre_tv_term(candidate);
            } else {
                increase_int_counter("ltms_unsat");
            }
            cdas_retract_health(tms, problem->tv_cache, node->assignments);
        }

        cdas_node_free(node);
    }

    cdas_context_free(ocsp);

    rfre_tv_clause_list(alpha);

    return 1;
}

static array conflict_label(array conflict)
{
    register unsigned int ix;

    array result = array_new(NULL, NULL);

    for (ix = 0; ix < conflict->sz; ix++) {
        cdas_literal lit = (cdas_literal)conflict->arr[ix];
        if (lit->sign) {
            array_append(result, (void *)(lit->var * 2 + 1));
        } else {
            array_append(result, (void *)(lit->var * 2));
        }
    }
    array_int_sort(result);

    return result;
}

static int better_conflict(const void *a, const void *b)
{
    const array aa = *((const array *)a);
    const array ab = *((const array *)b);

    return aa->sz - ab->sz;
}

static void enqueue_child(diagnostic_problem problem,
                          cdas_context kgp,
                          cdas_node parent,
                          int var,
                          signed char sign)
{
    cdas_node child = cdas_node_copy(parent);

    array_append(child->assignments, cdas_literal_new(var, sign));
    cdas_node_update_cost(child, problem->tv_cache);

    increase_int_counter("ltms_calls");
    if (cdas_assume_health(tms, problem->tv_cache, child->assignments)) {
        cdas_retract_health(tms, problem->tv_cache, child->assignments);

        priority_queue_push(kgp->nodes, child);
        increase_int_counter("pushed");
        maximize_int_counter("max", priority_queue_size(kgp->nodes));
    } else {
        array conflict;
        array label;

        increase_int_counter("ltms_unsat");

        conflict = cdas_constituent_kernels_new(problem, tms);
        label = conflict_label(conflict);
        if (!trie_is_subsumed(kgp->constituent_kernels_trie, label)) {
            array_append(kgp->constituent_kernels, conflict);
            lydia_qsort(kgp->constituent_kernels->arr,
                        kgp->constituent_kernels->sz,
                        sizeof(kgp->constituent_kernels->arr[0]),
                        better_conflict);
            trie_add(kgp->constituent_kernels_trie, label, NULL);
        } else {
            array_free(conflict);
        }
        array_free(label);

        cdas_retract_health(tms, problem->tv_cache, child->assignments);

        cdas_node_free(child);
    }
}

int cdas_search(diagnostic_problem problem, const_tv_term obs)
{
    cdas_context kgp;
    cdas_node node;
    tv_clause_list alpha = term_to_tv_clause_list(obs);
    tv_term candidate;
    FILE* outfile = stdout;
    int terminate = 0;
    int var;
    signed char sign;
    int current_min_cardinality=problem->tv_cache->assumables;
    kgp = cdas_context_new();

    enqueue_root(kgp);

    while (!terminate && !priority_queue_empty(kgp->nodes)) {
        node = priority_queue_pop(kgp->nodes);

        /* If we want only the minimal cardinality diagnosis, prune candidates with larger cardinality */
        if(option_mincard && (node->cardinality>current_min_cardinality)){
        	cdas_node_free(node);
        	continue;
        }

        candidate_found(node->cardinality);
        if (is_terminate()) {
        	if(is_timeout()){
				fprintf(outfile, "CDAS: Terminated by timeout @ ");
				print_stopwatch(outfile, "search time");
				fprintf(outfile, "\n");
        	}
        	cdas_node_free(node);
            break;
        }

        if (is_diagnosis(problem, node)) {
            candidate = assignments_to_tv_term(problem, node->assignments);
            
            if (!option_dpll || cdas_is_sat(problem->u.tv_cnf_sd,
                                            alpha,
                                            candidate)) {

            	add_diagnosis_from_tv_term(problem, candidate, &terminate, 1);

            	/* Update current minimal cardinality */
            	if(candidate->neg->sz<current_min_cardinality)
            		current_min_cardinality=candidate->neg->sz;
            }
            rfre_tv_term(candidate);
        } else {
            next_variable(problem, kgp, node, &var, &sign);

            enqueue_child(problem, kgp, node, var, CDAS_LITERAL_FALSE);
            enqueue_child(problem, kgp, node, var, CDAS_LITERAL_TRUE);
        }
        cdas_node_free(node);
    }
/* Clean-up. */
    cdas_context_free(kgp);

    rfre_tv_clause_list(alpha);

    return 1;
}

static cdas_node make_nominal_node(diagnostic_problem problem)
{
    register unsigned int ix;

    cdas_node node = cdas_node_new(NULL);
    cdas_literal lit;

    node->assignments = array_new((array_element_destroy_func_t)cdas_literal_free,
                                  (array_element_clone_func_t)cdas_literal_copy);

    for (ix = 0; ix < problem->tv_cache->assumables; ix++) {
        lit = cdas_literal_new(ix,
                               problem->tv_cache->p_true[ix] > 0.5 ?
                                   CDAS_LITERAL_TRUE :
                                   CDAS_LITERAL_FALSE);
        array_append(node->assignments, lit);
    }

    return node;
}

static void set_node_literal(cdas_node node, int var, signed char sign)
{
    assert(((cdas_literal)node->assignments->arr[var])->var == var);

    ((cdas_literal)node->assignments->arr[var])->sign = sign;
}

static int check_single_fault(ltms tms,
                              const_tv_variables_cache tv_cache,
                              int l1_var,
                              signed char l1_sign)
{
    ltms_retract_assumption(tms, tv_cache->a_to_v[l1_var]);
    ltms_enable_assumption(tms, tv_cache->a_to_v[l1_var], l1_sign);

    return !ltms_has_contradiction(tms);
}

static void undo_check_single_fault(ltms tms,
                                    const_tv_variables_cache tv_cache,
                                    int l1_var,
                                    signed char l1_sign)
{
    ltms_retract_assumption(tms, tv_cache->a_to_v[l1_var]);
    ltms_enable_assumption(tms, tv_cache->a_to_v[l1_var], !l1_sign);
}

static int check_double_fault(ltms tms,
                              const_tv_variables_cache tv_cache,
                              int l1_var,
                              int l2_var,
                              signed char l1_sign,
                              signed char l2_sign)
{
    ltms_retract_assumption(tms, tv_cache->a_to_v[l1_var]);
    ltms_enable_assumption(tms, tv_cache->a_to_v[l1_var], l1_sign);
    if (ltms_has_contradiction(tms)) {
        return 0;
    }
    ltms_retract_assumption(tms, tv_cache->a_to_v[l2_var]);
    ltms_enable_assumption(tms, tv_cache->a_to_v[l2_var], l2_sign);

    return !ltms_has_contradiction(tms);
}

static void undo_check_double_fault(ltms tms,
                                    const_tv_variables_cache tv_cache,
                                    int l1_var,
                                    int l2_var,
                                    signed char l1_sign,
                                    signed char l2_sign)
{
    ltms_retract_assumption(tms, tv_cache->a_to_v[l1_var]);
    ltms_enable_assumption(tms, tv_cache->a_to_v[l1_var], !l1_sign);
    ltms_retract_assumption(tms, tv_cache->a_to_v[l2_var]);
    ltms_enable_assumption(tms, tv_cache->a_to_v[l2_var], !l2_sign);
}

static int assume_nominal(ltms tms, const_tv_variables_cache tv_cache)
{
    register unsigned int ix;

    for (ix = 0; ix < tv_cache->assumables; ix++) {
        ltms_enable_assumption(tms,
                               tv_cache->a_to_v[ix],
                               tv_cache->p_true[ix] > 0.5 ?
                                   CDAS_LITERAL_TRUE :
                                   CDAS_LITERAL_FALSE);
    }

    return !ltms_has_contradiction(tms);
}

static array get_maximal_conflict(ltms engine, const_tv_variables_cache cache)
{
    register unsigned int ix;

    ltms_clause clause = NULL;

    ltms_node node;

    array result = array_new((array_element_destroy_func_t)cdas_literal_free,
                             (array_element_clone_func_t)cdas_literal_copy);
    stack clauses = stack_new(NULL, NULL);

    signed char *buf = (signed char *)malloc(sizeof(signed char) * engine->nodes->sz);
    if (buf == NULL) {
        array_free(result);
        stack_free(clauses);

        return NULL;
    }
    memset(buf, 0, sizeof(signed char) * engine->nodes->sz);

    for (ix = 0; ix < engine->nodes->sz; ix++) {
        node = (ltms_node)engine->nodes->arr[ix];
        if ((ltms_node_is_true(node) && ltms_node_is_enabled_false(node)) ||
            (ltms_node_is_false(node) && ltms_node_is_enabled_true(node))) {
            if (node->supporting_clause != NULL) {
                stack_push(clauses, node->supporting_clause);
            }
        }
    }
    for (ix = 0; ix < engine->clauses->sz; ix++) {
        clause = (ltms_clause)engine->clauses->arr[ix];
        if (clause->pvs == 0 && clause->satisfying_node == NULL) {
            stack_push(clauses, clause);
        }
    }

    while (!stack_empty(clauses)) {
        clause = (ltms_clause)stack_pop(clauses);

        for (ix = 0; ix < clause->literals->sz; ix++) {
            ltms_literal lit = (ltms_literal)clause->literals->arr[ix];
            if (buf[lit->proposition->index]) {
                continue;
            }
            buf[lit->proposition->index] = 1;
            if (ltms_node_has_flag(lit->proposition, LTMS_NODE_FG_ASSUMPTION)) {
                array_append(result,
                             cdas_literal_new(cache->v_to_a[lit->proposition->index],
                                              lit->sign));
            } else {
                if (NULL != lit->proposition->supporting_clause) {
                    stack_push(clauses, lit->proposition->supporting_clause);
                }
            }
        }
    }

    stack_free(clauses);

    free(buf);

    tv_cache = cache;

    lydia_qsort(result->arr,
                result->sz,
                sizeof(result->arr[0]),
                better_constituent_kernel);

    return result;
}

static int diagnose_double_faults(diagnostic_problem problem,
                                  const_tv_term obs)
{
    tv_clause_list alpha;

    array conflict = NULL;

    cdas_node node = NULL;

    int l1_var;
    int l2_var;
    signed char l1_sign;
    signed char l2_sign;

    register unsigned int ix;
    register unsigned int iy;

    signed char *buffer = NULL;

    signed char has_single_fault = 0;

    tv_term candidate;

    int terminate = 0;
    int rc = 1;

    alpha = term_to_tv_clause_list(obs);

    node = make_nominal_node(problem);
    candidate_found(0);
    if (is_terminate()) {
        goto exit;
    }

    increase_int_counter("ltms_calls");
    if (assume_nominal(tms, problem->tv_cache)) {
        candidate = assignments_to_tv_term(problem, node->assignments);
            
        if (!option_dpll || cdas_is_sat(problem->u.tv_cnf_sd,
                                        alpha,
                                        candidate)) {
            add_diagnosis_from_tv_term(problem, candidate, &terminate, 0);
        }
        rfre_tv_term(candidate);

        goto exit;
    }

    increase_int_counter("ltms_unsat");
    conflict = get_maximal_conflict(tms, problem->tv_cache);

    fprintf(stdout, "conflict size: %d\n", conflict->sz);

    unsigned int ia;
    signed char sampling = 0;
    unsigned int sample;
    rand_nr_ctx rnr;
    array new_conflict = array_new(NULL, NULL);
    if (conflict->sz > 100) {
        sampling = 1;

        rnr = rand_nr_start(conflict->sz);
        for (ia = 0; ia < conflict->sz / 10; ia++) {
            if ((unsigned int)-1 == (sample = rand_nr(rnr))) {
                assert(0);
            }
            array_append(new_conflict, conflict->arr[sample]);
        }
        rand_nr_end(rnr);

/*        array_free(conflict); */
        conflict = new_conflict;
    }

/* Check all single faults. */
    for (ix = 0; !terminate && ix < conflict->sz; ix++) {
        l1_var = ((cdas_literal)conflict->arr[ix])->var;
        l1_sign = ((cdas_literal)conflict->arr[ix])->sign;

        set_node_literal(node, l1_var, l1_sign);

        candidate_found(1);
        if (is_terminate()) {
            goto exit;
        }

        increase_int_counter("ltms_calls");
        if (check_single_fault(tms, problem->tv_cache, l1_var, l1_sign)) {
            candidate = assignments_to_tv_term(problem, node->assignments);
            if (!option_dpll || cdas_is_sat(problem->u.tv_cnf_sd,
                                            alpha,
                                            candidate)) {
                add_diagnosis_from_tv_term(problem, candidate, &terminate, 0);
            }
            rfre_tv_term(candidate);

            has_single_fault = 1;
        } else {
            increase_int_counter("ltms_unsat");
        }
        undo_check_single_fault(tms, problem->tv_cache, l1_var, l1_sign);
        set_node_literal(node, l1_var, !l1_sign);
    }

    if (has_single_fault) {
        goto exit;
    }

/* Check all double faults. */
    buffer = (signed char *)malloc(problem->tv_cache->assumables * sizeof(signed char));
    if (NULL == buffer) {
        rc = 0;

        goto exit;
    }
    memset(buffer, 0, problem->tv_cache->assumables * sizeof(signed char));
    for (ix = 0; ix < conflict->sz; ix++) {
        l1_var = ((cdas_literal)conflict->arr[ix])->var;
        l1_sign = ((cdas_literal)conflict->arr[ix])->sign;

        buffer[l1_var] = 1;

        check_single_fault(tms, problem->tv_cache, l1_var, l1_sign);

        set_node_literal(node, l1_var, l1_sign);
        for (iy = ix + 1; iy < conflict->sz; iy++) {
            l2_var = ((cdas_literal)conflict->arr[iy])->var;
            l2_sign = ((cdas_literal)conflict->arr[iy])->sign;
/*
        for (iy = 0; !terminate && iy < problem->tv_cache->assumables; iy++) {

            if (buffer[iy]) {
                continue;
            }

            l2_var = iy;
            l2_sign = problem->tv_cache->p_true[iy] > 0.5 ?
                          CDAS_LITERAL_FALSE :
                          CDAS_LITERAL_TRUE;
*/
            set_node_literal(node, l2_var, l2_sign);

            candidate_found(2);
            if (is_terminate()) {
                free(buffer);

                goto exit;
            }

            increase_int_counter("ltms_calls");
            if (check_single_fault(tms, problem->tv_cache, l2_var, l2_sign)) {
                candidate = assignments_to_tv_term(problem, node->assignments);
            
                if (!option_dpll || cdas_is_sat(problem->u.tv_cnf_sd,
                                                alpha,
                                                candidate)) {
                    add_diagnosis_from_tv_term(problem, candidate, &terminate, 0);
                }
                rfre_tv_term(candidate);
            } else {
                increase_int_counter("ltms_unsat");
            }
            undo_check_single_fault(tms, problem->tv_cache, l2_var, l2_sign);
            set_node_literal(node, l2_var, !l2_sign);
        }

        undo_check_single_fault(tms, problem->tv_cache, l1_var, l1_sign);

        set_node_literal(node, l1_var, !l1_sign);
    }
    free(buffer);

/*
    if (sampling) {
        unsigned int ib, id;
        unsigned int ic = problem->diagnoses->sz;

        for (ib = 0; ib < 99; ib++) {
            for (id = 0; id < ic; id++) {
                array_append(problem->diagnoses,
                             rdup_faultmode(problem->diagnoses->arr[id]));
            }
        }
    }
*/

exit:
    cdas_retract_health(tms, problem->tv_cache, node->assignments);

    if (NULL != conflict) {
        array_free(conflict);
    }

    if (NULL != node) {
        cdas_node_free(node);
    }

    rfre_tv_clause_list(alpha);

    return rc;
}

int cdas_diag(diagnostic_problem problem, const_tv_term alpha)
{
    int rc = 1;

/* Start the timer. */
    if (!diagnostic_problem_reset(problem)) {
        return 0;
    }

    start_stopwatch("search time");

    if (cdas_enable_observation(tms, alpha)) {
        if (option_direct) {
            diagnose_double_faults(problem, alpha);
        } else {
            if (option_conflicts) {
                rc = cdas_search(problem, alpha);
            } else {
                rc = cbas_search(problem, alpha);
            }
        }
    }
    cdas_retract_observation(tms, alpha);

/* Stop the timer. */
    stop_stopwatch("search time");

    return rc;
}

int cdas_init(diagnostic_problem problem,
              const int conflicts,
              const int mincard,
              const int direct,
              const int dpll)
{
    register unsigned int ix;

    option_conflicts = conflicts;
    option_mincard = mincard;
    option_direct = direct;
    option_dpll = dpll;

    stat_init();

    init_int_counter("pushed", "nodes pushed: %d", "A* queue");
    init_int_counter("max", "maximum size: %d nodes", "A* queue");
    init_int_counter("dpll_calls", "%d calls", "DPLL");
    init_int_counter("dpll_unsat", "UNSAT: %d", "DPLL");
    init_int_counter("ltms_calls", "%d calls", "LTMS");
    init_int_counter("ltms_unsat", "UNSAT: %d", "LTMS");
    init_stopwatch("search time", "search time: %d s %d.%d ms", "dynamics");

    cdas_var_buffer = (signed char *)malloc(problem->tv_cache->assumables * sizeof(signed char));
    if (NULL == cdas_var_buffer) {
        return 0;
    }

/* Now go, initialize the LTMS. */
    if (NULL == (tms = ltms_new())) {
        return 0;
    }

    for (ix = 0; ix < problem->u.tv_cnf_sd->variables->sz; ix++) {
        ltms_add_node(tms,
                      ix,
                      is_health(problem->u.tv_cnf_sd->variables->arr[ix]),
                      is_observable(problem->u.tv_cnf_sd->variables->arr[ix]));
    }
    for (ix = 0; ix < problem->u.tv_cnf_sd->clauses->sz; ix++) {
        ltms_add_clause(tms, problem->u.tv_cnf_sd->clauses->arr[ix]);
    }
    return 1;
}

void cdas_destroy()
{
    if (NULL != tms) {
        ltms_free(tms);
    }

    if (NULL != cdas_var_buffer) {
        free(cdas_var_buffer);
    }

    stat_destroy();
}

int cdas_obs(diagnostic_problem problem, const_tv_term obs)
{
    tv_term candidate;
    tv_clause_list alpha;

    int rc = 1;

    if (cdas_enable_observation(tms, obs)) {
        candidate = new_tv_term(new_int_list(), new_int_list());
        alpha = term_to_tv_clause_list(obs);
        if (cdas_is_sat(problem->u.tv_cnf_sd,
                        alpha,
                        candidate)) {
            printf("@ info SAT\n");
        } else {
            printf("@ info UNSAT\n");
        }
        rfre_tv_clause_list(alpha);
        rfre_tv_term(candidate);
    } else {
        printf("@ info UNSAT\n");
    }
    cdas_retract_observation(tms, obs);

    return rc;
}
