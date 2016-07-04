#ifndef __TYPETBL_H__
#define __TYPETBL_H__

#include "ast.h"

extern int build_user_type_table(model m, user_type_entry_list user_type_table);
extern void dereference_aliases(user_type_entry_list user_type_table);

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
