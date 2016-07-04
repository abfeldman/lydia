#ifndef SMOOTHY_H
#define SMOOTHY_H

#include "serializable.h"
#include "tv.h"

extern int smoothy(const_serializable, int, unsigned int, const int, const int, serializable *);
extern tv_clause_list multiply_tv_cnf(tv_clause_list, tv_clause_list, int_list_list, int_list_list, const unsigned int);
extern tv_term_list multiply_tv_dnf(tv_term_list, tv_term_list, variable_list, int_list_list, int_list_list, const unsigned int);
extern void mark_used(hierarchy, node, int_list);
extern void smoothy_init();
extern void smoothy_destroy();

#endif
