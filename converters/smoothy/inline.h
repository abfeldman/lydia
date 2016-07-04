#ifndef INLINE_H
#define INLINE_H

#include "variable.h"
#include "hierarchy.h"

extern int inline_variables(const_hierarchy,
                            const_node,
                            mapping_list,
                            values_set_list,
                            variable_list,
                            variable_list,
                            constant_list,
                            int_list_list,
                            int_list_list,
                            qualifier_list);
extern int combine_parent_child_variables(node, const_node, const_edge, int_list_list, int_list_list);

#endif
