#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "ast.h"

#define DECLARATION_UNUSED             0
#define DECLARATION_USED               1
#define DECLARATION_USED_PARTIALLY     2

#define UNUSED_STRUCTURE_MEMBER        1
#define UNUSED_ARRAY_ELEMENT           2

extern int_list init_reference_count(const_extent_list,
                                     type,
                                     const_user_type_entry_list);
extern void increase_reference_count_array(const_extent_list,
                                           const_extent_list,
                                           int_list,
                                           unsigned int,
                                           index_entry_list,
                                           const_user_type_entry_list);
extern void increase_reference_count(const_orig_symbol_list,
                                     const_type_list,
                                     const_extent_list_list,
                                     const_extent_list_list,
                                     unsigned int, unsigned int,
                                     int_list,
                                     const_index_entry_list,
                                     const_user_type_entry_list);
extern char *print_index(const_int_list);

extern int_list_list array_offsets(const_extent_list,
                                   const_index_entry_list,
                                   const_user_type_entry_list);
extern unsigned int array_offset(const_extent_list,
                                 const_int_list,
                                 const_index_entry_list,
                                 const_user_type_entry_list);

extern void warn_unused(const_variable_identifier,
                        const_type,
                        const_int_list,
                        const int,
                        const_lydia_symbol,
                        const_user_type_entry_list);
extern void warn_unused_reference(const_orig_symbol,
                                  const_extent_list,
                                  const_int_list,
                                  const_lydia_symbol,
                                  const_user_type_entry_list);
extern int check_array_dimensions(orig_symbol,
                                  const_extent_list,
                                  const_origin,
                                  const_extent_list,
                                  int,
                                  int,
                                  const_lydia_symbol);
extern int check_array_bounds(const_origin,
                              const_extent_list,
                              const_extent_list,
                              int,
                              const_lydia_symbol,
                              const_quantifier_entry_list,
                              const_user_type_entry_list);

extern unsigned int lc_array_size(const_extent_list,
                                  const_index_entry_list,
                                  const_user_type_entry_list);
extern unsigned int variable_declaration_size(const_type,
                                              const_index_entry_list,
                                              const_user_type_entry_list);

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
