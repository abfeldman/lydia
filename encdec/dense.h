#ifndef DENSE_H
#define DENSE_H

#include "mv.h"
#include "tv.h"

extern tv_wff dense_encode_wff(const_mv_wff);
extern edge_list dense_encode_edges(const_edge_list,
                                    const_variable_list,
                                    const_values_set_list);

#endif
