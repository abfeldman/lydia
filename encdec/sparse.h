#ifndef SPARSE_H
#define SPARSE_H

#include "mv.h"
#include "tv.h"

extern tv_wff sparse_encode_wff(const_mv_wff);
extern edge_list sparse_encode_edges(const_edge_list,
                                     const_variable_list,
                                     const_values_set_list);

#endif
