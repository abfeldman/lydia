#include <string.h>
#include <assert.h>
#include <stdlib.h>
#ifdef WIN32
# include <Windows.h>
#endif

#include "defs.h"
#include "lsss_context.h"
#include "sorted_int_list.h"

#ifndef WIN32
# define INT_PTR int
#endif

static signed char option_simplify_clauses = 1;

void dpll_enable_clause_simplification(const signed char fg)
{
    option_simplify_clauses = fg;
}

static tv_clause_list simplify_clauses(tv_clause_list clauses)
{
    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;

    int_list pos;
    int_list neg;

    for (ix = 0; ix < clauses->sz; ix++) {
        pos = sort_int_list(clauses->arr[ix]->pos);
        neg = sort_int_list(clauses->arr[ix]->neg);
        for (iy = iz = 0; iy < pos->sz; iy++) {
            if (iy == 0 || (pos->arr[iy] != pos->arr[iy - 1])) {
                pos->arr[iz++] = pos->arr[iy];
            }
        }
        pos->sz = iz;
        for (iy = iz = 0; iy < neg->sz; iy++) {
            if (iy == 0 || (neg->arr[iy] != neg->arr[iy - 1])) {
                neg->arr[iz++] = neg->arr[iy];
            }
        }
        neg->sz = iz;
    }

    return clauses;
}

static dpll_variable new_dpll_variable()
{
    dpll_variable var = (dpll_variable)malloc(sizeof(struct str_dpll_variable));
    if (NULL == var) {
        return var;
    }
    var->assignment = ASSIGNMENT_UNKNOWN;
    var->pos_clauses = new_int_list();
    var->neg_clauses = new_int_list();
    setroom_int_list(var->pos_clauses, 512);
    setroom_int_list(var->neg_clauses, 512);

    var->pos_filter_clause = NULL;
    var->neg_filter_clause = NULL;
    var->delete_clause = NULL;

    return var;
}

static void free_dpll_variable(dpll_variable var)
{
    rfre_int_list(var->pos_clauses);
    rfre_int_list(var->neg_clauses);
    free(var);
}

static void initialize_clause(dpll_problem problem, unsigned int clause)
{
    register unsigned int ix;

    const_int_list pos;
    const_int_list neg;
    
    pos = problem->clauses->arr[clause]->pos;
    neg = problem->clauses->arr[clause]->neg;
    for (ix = 0; ix < pos->sz; ix++) {
        append_int_list(problem->variables[pos->arr[ix]]->pos_clauses, clause);
    }
    for (ix = 0; ix < neg->sz; ix++) {
        append_int_list(problem->variables[neg->arr[ix]]->neg_clauses, clause);
    }
}

void dpll_add_clause(dpll_problem problem, tv_clause clause)
{
    append_tv_clause_list(problem->clauses, clause);

    problem->labelled = realloc(problem->labelled, sizeof(unsigned int) * problem->clauses->sz);
    problem->satisfied = realloc(problem->satisfied, sizeof(unsigned int) * problem->clauses->sz);

    problem->satisfied[problem->clauses->sz - 1] = 0;
    problem->labelled[problem->clauses->sz - 1] = clause->pos->sz + clause->neg->sz;

    initialize_clause(problem, problem->clauses->sz - 1);
}

/**
 * Creates a new DPLL problem for use by the DPLL consistency checker
 * or the CNF to DNF converter. This function is used internally by
 * the solvers.
 *
 * @param clauses the input set of clauses;
 * @param variables the number of variables in the CNF.
 * @returns the newly created DPLL problem.
 */
dpll_problem dpll_new_problem(tv_clause_list clauses,
                              const unsigned int variables_count)
{
    register unsigned int ix;

    const_int_list pos;
    const_int_list neg;

    dpll_problem problem = (dpll_problem)malloc(sizeof(struct str_dpll_problem));
    if (NULL == problem) {
        return problem;
    }

    problem->variables_count = variables_count;
    if (option_simplify_clauses) {
        problem->clauses = simplify_clauses(clauses);
    } else {
        problem->clauses = clauses;
    }
    problem->assigned_variables = 0;
    problem->current_variable = (unsigned int)-1;
    problem->consistent = 1;
    problem->branch_points = 0;
    problem->satisfied_clauses = 0;
    problem->randomized = 0;

    problem->empty_clause = new_tv_clause(new_int_list(), new_int_list());

    problem->assignment_history = (unsigned int *)malloc(sizeof(unsigned int) * variables_count);

    memset(problem->assignment_history, 0, sizeof(unsigned int) * variables_count);

    problem->satisfied = (unsigned int *)malloc(sizeof(unsigned int) * clauses->sz);
    problem->labelled = (unsigned int *)malloc(sizeof(unsigned int) * clauses->sz);

    memset(problem->satisfied, 0, sizeof(unsigned int) * clauses->sz);
    memset(problem->labelled, 0, sizeof(unsigned int) * clauses->sz);

    problem->unit_variables_pos = queue_new(NULL, NULL);
    problem->unit_variables_neg = queue_new(NULL, NULL);

    problem->unit_variables_pos_map = (unsigned char *)malloc(sizeof(unsigned char) * variables_count);
    problem->unit_variables_neg_map = (unsigned char *)malloc(sizeof(unsigned char) * variables_count);

    memset(problem->unit_variables_pos_map, 0, sizeof(unsigned char) * variables_count);
    memset(problem->unit_variables_neg_map, 0, sizeof(unsigned char) * variables_count);

    problem->variables = (dpll_variable *)malloc(sizeof(dpll_variable) * variables_count);
    for (ix = 0; ix < variables_count; ix++) {
        problem->variables[ix] = new_dpll_variable();
    }

    for (ix = 0; ix < problem->clauses->sz; ix++) {
        initialize_clause(problem, ix);

        pos = clauses->arr[ix]->pos;
        neg = clauses->arr[ix]->neg;
        if (pos->sz == 0 && neg->sz == 0) {
/* Problem is UNSAT due to an empty clause. */
            problem->consistent = 0;
        }
        if (pos->sz == 1 && neg->sz == 0) {
            int var = pos->arr[0];
            if (problem->variables[var]->pos_filter_clause != NULL) {
/* Problem is UNSAT due to arc inconsistency. */
                problem->consistent = 0;
            }
            if (problem->variables[var]->neg_filter_clause == NULL) {
                problem->variables[var]->neg_filter_clause = clauses->arr[ix];

                queue_push(problem->unit_variables_pos, i2p(var));
                problem->unit_variables_pos_map[var] = 1;
            }
        }
        if (neg->sz == 1 && pos->sz == 0) {
            int var = neg->arr[0];
            if (problem->variables[var]->neg_filter_clause != NULL) {
/* Problem is UNSAT due to arc inconsistency. */
                problem->consistent = 0;
            }
            if (problem->variables[var]->pos_filter_clause == NULL) {
                problem->variables[var]->pos_filter_clause = clauses->arr[ix];

                queue_push(problem->unit_variables_neg, i2p(var));
                problem->unit_variables_neg_map[var] = 1;
            }
        }
    }

    return problem;
}

/**
 * Destroys a DPLL problem.
 *
 * @param problem the DPLL problem to destroy.
 */
void dpll_free_problem(dpll_problem problem)
{
    register unsigned int ix;

    rfre_tv_clause(problem->empty_clause);

    queue_free(problem->unit_variables_pos);
    queue_free(problem->unit_variables_neg);

    free(problem->unit_variables_pos_map);
    free(problem->unit_variables_neg_map);

    free(problem->assignment_history);

    free(problem->satisfied);
    free(problem->labelled);

    for (ix = 0; ix < problem->variables_count; ix++) {
        if (NULL != problem->variables[ix]->delete_clause) {
            rfre_tv_clause(problem->variables[ix]->delete_clause);
        }
        free_dpll_variable(problem->variables[ix]);
    }
    free(problem->variables);
    free(problem);
}
