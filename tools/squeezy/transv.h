#ifndef TRANSV_H
#define TRANSV_H

#include "list.h"
#include "tv.h"

extern int_list_list new_adjacency_matrix(tv_literal_set_list, tv_literal_set_list);
extern void era(int_list_list, tv_literal_set_list);
extern tv_literal_set_list greedy_era(int_list_list, tv_literal_set_list);

#endif
