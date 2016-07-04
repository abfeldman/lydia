#ifndef __VAREXPR_H__
#define __VAREXPR_H__

extern int infer_types_expr_variable(expr_variable var, const_origin org, const_user_type_entry_list user_type_table, quantifier_entry_list quantifier_table, formal_list formals, local_list locals, reference_list references, lydia_symbol location, int fg_attribute, int fg_function);

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
