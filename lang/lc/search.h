#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "ast.h"
#include "lcm.h"
#include "list.h"

extern int search_quantifier_entry_list(const_quantifier_entry_list haystack, lydia_symbol needle, unsigned int *pos);
extern int search_index_entry_list(const_index_entry_list haystack, lydia_symbol needle, unsigned int *pos);
extern int search_attribute_entry_list(const_attribute_entry_list user_type_table, lydia_symbol name, unsigned int *pos);
extern int member_attribute_entry_list(const_attribute_entry_list user_type_table, lydia_symbol name);
extern int search_user_type_entry_list(const_user_type_entry_list user_type_table, lydia_symbol name, unsigned int *pos);
extern int member_user_type_entry_list(const_user_type_entry_list user_type_table, lydia_symbol name);
extern int search_user_type_entry_list_with_tag(const_user_type_entry_list user_type_table, lydia_symbol name, unsigned int type, unsigned int *pos);
extern int member_user_type_entry_list_with_tag(const_user_type_entry_list user_type_table, lydia_symbol name, unsigned int type);
extern int search_orig_symbol_list(const_orig_symbol_list orig_symbols, lydia_symbol name, unsigned int *pos);
extern int member_orig_symbol_list(const_orig_symbol_list orig_symbols, lydia_symbol name);
extern int member_lydia_symbol_list(const_lydia_symbol_list symbols, lydia_symbol symbol);
extern int search_local_list(const_local_list locals, const_variable_identifier id, unsigned int *pos);
extern int search_formal_list(const_formal_list formals, const_variable_identifier id, unsigned int *pos);
extern int search_reference_list(const_reference_list references, lydia_symbol name, unsigned int *pos);
extern int member_reference_list(const_reference_list references, lydia_symbol name);
extern int search_system_definition_list(const_definition_list defs, lydia_symbol name, unsigned int *pos);
extern int member_system_definition_list(const_definition_list defs, lydia_symbol name);
extern int reverse_search_beginning_formal_list(const_formal_list formals, variable_identifier id, unsigned int from, unsigned int *pos);
extern int reverse_search_reference_list(const_reference_list references, lydia_symbol name, unsigned int *pos);
extern int reverse_search_local_list(const_local_list locals, lydia_symbol name, unsigned int *pos);
extern int reverse_search_formal_list(const_formal_list formals, lydia_symbol name, unsigned int *pos);
extern int reverse_search_user_type_entry_list_with_tag(user_type_entry_list user_type_table, lydia_symbol name, unsigned int type, unsigned int *pos);
extern int reverse_search_user_type_entry_list(user_type_entry_list user_type_table, lydia_symbol name, unsigned int *pos);
extern int reverse_search_attribute_entry_list(attribute_entry_list attribute_table, lydia_symbol name, unsigned int *pos);
extern int reverse_search_attribute_list(const_attribute_list attributes, variable_identifier id, lydia_symbol type, unsigned int *pos);

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
