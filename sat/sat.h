#ifndef SAT_H
#define SAT_H

#include "tv.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SAT_SOLVER_BCP     0
#define SAT_SOLVER_LSSS    1
#define SAT_SOLVER_MINISAT 2

extern int is_consistent(tv_clause_list, unsigned int, const unsigned char);
extern int get_random_solution(tv_clause_list,
                               unsigned int,
                               tv_term *,
                               const unsigned char);
extern int get_solution(tv_clause_list,
                        unsigned int,
                        tv_term *,
                        const unsigned char);
extern int get_all_solutions(tv_clause_list,
                             unsigned int,
                             tv_term_list,
                             const unsigned char);

#ifdef __cplusplus
}
#endif

#endif
