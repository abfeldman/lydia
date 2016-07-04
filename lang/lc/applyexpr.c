#include "config.h"

#include <assert.h>

#include "typeinfer.h"
#include "varexpr.h"
#include "search.h"
#include "error.h"
#include "array.h"
#include "iter.h"
#include "expr.h"
#include "defs.h"
#include "lc.h"

extern lydia_symbol_list undefined;

static int check_redefinition(const_orig_symbol name,
			      const_extent_list instantiation_extents,
			      const_extent_list declaration_extents,
			      const_int_list ref_count,
			      const int fg_function,
			      const_lydia_symbol location,
			      const_index_entry_list quantifier_indices,
			      const_user_type_entry_list user_type_table)
{
    int_list_list offsets;

    register unsigned int ix;

    if (int_listNIL == ref_count) {
	return 1; /* Used for suppressing errors and warnings. */
    }

    if (extent_listNIL == declaration_extents) {
/* This is not an array but a single system declaration. */
	if (ref_count->arr[0] > 0) {
/* To Do: Show the location of the previous instantiation. */
	    leh_error(ERR_ALREADY_INSTANTIATED,
		      fg_function,
		      name->org,
		      location->name,
		      name->sym->name);
	    return 0;
	}
	return 1;
    }

    offsets = array_offsets(instantiation_extents,
			    quantifier_indices,
			    user_type_table);
    for (ix = 0; ix < offsets->sz; ix++) {
	if (ref_count->arr[array_offset(declaration_extents,
					offsets->arr[ix],
					quantifier_indices,
					user_type_table)] > 0) {
	    char *index = print_index(offsets->arr[ix]);
/* To Do: Show the location of the previous instantiation. */
	    leh_error(ERR_ALREADY_INSTANTIATED_ARRAY,
		      fg_function,
		      name->org,
		      location->name,
		      name->sym->name,
		      index,
		      location);
	    free(index);
	    rfre_int_list_list(offsets);
	    return 0;
	}
    }
    rfre_int_list_list(offsets);

    return 1;
}

static unsigned int formal_declaration_size(const_formal f, const_user_type_entry_list user_type_table)
{
    unsigned int size = variable_declaration_size(f->type,
						  index_entry_listNIL,
						  user_type_table);
    if (extent_listNIL != f->name->ranges && f->name->ranges->sz > 0) {
	size *= lc_array_size(f->name->ranges,
			      index_entry_listNIL,
			      user_type_table);
    }
    return size;
}

int check_arguments(const_orig_symbol referrer_name,
		    const_expr_list referrer_args,
		    const_formal_list referee_args,
		    const int fg_dest_function,
		    const_lydia_symbol location,
		    const int fg_function,
		    const_user_type_entry_list user_type_table,
		    const_quantifier_entry_list quantifier_table)
{
    register int result = 1;
    register unsigned int ix;

    index_entry_list indices;

    if (referrer_args->sz > referee_args->sz) {
	leh_error(fg_dest_function ? ERR_TOO_MANY_ARGUMENTS_FUNCTION : ERR_TOO_MANY_ARGUMENTS_SYSTEM,
		  fg_function,
		  referrer_name->org,
		  location->name,
		  referrer_name->sym->name);
	result = 0;
    }
    if (referrer_args->sz < referee_args->sz) {
	leh_error(fg_dest_function ? ERR_TOO_FEW_ARGUMENTS_FUNCTION : ERR_TOO_FEW_ARGUMENTS_SYSTEM,
		  fg_function,
		  referrer_name->org,
		  location->name,
		  referrer_name->sym->name);
	result = 0;
    }
    for (ix = 0; ix < min(referrer_args->sz, referee_args->sz); ix++) {
	if (!compatible_types(referrer_args->arr[ix]->type,
			      referee_args->arr[ix]->type,
			      user_type_table)) {
	    leh_error(fg_dest_function ? ERR_INCOMPATIBLE_ARGUMENT_TYPE_FUNCTION : ERR_INCOMPATIBLE_ARGUMENT_TYPE_SYSTEM,
		      fg_function,
		      referrer_name->org,
		      location->name,
		      ix + 1,
		      referrer_name->sym->name);
	    result = 0;
	    continue;
	}
	indices = index_entry_listNIL;
	if (quantifier_table->sz > 0) {
	    indices = init_quantifier_table(quantifier_table, user_type_table);
	}
	do {
	    assert(typeNIL != referee_args->arr[ix]->type);
	    if (typeNIL != referrer_args->arr[ix]->type &&
		expr_size(referrer_args->arr[ix], indices, user_type_table) !=
		formal_declaration_size(referee_args->arr[ix], user_type_table)) {
		leh_error(fg_dest_function ? ERR_BAD_ARGUMENT_SIZE_FUNCTION : ERR_BAD_ARGUMENT_SIZE_SYSTEM,
			  fg_function,
			  referrer_name->org,
			  location->name,
			  ix + 1,
			  referrer_name->sym->name);
		result = 0;
		continue;
	    }
	} while (quantifier_table->sz > 0 &&
		 index_entry_listNIL != (indices = advance_quantifier_table(indices, quantifier_table, user_type_table)));
    }
    return result;
}

int infer_types_expr_apply(expr_apply apply,
			   const_origin org,
			   const_user_type_entry_list user_type_table,
			   quantifier_entry_list quantifier_table,
			   formal_list formals,
			   local_list locals,
			   reference_list references,
			   lydia_symbol location,
			   int fg_attribute,
			   int fg_function)
{
    register unsigned int ix;

    orig_symbol_list var_name;
    type_list declaration_type;
    extent_list_list var_ranges;
    extent_list_list declaration_ranges;

    int result = 1;

    unsigned int pos;

    reference ref;

    system_user_type_entry sys;

/* Typecheck the all the arguments of the referee. */
    for (ix = 0; ix < apply->parms->sz; ix++) {
	result &= infer_types_expr(to_expr(apply->parms->arr[ix]),
				   user_type_table,
				   quantifier_table,
				   org,
				   formals,
				   locals,
				   references,
				   fg_attribute,
				   location,
				   fg_function);
    }

    if (search_user_type_entry_list_with_tag(user_type_table,
					     apply->name->sym,
					     TAGfunction_user_type_entry,
					     &pos)) {
/* We have a function call. */
	function_user_type_entry func = to_function_user_type_entry(user_type_table->arr[pos]);
	if (!check_arguments(apply->name,
			     apply->parms,
			     func->formals,
			     1,
			     location,
			     fg_function,
			     user_type_table,
			     quantifier_table)) {
	    return 0;
	}
	apply->type = to_type(new_user_type(rdup_orig_symbol(apply->name)));
	return result;
    }

    assert(reference_listNIL != references);

    if (!search_reference_list(references, apply->name->sym, &pos)) {
	if (!member_lydia_symbol_list(undefined, apply->name->sym)) {
	    leh_error(ERR_UNDEFINED_IDENTIFIER,
		      fg_function,
		      apply->name->org,
		      location->name,
		      apply->name->sym->name);
	    undefined = append_lydia_symbol_list(undefined, apply->name->sym);
	}
	return 0;
    }

    ref = references->arr[pos];

    if (ref->type == typeNIL) {
/*
 * Suppress the error message for undefined error variables when this
 * variable has an undefined type.
 */
	return result;
    }


    assert(ref->type->tag == TAGuser_type);
    if (!search_user_type_entry_list_with_tag(user_type_table,
					      to_user_type(ref->type)->name->sym,
					      TAGsystem_user_type_entry,
					      &pos)) {
	return result;
    }
/* This is a system reference. */
    sys = to_system_user_type_entry(user_type_table->arr[pos]);

    if (extent_listNIL != apply->extents && 0 == apply->extents->sz) {
	rfre_extent_list(apply->extents);
	apply->extents = extent_listNIL;
    }
    if (extent_listNIL != apply->extents) {
	for (ix = 0; ix < apply->extents->sz; ix++) {
	    extent range = apply->extents->arr[ix];
	    if (exprNIL != range->from) {
		result &= infer_types_expr(range->from,
					   user_type_table,
					   quantifier_table,
					   org,
					   formals,
					   locals,
					   references,
					   fg_attribute,
					   location,
					   fg_function);
	    }
	    if (exprNIL != range->to) {
		result &= infer_types_expr(range->to,
					   user_type_table,
					   quantifier_table,
					   org,
					   formals,
					   locals,
					   references,
					   fg_attribute,
					   location,
					   fg_function);
	    }
	}
    }

    if (extent_listNIL == apply->extents && extent_listNIL != ref->ranges) {
/*
 * We have an instantiation which does not specify extents, hence take
 * the whole range of the system declaration.
 */
	apply->extents = rdup_extent_list(ref->ranges);
    }

    if (!check_array_dimensions(apply->name,
				apply->extents,
				ref->name->org,
				ref->ranges,
				1,
				fg_function,
				location)) {
	rfre_int_list(ref->ref_count);
	ref->ref_count = int_listNIL;
	return 0;
    }
    if (!check_array_bounds(apply->name->org,
			    apply->extents,
			    ref->ranges,
			    fg_function,
			    location,
			    quantifier_table,
			    user_type_table)) {
	rfre_int_list(ref->ref_count);
	ref->ref_count = int_listNIL;
	return 0;
    }

    var_name = append_orig_symbol_list(new_orig_symbol_list(), apply->name);
    declaration_type = append_type_list(new_type_list(), ref->type);
    var_ranges = append_extent_list_list(new_extent_list_list(), apply->extents);
    declaration_ranges = append_extent_list_list(new_extent_list_list(), ref->ranges);
    if (quantifier_table->sz == 0) {
	if (!check_redefinition(apply->name,
				apply->extents,
				ref->ranges,
				ref->ref_count,
				fg_function,
				location,
				index_entry_listNIL,
				user_type_table)) {
	    rfre_int_list(ref->ref_count);
	    ref->ref_count = int_listNIL;
	    return 0;
	}
	increase_reference_count(var_name,
				 declaration_type,
				 var_ranges,
				 declaration_ranges,
				 0,
				 0,
				 ref->ref_count,
				 index_entry_listNIL,
				 user_type_table);
    } else {
	index_entry_list indices = init_quantifier_table(quantifier_table, user_type_table);
	do {
	    if (!check_redefinition(apply->name,
				    apply->extents,
				    ref->ranges,
				    ref->ref_count,
				    fg_function,
				    location,
				    indices,
				    user_type_table)) {
		rfre_int_list(ref->ref_count);
		ref->ref_count = int_listNIL;
		return 0;
	    }
	    increase_reference_count(var_name,
				     declaration_type,
				     var_ranges,
				     declaration_ranges,
				     0,
				     0,
				     ref->ref_count,
				     indices,
				     user_type_table);
	} while (index_entry_listNIL != (indices = advance_quantifier_table(indices, quantifier_table, user_type_table)));
    }
    fre_orig_symbol_list(var_name);
    fre_type_list(declaration_type);
    fre_extent_list_list(var_ranges);
    fre_extent_list_list(declaration_ranges);

    if (!check_arguments(apply->name,
			 apply->parms,
			 sys->formals,
			 0,
			 location,
			 fg_function,
			 user_type_table,
			 quantifier_table)) {
	return 0;
    }

    apply->type = rdup_type(ref->type);

    return result;
}

/*
 * local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
