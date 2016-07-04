#ifndef REWRITE_H
#define REWRITE_H

#include "ast.h"

extern void flatten_predicates(model, const_attribute_entry_list, index_entry_list, const_user_type_entry_list);
extern void rewrite_systems(model, user_type_entry_list);

#endif

/*
 * local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
