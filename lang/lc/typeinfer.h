#ifndef __TYPEINFER_H__
#define __TYPEINFER_H__

#include "ast.h"

extern int compatible_types(type, type, const_user_type_entry_list);

extern int compute_array_sizes(model, user_type_entry_list);
extern int infer_types_model(model, const_user_type_entry_list, const_attribute_entry_list);
extern int infer_types_expr(expr, const_user_type_entry_list, quantifier_entry_list, const_origin, formal_list, local_list, reference_list, int, lydia_symbol, int);
extern unsigned int expr_size(const_expr, const_index_entry_list, const_user_type_entry_list);

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
