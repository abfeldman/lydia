#ifndef __ITER_H__
#define __ITER_H__

#include "ast.h"

extern index_entry_list init_quantifier_table(const_quantifier_entry_list quantifier_table, const_user_type_entry_list user_type_table);
extern index_entry_list advance_quantifier_table(index_entry_list indices, const_quantifier_entry_list quantifier_table, const_user_type_entry_list user_type_table);

extern int_list init_index(const_extent_list ranges, const_index_entry_list quantifier_indices, const_user_type_entry_list user_type_table);
extern int_list advance_index(int_list indices, const_extent_list ranges, const_index_entry_list quantifier_indices, const_user_type_entry_list user_type_table);

extern extent_list init_extent(const_extent_list ranges, const_index_entry_list quantifier_indices, const_user_type_entry_list user_type_table);
extern extent_list advance_extent(extent_list extent, const_extent_list ranges, const_index_entry_list quantifier_indices, const_user_type_entry_list user_type_table);

#endif
