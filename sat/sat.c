#include "minisat.h"
#include "lsss.h"
#include "bcp.h"
#include "sat.h"
#include "tv.h"

#include <assert.h>

int is_consistent(tv_clause_list clauses,
                  unsigned int variables,
                  const unsigned char solver)
{
    switch (solver) {
        case SAT_SOLVER_BCP:
            return bcp_is_consistent(clauses, variables);
        case SAT_SOLVER_LSSS:
            return lsss_is_consistent(clauses, variables);
        case SAT_SOLVER_MINISAT:
            return minisat_is_consistent(clauses, variables);
        default:
            assert(0);
            abort();
    }
    assert(0);
    abort();
}

int get_solution(tv_clause_list clauses,
                 unsigned int variables,
                 tv_term *solution,
                 const unsigned char solver)
{
    switch (solver) {
        case SAT_SOLVER_BCP:
            return bcp_get_solution(clauses, variables, solution);
        case SAT_SOLVER_LSSS:
            return lsss_get_solution(clauses, variables, solution);
        case SAT_SOLVER_MINISAT:
            return minisat_get_solution(clauses, variables, solution);
        default:
            assert(0);
            abort();
    }
    assert(0);
    abort();
}

int get_random_solution(tv_clause_list clauses,
                        unsigned int variables,
                        tv_term *solution,
                        const unsigned char solver)
{
    switch (solver) {
        case SAT_SOLVER_BCP:
            break;
        case SAT_SOLVER_LSSS:
            return lsss_get_random_solution(clauses, variables, solution);
        case SAT_SOLVER_MINISAT:
            break;
        default:
            assert(0);
            abort();
    }
    assert(0);
    abort();
}

int get_all_solutions(tv_clause_list clauses,
                      unsigned int variables,
                      tv_term_list solutions,
                      const unsigned char solver)
{
    switch (solver) {
        case SAT_SOLVER_BCP:
            break;
        case SAT_SOLVER_LSSS:
            return lsss_get_all_solutions(clauses, variables, solutions);
        case SAT_SOLVER_MINISAT:
            break;
        default:
            assert(0);
            abort();
    }
    assert(0);
    abort();
}
