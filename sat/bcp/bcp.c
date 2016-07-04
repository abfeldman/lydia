#include "tv.h"
#include "bcp.h"
#include "defs.h"
#include "list.h"
#include "queue.h"
#include "bcp_context.h"
#include "sorted_int_list.h"

#include <string.h>
#include <assert.h>
#ifdef WIN32
# include <Windows.h>
#endif

#ifndef WIN32
# define INT_PTR int
#endif

static void satisfy_clauses(bcp_problem problem, int_list clauses)
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

static int filter_clauses(bcp_problem problem, int_list clauses)
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
            bcp_variable var_ctx = problem->variables[var];
            if (var_ctx->assignment != ASSIGNMENT_UNKNOWN) {
                continue;
            }
            if (NULL == var_ctx->pos_filter_clause) {
                var_ctx->pos_filter_clause = problem->clauses->arr[clause_idx];
                queue_push(problem->unit_variables_neg, i2p(var));
                problem->unit_variables_neg_map[var] = 1;
                if (problem->unit_variables_pos_map[var]) {
                    result = 1;
                }
            }
        }
        for (iy = 0; iy < clause->pos->sz; iy++) {
            int var = clause->pos->arr[iy];
            bcp_variable var_ctx = problem->variables[var];
            if (var_ctx->assignment != ASSIGNMENT_UNKNOWN) {
                continue;
            }
            if (NULL == var_ctx->neg_filter_clause) {
                var_ctx->neg_filter_clause = problem->clauses->arr[clause_idx];
                queue_push(problem->unit_variables_pos, i2p(var));
                problem->unit_variables_pos_map[var] = 1;
                if (problem->unit_variables_neg_map[var]) {
                    result = 1;
                }
            }
        }
    }

    return result;
}

static int deduce(bcp_problem problem)
{
    int fg;
    do {
        fg = 0;
        if (!queue_empty(problem->unit_variables_pos)) {
            int var;
            var = p2i(queue_pop(problem->unit_variables_pos));
            problem->unit_variables_pos_map[var] = 0;
            problem->variables[var]->assignment = ASSIGNMENT_TRUE;
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
            problem->variables[var]->assignment = ASSIGNMENT_FALSE;
            satisfy_clauses(problem, problem->variables[var]->neg_clauses);
            if (filter_clauses(problem, problem->variables[var]->pos_clauses)) {
                return 1;
            }
            fg = 1;
        }
    } while (fg);

    return 0;
}

static void get_result(bcp_problem problem, tv_term solution)
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

static int sat(bcp_problem problem)
{
    if (!problem->consistent || deduce(problem)) {
        return UNSAT;
    }
    if (problem->clauses->sz != problem->satisfied_clauses) {
        return UNKNOWN;
    }

    return SAT;
}

/**
 * Determines if a CNF is consistent or not.
 *
 * @param clauses a set of clauses;
 * @param variables the number of variables in the CNF.
 * @returns 1 if the CNF is satisfiable, 0 otherwise.
 */
int bcp_is_consistent(tv_clause_list clauses, unsigned int variables)
{
    bcp_problem problem = bcp_new_problem(clauses, variables);
    int result = sat(problem);
    bcp_free_problem(problem);

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
 * @returns 1 if the CNF is satisfiable, 0 if not and 2 if the BCP
 *          algorithm can not determine satisfiability.
 */
int bcp_get_solution(tv_clause_list clauses,
                     unsigned int variables,
                     tv_term *solution)
{
    int result;

    bcp_problem problem = bcp_new_problem(clauses, variables);
    if ((result = sat(problem)) == SAT) {
        *solution = new_tv_term(new_int_list(), new_int_list());
        get_result(problem, *solution);
    }
    bcp_free_problem(problem);

    return result;
}
