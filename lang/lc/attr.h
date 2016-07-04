#ifndef __ATTR_H__
#define __ATTR_H__

#include "variable.h"
#include "ast.h"

extern int build_attribute_table(model m, user_type_entry_list user_type_table, attribute_entry_list attribute_table);
extern int is_attribute(const_attribute_entry_list attribute_table, lydia_symbol name);
extern void check_internal_attribute(variable_attribute target, attribute entry, lydia_symbol system, const_identifier variable);
extern int cross_check_attributes(lydia_symbol system, variable_list variables, formal_list formals, local_list locals);

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
