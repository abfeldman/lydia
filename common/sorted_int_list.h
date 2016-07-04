#ifndef SORTED_INT_LIST_H
#define SORTED_INT_LIST_H


#include "list.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int_list sort_int_list(int_list);
extern int member_sorted_int_list(const_int_list l, int v);
extern int_list merge_sorted_int_list(int_list, int_list);
extern int search_sorted_int_list(int_list, int, unsigned int *);
extern int is_sorted_term_consistent(int_list, int_list);

#ifdef __cplusplus
}
#endif

#endif
