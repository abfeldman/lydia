#include "lc.h"
#include "expr.h"
#include "iter.h"
#include "array.h"
#include "error.h"
#include "search.h"
#include "config.h"

#include <assert.h>
#include <math.h>

static void order_int_pair(int *a, int *b)
{
    if (*b < *a) {
	int x = *b;
	*b = *a;
	*a = x;
    }
}

/* Maps a multidimensional array element to a single dimensional offset. */
unsigned int array_offset(const_extent_list declaration_ranges,
			  const_int_list offset,
			  const_index_entry_list quantifier_indices,
			  const_user_type_entry_list user_type_table)
{
    register unsigned int ix, iy = 1;

    unsigned int result = 0;

    assert(declaration_ranges != extent_listNIL);
    assert(offset != int_listNIL);
    assert(declaration_ranges->sz == offset->sz);

    for (ix = declaration_ranges->sz - 1; ix < declaration_ranges->sz; ix--) {
	int p = eval_int_expr(declaration_ranges->arr[ix]->from, quantifier_indices, user_type_table);
	int s = eval_int_expr(declaration_ranges->arr[ix]->to, quantifier_indices, user_type_table);
	int q = abs(s - p) + 1;
	if (s > p) {
	    result += (offset->arr[ix] - p) * iy;
	} else {
	    result += (offset->arr[ix] - s) * iy;
	}
	iy *= q;
    }

    return result;
}

/* Computes the number of elements in an array. */
unsigned int lc_array_size(const_extent_list extents,
			   const_index_entry_list quantifier_indices,
			   const_user_type_entry_list user_type_table)
{
    register unsigned int result = 1;
    register unsigned int ix;

    assert(extents != extent_listNIL);

    for (ix = 0; ix < extents->sz; ix++) {
	int q = abs(eval_int_expr(extents->arr[ix]->to, quantifier_indices, user_type_table) -
		    eval_int_expr(extents->arr[ix]->from, quantifier_indices, user_type_table)) + 1;
	result *= q;
    }

    return result;
}

/**
 * Converts an array specifier to a list of array elements. For example,
 * if you have [1:2][3:4] as an input, the function will return four
 * integer lists: * (1, 3), (1, 4), (2, 3) and (2, 4).
 */
int_list_list array_offsets(const_extent_list elements,
			    const_index_entry_list quantifier_indices,
			    const_user_type_entry_list user_type_table)
{
    register unsigned int iv, ix, iy, iz;

    int_list_list result = new_int_list_list();

    if (extent_listNIL == elements) {
	append_int_list_list(result, append_int_list(new_int_list(), 0));
	return result;
    }

    iy = lc_array_size(elements, quantifier_indices, user_type_table);
    for (ix = 0; ix < iy; ix++) {
	int_list element = setroom_int_list(new_int_list(), elements->sz);
	element->sz = elements->sz;
	iv = ix;
	for (iz = elements->sz - 1; iz < elements->sz; iz--) {
	    int p = eval_int_expr(elements->arr[iz]->from, quantifier_indices, user_type_table);
	    int q = eval_int_expr(elements->arr[iz]->to, quantifier_indices, user_type_table);
	    int s = abs(q - p) + 1;
	    if (q > p) {
		element->arr[iz] = p + iv % s;
	    } else {
		element->arr[iz] = p - iv % s;
	    }
	    iv /= s;
	}
	append_int_list_list(result, element);
    }

    return result;
}

unsigned int variable_declaration_size(const_type variable_type,
				       const_index_entry_list quantifier_indices,
				       const_user_type_entry_list user_type_table)
{
    const_struct_user_type_entry structure;

    register unsigned int result = 0;
    register unsigned int ix;

    unsigned int pos;

    assert(typeNIL != variable_type);
    if (TAGuser_type != variable_type->tag ||
	!search_user_type_entry_list_with_tag(user_type_table,
					      to_user_type(variable_type)->name->sym,
					      TAGstruct_user_type_entry,
					      &pos)) {
	return 1;
    }
    structure = to_struct_user_type_entry(user_type_table->arr[pos]);

    assert(structure->entries->sz == structure->ranges->sz);
    assert(structure->entries->sz == structure->types->sz);
    for (ix = 0; ix < structure->entries->sz; ix++) {
	unsigned int array_length = 1, structure_length;
	if (structure->ranges->arr[ix] != extent_listNIL &&
	    structure->ranges->arr[ix]->sz > 0) {
	    array_length = lc_array_size(structure->ranges->arr[ix],
					 quantifier_indices,
					 user_type_table);
	}
	structure_length = variable_declaration_size(structure->types->arr[ix],
								  index_entry_listNIL,
								  user_type_table);
	result += array_length * structure_length;
    }
    return result;
}

int_list init_reference_count(const_extent_list declaration_ranges,
			      type declaration_type,
			      const_user_type_entry_list user_type_table)
{
    int_list result;
    int size;

    if (typeNIL == declaration_type) {
	return int_listNIL;
    }

    result = new_int_list();
    size = variable_declaration_size(declaration_type, index_entry_listNIL, user_type_table);
    if (extent_listNIL != declaration_ranges && declaration_ranges->sz > 0) {
/**
 * Create a list of integers for the reference counting. The number of
 * elements in the list is equal to the number of elements in the
 * array.
 */
	size *= lc_array_size(declaration_ranges, index_entry_listNIL, user_type_table);
    }

    result = setroom_int_list(result, size);
    result->sz = size;
    memset(result->arr, 0, sizeof(int) * size);

    return result;
}

/* To Do: This should go. */
void increase_reference_count_array(const_extent_list type_extents,
				    const_extent_list variable_extents,
				    int_list reference_count,
				    unsigned int start_offset,
				    index_entry_list quantifier_indices,
				    const_user_type_entry_list user_type_table)
{
    int_list_list offsets;

    register unsigned int ix;

    if (int_listNIL == reference_count) {
/**
 * Suppress errors and warnings. If this is, for example, a reference
 * to a redefined system, we do not count references, so we don't
 * report chained errors and warnings.
 */
	return;
    }

    if (extent_listNIL == type_extents || 0 == type_extents->sz) {
/* This is not an array but a single system declaration. */
	reference_count->arr[start_offset] += 1;
	return;
    }

    if (extent_listNIL == variable_extents) {
/**
 * The variable does not specify indices, hence we will use the
 * extents of the type.
 */
	variable_extents = type_extents;
    }

    offsets = array_offsets(variable_extents, quantifier_indices, user_type_table);
    for (ix = 0; ix < offsets->sz; ix++) {
	unsigned int iy = start_offset + array_offset(type_extents,
						      offsets->arr[ix],
						      quantifier_indices,
						      user_type_table);
	assert(iy < reference_count->sz);
	reference_count->arr[iy] += 1;
    }
    rfre_int_list_list(offsets);
}

static unsigned int struct_offset(type declaration_type,
				  orig_symbol member,
				  const_index_entry_list quantifier_indices,
				  const_user_type_entry_list user_type_table)
{
    struct_user_type_entry structure;

    register unsigned int result = 0;
    register unsigned int ix;
    unsigned int pos;

    assert(TAGuser_type == declaration_type->tag);
    search_user_type_entry_list(user_type_table,
				to_user_type(declaration_type)->name->sym,
				&pos);
    assert(user_type_table->arr[pos]->tag == TAGstruct_user_type_entry);
    structure = to_struct_user_type_entry(user_type_table->arr[pos]);

    assert(structure->entries->sz == structure->ranges->sz);
    assert(structure->entries->sz == structure->types->sz);
    if (!search_orig_symbol_list(structure->entries, member->sym, &pos)) {
	assert(0);
	abort();
    }
    for (ix = 0; ix < pos; ix++) {
	unsigned int iy = 1;
	unsigned int iz = 1;
	if (structure->ranges->arr[ix] != extent_listNIL && structure->ranges->arr[ix]->sz > 0) {
	    iy = lc_array_size(structure->ranges->arr[ix], quantifier_indices, user_type_table);
	}
	iz = variable_declaration_size(structure->types->arr[ix], index_entry_listNIL, user_type_table);
	result += iy * iz;
    }
    return result;
}

void increase_reference_count(const_orig_symbol_list var_name,
			      const_type_list declaration_type,
			      const_extent_list_list var_ranges,
			      const_extent_list_list declaration_ranges,
			      unsigned int current_range,
			      unsigned int offset,
			      int_list ref_count,
			      const_index_entry_list quantifier_indices,
			      const_user_type_entry_list user_type_table)
{
    register unsigned int iw, ix, iy, iz;

    int_list_list offsets;

    if (int_listNIL == ref_count) {
/* Supressing chained errors. */
	return;
    }

    iw = variable_declaration_size(declaration_type->arr[current_range],
				   quantifier_indices,
				   user_type_table);

    if (extent_listNIL == var_ranges->arr[current_range]) {
	if (current_range < var_ranges->sz - 1) {
	    iz = struct_offset(declaration_type->arr[current_range],
			       var_name->arr[current_range + 1],
			       quantifier_indices,
			       user_type_table);
	    increase_reference_count(var_name,
				     declaration_type,
				     var_ranges,
				     declaration_ranges,
				     current_range + 1,
				     offset + iz,
				     ref_count,
				     quantifier_indices,
				     user_type_table);
	    return;
	}
	for (iz = 0; iz < iw; iz++) {
	    ref_count->arr[offset + iz] += 1;
	}
	return;
    }

    offsets = array_offsets(var_ranges->arr[current_range],
			    quantifier_indices,
			    user_type_table);
    for (ix = 0; ix < offsets->sz; ix++) {
	iy = array_offset(declaration_ranges->arr[current_range],
			  offsets->arr[ix],
			  quantifier_indices,
			  user_type_table);
	if (current_range < var_ranges->sz - 1) {
	    iz = struct_offset(declaration_type->arr[current_range],
			       var_name->arr[current_range + 1],
			       quantifier_indices,
			       user_type_table);
	    increase_reference_count(var_name,
				     declaration_type,
				     var_ranges,
				     declaration_ranges,
				     current_range + 1,
				     offset + iy * iw + iz,
				     ref_count,
				     quantifier_indices,
				     user_type_table);
	    continue;
	}
	for (iz = 0; iz < iw; iz++) {
	    assert(offset + iy + iz < ref_count->sz);
	    ref_count->arr[offset + iy + iz] += 1;
	}
    }
    rfre_int_list_list(offsets);
}

/* This subroutine is used by the error handler in the type-checking phase. */
char *print_index(const_int_list index)
{
    register unsigned int ix, size = 1;
    char *result;

    for (ix = 0; ix < index->sz; ix++) {
	size += (unsigned int)log10(index->arr[ix] + 10) + 2;
    }

    result = malloc(sizeof(char) * size + 1);
    if (NULL == result) {
	return result;
    }
    result[0] = '\0';
    for (ix = 0; ix < index->sz; ix++) {
	sprintf(result + strlen(result), "[%d]", index->arr[ix]);
    }

    return result;
}

static int check_unused(const_type declaration_type,
			const_extent_list declaration_ranges,
			const_int_list ref_count,
			unsigned int offset,
			const_user_type_entry_list user_type_table,
			int *unused_type,
			char **unused_location)
{
    unsigned int pos;
    register unsigned int ix, iy;

    if (declaration_ranges != extent_listNIL &&
	declaration_ranges->sz > 0) {
	int_list_list offsets = array_offsets(declaration_ranges,
					      index_entry_listNIL,
					      user_type_table);

	unsigned int struct_length = variable_declaration_size(declaration_type,
							       index_entry_listNIL,
							       user_type_table);
	unsigned int array_length = offsets->sz;
	unsigned int used_elements = 0;
	int fg_first = 1;
	for (ix = 0; ix < offsets->sz; ix++) {
	    iy = check_unused(declaration_type,
			      extent_listNIL,
			      ref_count,
			      offset + ix * struct_length,
			      user_type_table,
			      unused_type,
			      unused_location);
	    if (iy == DECLARATION_USED_PARTIALLY) {
		char *index = print_index(offsets->arr[ix]);
		char *buf = (char *)malloc(strlen(index) + strlen(*unused_location) + 1);
		if (NULL == buf) {
		    assert(0);
		    abort();
		}
		sprintf(buf, "%s%s", index, *unused_location);
		free(index);
		free(*unused_location);
		*unused_location = buf;
		return DECLARATION_USED_PARTIALLY;
	    }
	    if (iy == DECLARATION_USED) {
		used_elements += 1;
	    }
	    if (iy == DECLARATION_UNUSED && fg_first) {
		fg_first = 0;
		if (NULL != *unused_location) {
		    free(*unused_location);
		}
		*unused_type = UNUSED_ARRAY_ELEMENT;
		*unused_location = print_index(offsets->arr[ix]);
	    }
	}
	rfre_int_list_list(offsets);

	if (used_elements == 0) {
	    return DECLARATION_UNUSED;
	}
	if (used_elements == array_length) {
	    return DECLARATION_USED;
	}
	return DECLARATION_USED_PARTIALLY;
    }

    if (TAGuser_type == declaration_type->tag &&
	search_user_type_entry_list_with_tag(user_type_table,
					     to_user_type(declaration_type)->name->sym,
					     TAGstruct_user_type_entry,
					     &pos)) {
	const_struct_user_type_entry structure = to_const_struct_user_type_entry(user_type_table->arr[pos]);
	unsigned int used_elements = 0;
	unsigned int structure_offset = 0;
	int fg_first = 1;
	for (ix = 0; ix < structure->entries->sz; ix++) {
	    unsigned int element_length = variable_declaration_size(structure->types->arr[ix],
								    index_entry_listNIL,
								    user_type_table);
	    if (structure->ranges->arr[ix] != extent_listNIL &&
		structure->ranges->arr[ix]->sz > 0) {
		element_length *= lc_array_size(structure->ranges->arr[ix],
						index_entry_listNIL,
						user_type_table);
	    }
	    iy = check_unused(structure->types->arr[ix],
			      structure->ranges->arr[ix],
			      ref_count,
			      offset + structure_offset,
			      user_type_table,
			      unused_type,
			      unused_location);
	    structure_offset += element_length;
	    if (iy == DECLARATION_USED_PARTIALLY) {
		char *buf = (char *)malloc(strlen(structure->entries->arr[ix]->sym->name) + strlen(*unused_location) + 2);
		if (NULL == buf) {
		    assert(0);
		    abort();
		}
		sprintf(buf, ".%s%s", structure->entries->arr[ix]->sym->name, *unused_location);
		free(*unused_location);
		*unused_location = buf;
		return DECLARATION_USED_PARTIALLY;
	    }
	    if (iy == DECLARATION_USED) {
		used_elements += 1;
	    }
	    if (iy == DECLARATION_UNUSED && fg_first) {
		fg_first = 0;
		if (*unused_location == NULL) {
		    free(*unused_location);
		}
		*unused_type = UNUSED_STRUCTURE_MEMBER;
		*unused_location = (char *)malloc(strlen(structure->entries->arr[ix]->sym->name) + 2);
		if (NULL == *unused_location) {
		    assert(0);
		    abort();
		}
		sprintf(*unused_location, ".%s", structure->entries->arr[ix]->sym->name);
	    }
	}
	if (used_elements == 0) {
	    return DECLARATION_UNUSED;
	}
	if (used_elements == structure->entries->sz) {
	    return DECLARATION_USED;
	}
	return DECLARATION_USED_PARTIALLY;
    }
    return ref_count->arr[offset] > 0 ? DECLARATION_USED : DECLARATION_UNUSED;
}

void warn_unused_reference(const_orig_symbol name,
			   const_extent_list extents,
			   const_int_list ref_count,
			   const_lydia_symbol location,
			   const_user_type_entry_list user_type_table)
{
    int_list position = int_listNIL;
    int_list_list offsets;

    register unsigned int ix, iy = 0;

    if (int_listNIL == ref_count) {
	return; /* Suppress errors and warnings. */
    }
    if (extent_listNIL == extents || 0 == extents->sz) {
/* This is not an array but a single system declaration. */
	if (ref_count->arr[0] == 0) {
	    leh_error(WARN_UNUSED_SYSTEM,
		      LEH_LOCATION_SYSTEM,
		      name->org,
		      location->name,
		      name->sym->name);
	}
	return;
    }

    position = int_listNIL;
    offsets = array_offsets(extents, index_entry_listNIL, user_type_table);
    for (ix = 0; ix < offsets->sz; ix++) {
	if (0 == ref_count->arr[array_offset(extents, offsets->arr[ix], index_entry_listNIL, user_type_table)]) {
	    if (int_listNIL == position) {
		position = offsets->arr[ix];
	    }
	    iy += 1;
	}
    }
    if (iy == ref_count->sz) {
	leh_error(WARN_UNUSED_SYSTEM_ARRAY_ALL,
		  LEH_LOCATION_SYSTEM,
		  name->org,
		  location->name,
		  name->sym->name);
	rfre_int_list_list(offsets);
	return;
    }
    if (iy > 0) {
	char *buffer = print_index(position);
	leh_error(WARN_UNUSED_SYSTEM_ARRAY,
		  LEH_LOCATION_SYSTEM,
		  name->org,
		  location->name,
		  name->sym->name,
		  buffer);
	free(buffer);
    }

    rfre_int_list_list(offsets);
}

void warn_unused(const_variable_identifier declaration_name,
		 const_type declaration_type,
		 const_int_list declaration_ref_count,
		 const int fg_function,
		 const_lydia_symbol location,
		 const_user_type_entry_list user_type_table)
{
    unsigned int pos;
    char *unused_location = NULL;
    int unused_type = 0, result, is_structure;

    if (int_listNIL == declaration_ref_count) {
/* Suppress warnings on declarations for which there are error messages. */
	return;
    }

    assert(variable_qualifier_listNIL == declaration_name->qualifiers);

    result = check_unused(declaration_type,
			  declaration_name->ranges,
			  declaration_ref_count,
			  0,
			  user_type_table,
			  &unused_type,
			  &unused_location);
    if (result == DECLARATION_USED) {
	return;
    }
    if (result == DECLARATION_UNUSED) {
	if (declaration_name->ranges != extent_listNIL &&
	    declaration_name->ranges->sz > 0) {
	    leh_error(WARN_UNUSED_VARIABLE_ARRAY,
		      fg_function,
		      declaration_name->name->org,
		      location->name,
		      declaration_name->name->sym->name);
	    return;
	}
	is_structure = TAGuser_type == declaration_type->tag &&
	    search_user_type_entry_list_with_tag(user_type_table,
						 to_user_type(declaration_type)->name->sym,
						 TAGstruct_user_type_entry,
						 &pos);
	leh_error(is_structure ? WARN_UNUSED_VARIABLE_STRUCTURE : WARN_UNUSED_VARIABLE,
		  fg_function,
		  declaration_name->name->org,
		  location->name,
		  declaration_name->name->sym->name);
	return;
    }
    assert(unused_type != 0);
    assert(unused_location != NULL);
    if (unused_type == UNUSED_ARRAY_ELEMENT) {
	leh_error(WARN_UNUSED_VARIABLE_ARRAY_ELEMENT,
		  fg_function,
		  declaration_name->name->org,
		  location->name,
		  declaration_name->name->sym->name,
		  unused_location);
	free(unused_location);
	return;
    }
    assert(unused_type == UNUSED_STRUCTURE_MEMBER);
    leh_error(WARN_UNUSED_VARIABLE_STRUCTURE_MEMBER,
	      fg_function,
	      declaration_name->name->org,
	      location->name,
	      declaration_name->name->sym->name,
	      unused_location);
    free(unused_location);
}

int check_array_dimensions(orig_symbol referrer_name,
			   const_extent_list referrer_extents,
			   const_origin referee_org,
			   const_extent_list referee_extents,
			   int fg_declaration,
			   int fg_function,
			   const_lydia_symbol location)
{
    unsigned int referee_dims = referee_extents == extent_listNIL ? 0 : referee_extents->sz;
    unsigned int referrer_dims = referrer_extents == extent_listNIL ? 0 : referrer_extents->sz;

    if (!fg_declaration && referrer_dims == 1 && referee_dims == 0) {
/* To Do: Are all the sizes checked here? */
	return 1;
    }
    if (referrer_dims > 0 && referee_dims == 0) {
	leh_error(ERR_TYPE_SUBSCRIPT,
		  fg_function,
		  referrer_name->org,
		  location->name);
	return 0;
    }
    if (referee_dims != referrer_dims) {
	leh_error(fg_declaration ? ERR_TYPE_ARRAY_DIMS_DECLARATION : ERR_TYPE_ARRAY_DIMS,
		  fg_function,
		  referrer_name->org,
		  location->name,
		  referrer_name->sym->name,
/* To Do: Check if this is the best origin for the error message. */
		  referee_org,
		  referee_dims,
		  referee_dims == 1 ? "" : "s",
		  referrer_dims,
		  referrer_dims == 1 ? "" : "s");
	return 0;
    }

    return 1;
}

int check_array_bounds(const_origin referrer_org,
		       const_extent_list referrer_extents,
		       const_extent_list referee_extents,
		       int fg_function,
		       const_lydia_symbol location,
		       const_quantifier_entry_list quantifier_table,
		       const_user_type_entry_list user_type_table)
{
    register unsigned int iy;

    unsigned int referee_dims = referee_extents == extent_listNIL ? 0 : referee_extents->sz;
    for (iy = 0; iy < referee_dims; iy++) {
	index_entry_list indices = index_entry_listNIL;
	if (quantifier_table->sz > 0) {
	    indices = init_quantifier_table(quantifier_table, user_type_table);
	}
	do {
	    int referee_from = eval_int_expr(referee_extents->arr[iy]->from, indices, user_type_table);
	    int referee_to = eval_int_expr(referee_extents->arr[iy]->to, indices, user_type_table);
	    int referrer_from = eval_int_expr(referrer_extents->arr[iy]->from, indices, user_type_table);
	    int referrer_to = eval_int_expr(referrer_extents->arr[iy]->to, indices, user_type_table);
	    order_int_pair(&referee_from, &referee_to);
	    order_int_pair(&referrer_from, &referrer_to);
	    if ((referrer_from < referee_from) || (referrer_to > referee_to)) {
/* To Do: Show the variable name. */
		leh_error(ERR_TYPE_OUT_OF_RANGE,
			  fg_function,
			  referrer_org,
			  location->name);
		return 0;
	    }
	} while (quantifier_table->sz > 0 &&
		 index_entry_listNIL != (indices = advance_quantifier_table(indices, quantifier_table, user_type_table)));
    }

    return 1;
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
