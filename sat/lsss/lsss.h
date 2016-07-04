#ifndef LSSS_H
#define LSSS_H

#include "tv.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int lsss_is_consistent(tv_clause_list, unsigned int);
extern int lsss_get_random_solution(tv_clause_list, unsigned int, tv_term *);
extern int lsss_get_solution(tv_clause_list, unsigned int, tv_term *);
extern int lsss_get_all_solutions(tv_clause_list, unsigned int, tv_term_list);

#ifdef __cplusplus
}
#endif

#endif
