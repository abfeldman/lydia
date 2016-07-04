#include <string.h>
#include <assert.h>
#ifdef WIN32
# include <Windows.h>
#endif

#include "defs.h"
#include "bcp_context.h"
#include "sorted_int_list.h"

#ifndef WIN32
# define INT_PTR int
#endif

static tv_clause_list simplify_clauses(tv_clause_list clauses)
{
    register unsigned int ix, iy, iz;

    for (ix = 0; ix < clauses->sz; ix++) {
        int_list pos = sort_int_list(clauses->arr[ix]->pos);
        int_list neg = sort_int_list(clauses->arr[ix]->neg);
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

static bcp_variable new_bcp_variable()
{
    bcp_variable var = (bcp_variable)malloc(sizeof(struct str_bcp_variable));
    if (NULL == var) {
        return var;
    }
    var->assignment = ASSIGNMENT_UNKNOWN;
    var->pos_clauses = new_int_list();
    var->neg_clauses = new_int_list();

    var->pos_filter_clause = NULL;
    var->neg_filter_clause = NULL;

    return var;
}

static void free_bcp_variable(bcp_variable var)
{
    rfre_int_list(var->pos_clauses);
    rfre_int_list(var->neg_clauses);
    free(var);
}

static void initialize_clause(bcp_problem problem, unsigned int clause)
{
    register unsigned int ix;

    const_int_list pos = problem->clauses->arr[clause]->pos;
    const_int_list neg = problem->clauses->arr[clause]->neg;
    for (ix = 0; ix < pos->sz; ix++) {
        append_int_list(problem->variables[pos->arr[ix]]->pos_clauses, clause);
    }
    for (ix = 0; ix < neg->sz; ix++) {
        append_int_list(problem->variables[neg->arr[ix]]->neg_clauses, clause);
    }
}

void add_clause(bcp_problem problem, tv_clause clause)
{
    append_tv_clause_list(problem->clauses, clause);

    problem->labelled = realloc(problem->labelled, sizeof(unsigned int) * problem->clauses->sz);
    problem->satisfied = realloc(problem->satisfied, sizeof(unsigned int) * problem->clauses->sz);

    problem->satisfied[problem->clauses->sz - 1] = 0;
    problem->labelled[problem->clauses->sz - 1] = clause->pos->sz + clause->neg->sz;

    initialize_clause(problem, problem->clauses->sz - 1);
}

/**
 * Creates a new BCP problem for use by the BCP consistency checker
 * or the CNF to DNF converter. This function is used internally by
 * the solvers.
 *
 * @param clauses the input set of clauses;
 * @param variables the number of variables in the CNF.
 * @returns the newly created BCP problem.
 */
bcp_problem bcp_new_problem(tv_clause_list clauses, const unsigned int variables_count)
{
    register unsigned int ix;

    bcp_problem problem = (bcp_problem)malloc(sizeof(struct str_bcp_problem));
    if (NULL == problem) {
        return problem;
    }

    problem->variables_count = variables_count;
    problem->clauses = simplify_clauses(clauses);
    problem->consistent = 1;
    problem->satisfied_clauses = 0;

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

    problem->variables = (bcp_variable *)malloc(sizeof(bcp_variable) * variables_count);
    for (ix = 0; ix < variables_count; ix++) {
        problem->variables[ix] = new_bcp_variable();
    }

    for (ix = 0; ix < problem->clauses->sz; ix++) {
        const_int_list pos, neg;

        initialize_clause(problem, ix);

        pos = clauses->arr[ix]->pos;
        neg = clauses->arr[ix]->neg;
        if (pos->sz == 1 && neg->sz == 0) {
            int var = pos->arr[0];
            if (problem->variables[var]->pos_filter_clause != NULL) {
/* Problem is UNSAT due to arc inconsistency. */
                problem->consistent = 0;
            }
            problem->variables[var]->neg_filter_clause = clauses->arr[ix];

            queue_push(problem->unit_variables_pos, i2p(var));
            problem->unit_variables_pos_map[var] = 1;
        }
        if (neg->sz == 1 && pos->sz == 0) {
            int var = neg->arr[0];
            if (problem->variables[var]->neg_filter_clause != NULL) {
/* Problem is UNSAT due to arc inconsistency. */
                problem->consistent = 0;
            }
            problem->variables[var]->pos_filter_clause = clauses->arr[ix];

            queue_push(problem->unit_variables_neg, i2p(var));
            problem->unit_variables_neg_map[var] = 1;
        }

        assert(pos->sz + neg->sz > 0);
    }

    return problem;
}

/**
 * Destroys a BCP problem.
 *
 * @param problem the BCP problem to destroy.
 */
void bcp_free_problem(bcp_problem problem)
{
    register unsigned int ix;

    queue_free(problem->unit_variables_pos);
    queue_free(problem->unit_variables_neg);

    free(problem->unit_variables_pos_map);
    free(problem->unit_variables_neg_map);

    free(problem->satisfied);
    free(problem->labelled);

    for (ix = 0; ix < problem->variables_count; ix++) {
        free_bcp_variable(problem->variables[ix]);
    }
    free(problem->variables);
    free(problem);
}
