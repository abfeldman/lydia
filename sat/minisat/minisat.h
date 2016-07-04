#ifndef MINISAT_H
#define MINISAT_H

#include "tv.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int minisat_is_consistent(tv_clause_list, unsigned int);
extern int minisat_get_solution(tv_clause_list, unsigned int, tv_term *);

#ifdef __cplusplus
}
#endif

#endif
