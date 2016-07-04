#include "minisat.h"

extern int minisat_is_consistent_(tv_clause_list, unsigned int);
extern int minisat_get_solution_(tv_clause_list, unsigned int, tv_term *);

int minisat_is_consistent(tv_clause_list clauses, unsigned int variables)
{
    return minisat_is_consistent_(clauses, variables);
}

int minisat_get_solution(tv_clause_list clauses,
                         unsigned int variables,
                         tv_term *solution)
{
    return minisat_get_solution_(clauses, variables, solution);
}
