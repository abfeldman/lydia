#include "defs.h"
#include "lsss.h"
#include "stack.h"
#include "qsort.h"
#include "lsss_context.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#ifdef WIN32
# include <Windows.h>
#endif

#ifndef WIN32
# define INT_PTR int
#endif

#undef CHRONOLOGICAL_BACKTRACKING

static unsigned int decide(dpll_problem problem)
{
    register unsigned int ix, iy, iz, unassigned;

    if (problem->randomized) {
        for (ix = unassigned = 0; ix < problem->variables_count; ix++) {
            if (problem->variables[ix]->assignment == ASSIGNMENT_UNKNOWN) {
                unassigned += 1;
            }
        }
        assert(unassigned > 0);
        ix = rand() % unassigned;
        for (iy = iz = 0; iy < problem->variables_count; iy++) {
            if (problem->variables[iy]->assignment == ASSIGNMENT_UNKNOWN) {
                if (iz == ix) {
                    return iy;
                }
                iz += 1;
            }
        }
    } else {
        for (ix = 0; ix < problem->variables_count; ix++) {
            if (problem->variables[ix]->assignment == ASSIGNMENT_UNKNOWN) {
                return ix;
            }
        }
    }
/*
 * Never reach this point, because if there is no unassigned variable
 * we should have terminated before calling this method.
 */
    assert(0);
    abort();
}

static void assign_variable(dpll_problem problem, unsigned int var, char val)
{
    problem->variables[var]->assignment = val;
    problem->assignment_history[problem->assigned_variables] = var;
    problem->current_variable = var;

    problem->assigned_variables += 1;
}

static void satisfy_clauses(dpll_problem problem, int_list clauses)
{
    register unsigned int ix;
    for (ix = 0; ix < clauses->sz; ix++) {
        int clause = clauses->arr[ix];
        problem->labelled[clause] += 1;
        if (problem->satisfied[clause] == 0) {
            problem->satisfied[clause] = problem->labelled[clause];
            problem->satisfied_clauses += 1;
        }
    }
}

static int filter_clauses(dpll_problem problem, int_list clauses)
{
    int result = 0;

    register unsigned int ix, iy;
    for (ix = 0; ix < clauses->sz; ix++) {
        int clause_idx = clauses->arr[ix];
        const_tv_clause clause = problem->clauses->arr[clause_idx];
        unsigned int literals = clause->pos->sz + clause->neg->sz;
        problem->labelled[clause_idx] += 1;
        if (problem->satisfied[clause_idx] != 0 || problem->labelled[clause_idx] != literals - 1) {
            continue;
        }
        for (iy = 0; iy < clause->neg->sz; iy++) {
            int var = clause->neg->arr[iy];
            dpll_variable var_ctx = problem->variables[var];
            if (var_ctx->assignment != ASSIGNMENT_UNKNOWN) {
                continue;
            }
            if (NULL == var_ctx->pos_filter_clause) {
                var_ctx->pos_filter_clause = problem->clauses->arr[clause_idx];
                queue_push(problem->unit_variables_neg, i2p(var));
                problem->unit_variables_neg_map[var] = 1;
                if (problem->unit_variables_pos_map[var]) {
                    problem->contradiction_variable = (unsigned int)var;
                    result = 1;
                }
            }
        }
        for (iy = 0; iy < clause->pos->sz; iy++) {
            int var = clause->pos->arr[iy];
            dpll_variable var_ctx = problem->variables[var];
            if (var_ctx->assignment != ASSIGNMENT_UNKNOWN) {
                continue;
            }
            if (NULL == var_ctx->neg_filter_clause) {
                var_ctx->neg_filter_clause = problem->clauses->arr[clause_idx];
                queue_push(problem->unit_variables_pos, i2p(var));
                problem->unit_variables_pos_map[var] = 1;
                if (problem->unit_variables_neg_map[var]) {
                    problem->contradiction_variable = (unsigned int)var;
                    result = 1;
                }
            }
        }
    }

    return result;
}

static int deduce(dpll_problem problem)
{
    int fg;
    do {
        fg = 0;
        if (!queue_empty(problem->unit_variables_pos)) {
            int var;
            var = p2i(queue_pop(problem->unit_variables_pos));
            problem->unit_variables_pos_map[var] = 0;
            assign_variable(problem, var, ASSIGNMENT_TRUE);
            satisfy_clauses(problem, problem->variables[var]->pos_clauses);
            if (filter_clauses(problem, problem->variables[var]->neg_clauses)) {
                return 1;
            }
            fg = 1;
        }
        if (!queue_empty(problem->unit_variables_neg)) {
            int var;
            var = p2i(queue_pop(problem->unit_variables_neg));
            problem->unit_variables_neg_map[var] = 0;
            assign_variable(problem, var, ASSIGNMENT_FALSE);
            satisfy_clauses(problem, problem->variables[var]->neg_clauses);
            if (filter_clauses(problem, problem->variables[var]->pos_clauses)) {
                return 1;
            }
            fg = 1;
        }
    } while (fg);

    return 0;
}

static void clear_unit_variables(dpll_problem problem)
{
    while (!queue_empty(problem->unit_variables_pos)) {
        problem->variables[p2i(queue_pop(problem->unit_variables_pos))]->neg_filter_clause = NULL;
    }
    while (!queue_empty(problem->unit_variables_neg)) {
        problem->variables[p2i(queue_pop(problem->unit_variables_neg))]->pos_filter_clause = NULL;
    }
    memset(problem->unit_variables_pos_map, 0, sizeof(unsigned char) * problem->variables_count);
    memset(problem->unit_variables_neg_map, 0, sizeof(unsigned char) * problem->variables_count);
}

static void undo_current_assignment(dpll_problem problem)
{
    register unsigned int ix;

    int_list sat;
    int_list unsat;

    dpll_variable current_variable_ctx = problem->variables[problem->current_variable];
    if (current_variable_ctx->assignment == ASSIGNMENT_TRUE) {
        sat = current_variable_ctx->pos_clauses;
        unsat = current_variable_ctx->neg_clauses;
    } else {
        assert(current_variable_ctx->assignment == ASSIGNMENT_FALSE);
        sat = current_variable_ctx->neg_clauses;
        unsat = current_variable_ctx->pos_clauses;
    }

    for (ix = 0; ix < sat->sz; ix++) {
        if (problem->satisfied[sat->arr[ix]] == problem->labelled[sat->arr[ix]]) {
            problem->satisfied_clauses -= 1;
            problem->satisfied[sat->arr[ix]] = 0;
        }
        problem->labelled[sat->arr[ix]] -= 1;
    }
    for (ix = 0; ix < unsat->sz; ix++) {
        problem->labelled[unsat->arr[ix]] -= 1;
    }
}

#ifdef CHRONOLOGICAL_BACKTRACKING
# define chronological_backtrack backtrack
#endif

static int chronological_backtrack(dpll_problem problem)
{
    clear_unit_variables(problem);
    while (1) {
        dpll_variable current_variable_ctx = problem->variables[problem->current_variable];

        undo_current_assignment(problem);
        if (NULL == current_variable_ctx->pos_filter_clause &&
            NULL == current_variable_ctx->neg_filter_clause) {
            problem->branch_points += 1;
            if (current_variable_ctx->assignment == ASSIGNMENT_FALSE) {
                current_variable_ctx->neg_filter_clause = problem->empty_clause;
                queue_push(problem->unit_variables_pos, i2p(problem->current_variable));
                problem->unit_variables_pos_map[problem->current_variable] = 1;
            } else {
                assert(current_variable_ctx->assignment == ASSIGNMENT_TRUE);
                current_variable_ctx->pos_filter_clause = problem->empty_clause;
                queue_push(problem->unit_variables_neg, i2p(problem->current_variable));
                problem->unit_variables_neg_map[problem->current_variable] = 1;
            }
            problem->assigned_variables -= 1;
            if (!deduce(problem)) {
                return 0;
            }
            clear_unit_variables(problem);
            continue;
        }
        if (problem->assigned_variables == 1) {
            return 1;
        }
        current_variable_ctx->assignment = ASSIGNMENT_UNKNOWN;
        current_variable_ctx->pos_filter_clause = NULL;
        current_variable_ctx->neg_filter_clause = NULL;
        problem->assigned_variables -= 1;
        if (problem->assigned_variables == 0) {
            problem->current_variable = (unsigned int)-1;
            return 0;
        }
        problem->current_variable = problem->assignment_history[problem->assigned_variables - 1];
    }
}

#ifndef CHRONOLOGICAL_BACKTRACKING
static void pop_variable(dpll_problem problem)
{
    dpll_variable current_variable_ctx = problem->variables[problem->current_variable];
    if (NULL != current_variable_ctx->delete_clause) {
        rfre_tv_clause(current_variable_ctx->delete_clause);
        current_variable_ctx->delete_clause = NULL;
    }
/* Pop the next variable from the decision stack. */
    current_variable_ctx->assignment = ASSIGNMENT_UNKNOWN;
    current_variable_ctx->pos_filter_clause = NULL;
    current_variable_ctx->neg_filter_clause = NULL;
    problem->assigned_variables -= 1;
    assert(problem->assigned_variables != 0);
    problem->current_variable = problem->assignment_history[problem->assigned_variables - 1];
}

static tv_clause make_conflict(dpll_problem problem)
{
    tv_clause pos_fail_clause, neg_fail_clause, result;

    dpll_variable contradiction_variable_ctx = problem->variables[problem->contradiction_variable];

    assert(contradiction_variable_ctx->pos_filter_clause != NULL);
    assert(contradiction_variable_ctx->neg_filter_clause != NULL);

    pos_fail_clause = contradiction_variable_ctx->pos_filter_clause;
    neg_fail_clause = contradiction_variable_ctx->neg_filter_clause;

    clear_unit_variables(problem);

    result = rdup_tv_clause(pos_fail_clause);

    merge_int_list(result->pos, neg_fail_clause->pos);
    merge_int_list(result->neg, neg_fail_clause->neg);

    remove_int_list(result->pos, problem->contradiction_variable);
    remove_int_list(result->neg, problem->contradiction_variable);

    return result;
}

static void resolve_conflict(dpll_problem problem, tv_clause cl1, tv_clause cl2)
{
    merge_int_list(cl1->pos, cl2->pos);
    merge_int_list(cl1->neg, cl2->neg);
    remove_int_list(cl1->pos, problem->current_variable);
    remove_int_list(cl1->neg, problem->current_variable);
}

static void learn(dpll_problem problem, const_tv_clause cl)
{
/* Only non-unit clauses can be learnt at this stage. */
    if (cl->pos->sz + cl->neg->sz == 1) {
        return;
    }
    if (cl->pos->sz + cl->neg->sz > 10) {
        return;
    }
    dpll_add_clause(problem, rdup_tv_clause(cl));
}

static int backtrack(dpll_problem problem)
{
    tv_clause conflict_clause = make_conflict(problem);
    learn(problem, conflict_clause);

    while (1) {
        dpll_variable current_variable_ctx = problem->variables[problem->current_variable];

        undo_current_assignment(problem);
        if (!member_int_list(conflict_clause->pos, problem->current_variable) &&
            !member_int_list(conflict_clause->neg, problem->current_variable)) {
            pop_variable(problem);
            continue;
        }
        if (NULL == current_variable_ctx->pos_filter_clause &&
            NULL == current_variable_ctx->neg_filter_clause) {
            problem->branch_points += 1;
            if (NULL != current_variable_ctx->delete_clause) {
                rfre_tv_clause(current_variable_ctx->delete_clause);
            }
            current_variable_ctx->delete_clause = conflict_clause;
            if (current_variable_ctx->assignment == ASSIGNMENT_FALSE) {
                current_variable_ctx->neg_filter_clause = conflict_clause;
                queue_push(problem->unit_variables_pos, i2p(problem->current_variable));
                problem->unit_variables_pos_map[problem->current_variable] = 1;
            } else {
                assert(current_variable_ctx->assignment == ASSIGNMENT_TRUE);
                current_variable_ctx->pos_filter_clause = conflict_clause;
                queue_push(problem->unit_variables_neg, i2p(problem->current_variable));
                problem->unit_variables_neg_map[problem->current_variable] = 1;
            }
            problem->assigned_variables -= 1;
            if (!deduce(problem)) {
                return 0;
            }
            conflict_clause = make_conflict(problem);
            learn(problem, conflict_clause);
            continue;
        }
        resolve_conflict(problem, conflict_clause, current_variable_ctx->neg_filter_clause == NULL ? current_variable_ctx->pos_filter_clause : current_variable_ctx->neg_filter_clause);
        if (conflict_clause->pos->sz == 0 && conflict_clause->neg->sz == 0) {
            rfre_tv_clause(conflict_clause);
            return 1; /* UNSAT */
        }
        learn(problem, conflict_clause);
        pop_variable(problem);
    }
}
#endif

static int sat(dpll_problem problem)
{
    if (!problem->consistent || deduce(problem)) {
        return 0;
    }
    while (problem->assigned_variables != problem->variables_count &&
           problem->clauses->sz != problem->satisfied_clauses) {
        int var = decide(problem);
        if (problem->randomized && rand() % 2) {
            queue_push(problem->unit_variables_pos, i2p(var));
            problem->unit_variables_pos_map[var] = 1;
        } else {
            queue_push(problem->unit_variables_neg, i2p(var));
            problem->unit_variables_neg_map[var] = 1;
        }
        if (deduce(problem)) {
            if (backtrack(problem)) {
                return 0;
            }
        }
    }

    return 1;
}

static void get_result(dpll_problem problem, tv_term solution)
{
    register unsigned int ix;

    int_list pos = solution->pos;
    int_list neg = solution->neg;

    for (ix = 0; ix < problem->variables_count; ix++) {
        if (problem->variables[ix]->assignment == ASSIGNMENT_TRUE) {
            pos = append_int_list(pos, ix);
        } else if (problem->variables[ix]->assignment == ASSIGNMENT_FALSE) {
            neg = append_int_list(neg, ix);
        }
    }
}

static int allsat(dpll_problem problem, tv_term_list result)
{
    tv_clause blocking_clause;

    int solutions = 0;
    if (!problem->consistent || deduce(problem)) {
        return 0;
    }
    while (1) {
        tv_term instance;

        while (problem->assigned_variables != problem->variables_count &&
               problem->clauses->sz != problem->satisfied_clauses) {
            int var = decide(problem);
            queue_push(problem->unit_variables_pos, i2p(var));
            problem->unit_variables_pos_map[var] = 1;
            if (deduce(problem)) {
                if (backtrack(problem)) {
                    return solutions;
                }
            }
        }
        instance = new_tv_term(new_int_list(), new_int_list());
        get_result(problem, instance);
        append_tv_term_list(result, instance);

        solutions += 1;

        if (instance->pos->sz == 0 && instance->neg->sz == 0) {
            return solutions;
        }

        blocking_clause = new_tv_clause(rdup_int_list(instance->neg),
                                        rdup_int_list(instance->pos));
        dpll_add_clause(problem, blocking_clause);
        if (chronological_backtrack(problem)) {
            return solutions;
        }
    }
}

/**
 * Determines if a CNF is consistent or not.
 *
 * @param clauses a set of clauses;
 * @param variables the number of variables in the CNF.
 * @returns 1 if the CNF is satisfiable, 0 otherwise.
 */
int lsss_is_consistent(tv_clause_list clauses, unsigned int variables)
{
    dpll_problem problem = dpll_new_problem(clauses, variables);
    int result = sat(problem);
    dpll_free_problem(problem);

    return result;
}

/**
 * Determines if a CNF is consistent or not and if it is, returns one
 * arbitrary instantiation which satisfies it.
 *
 * @param clauses a set of clauses;
 * @param variables the number of variables in the CNF;
 * @param solution an existing term which will be modified to contain the
 *        satisfying instantiation.
 * @returns 1 if the CNF is satisfiable, 0 otherwise.
 */
int lsss_get_solution(tv_clause_list clauses,
                      unsigned int variables,
                      tv_term *solution)
{
    dpll_problem problem = dpll_new_problem(clauses, variables);
    int result = sat(problem);

    if (result) {
        *solution = new_tv_term(new_int_list(), new_int_list());
        get_result(problem, *solution);
    }

    dpll_free_problem(problem);

    return result;
}

int lsss_get_random_solution(tv_clause_list clauses,
                             unsigned int variables,
                             tv_term *solution)
{
    int result = 0;

    dpll_problem problem = dpll_new_problem(clauses, variables);
    problem->randomized = 1;
    if (sat(problem)) {
        *solution = new_tv_term(new_int_list(), new_int_list());
        get_result(problem, *solution);
        result = 1;
    }
    dpll_free_problem(problem);

    return result;
}

int lsss_get_all_solutions(tv_clause_list clauses, unsigned int variables, tv_term_list solutions)
{
    dpll_problem problem;

    int result;

    if (tv_clause_listNIL == clauses) {
        return 0;
    }

    problem = dpll_new_problem(clauses, variables);
    result = allsat(problem, solutions);
    dpll_free_problem(problem);

    return result;
}
