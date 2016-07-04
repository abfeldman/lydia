#ifndef BCP_H
#define BCP_H

#include "tv.h"

extern int bcp_is_consistent(tv_clause_list, unsigned int);
extern int bcp_get_solution(tv_clause_list, unsigned int, tv_term *);

#define UNSAT   0
#define SAT     1
#define UNKNOWN 2

#endif
