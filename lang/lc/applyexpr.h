#ifndef __APPLYEXPR_H__
#define __APPLYEXPR_H__

extern int infer_types_expr_apply(expr_apply apply, const_origin org, const_user_type_entry_list user_type_table, quantifier_entry_list quantifier_table, formal_list formals, local_list locals, reference_list references, lydia_symbol location, int fg_attribute, int fg_function);
extern int check_arguments(const_orig_symbol referrer_name, const_expr_list referrer_args, const_formal_list referee_args, const int fg_dest_function, const_lydia_symbol location, const int fg_function, const_user_type_entry_list user_type_table, const_quantifier_entry_list quantifier_table);

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
