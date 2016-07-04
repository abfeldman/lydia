#include "config.h"

#include <assert.h>

#include "typeinfer.h"
#include "varexpr.h"
#include "search.h"
#include "error.h"
#include "array.h"
#include "expr.h"
#include "iter.h"
#include "lc.h"

extern lydia_symbol_list undefined;

static int search_formal_and_local_lists(formal_list formals,
                                         local_list locals,
                                         variable_identifier variable_name,
                                         type *declaration_type,
                                         extent_list *declaration_ranges,
                                         int_list **declaration_ref_count,
                                         origin *declaration_org)
{
    unsigned int pos;

    if (search_local_list(locals, variable_name, &pos)) {
        *declaration_org = locals->arr[pos]->name->name->org;
        *declaration_type = locals->arr[pos]->type;
        *declaration_ref_count = &locals->arr[pos]->ref_count;
        *declaration_ranges = locals->arr[pos]->name->ranges;
        return 1;
    }
    if (search_formal_list(formals, variable_name, &pos)) {
        *declaration_org = formals->arr[pos]->name->name->org;
        *declaration_type = formals->arr[pos]->type;
        *declaration_ref_count = &formals->arr[pos]->ref_count;
        *declaration_ranges = formals->arr[pos]->name->ranges;
        return 1;
    }

    return 0;
}

static int infer_types_enum_constant(expr_variable var,
                                     orig_symbol_list var_name,
                                     extent_list_list var_ranges,
                                     enum_user_type_entry enum_type_entry,
                                     lydia_symbol location,
                                     int fg_function)
{
    if (var_name->sz == 1) {
/* There is no selector. */
        leh_error(ERR_TYPE_ENUM_NOSEL,
                  fg_function,
                  var_name->arr[0]->org,
                  location->name,
                  var_name->arr[0]->sym->name);
        return 0;
    }
    if (var_ranges->arr[0] != extent_listNIL) {
        leh_error(ERR_TYPE_NOT_ARRAY,
                  fg_function,
                  var_name->arr[0]->org,
                  location->name,
                  var_name->arr[0]->sym->name);
        return 0;
    }
    if (var_name->sz != 2 ||
        !member_orig_symbol_list(enum_type_entry->entries,
                                 var_name->arr[1]->sym)) {
        leh_error(ERR_TYPE_ENUM_BADSEL,
                  fg_function,
                  var_name->arr[0]->org,
                  location->name,
                  var_name->arr[0]->sym->name);
        return 0;
    }
    if (var_ranges->arr[1] != extent_listNIL) {
        leh_error(ERR_TYPE_NOT_ARRAY,
                  fg_function,
                  var_name->arr[1]->org,
                  location->name,
                  var_name->arr[1]->sym->name);
        return 0;
    }
    var->type = to_type(new_user_type(rdup_orig_symbol(var_name->arr[0])));

    return 1;
}

static int infer_types_quantifier_index(expr_variable var,
                                        orig_symbol_list var_name,
                                        extent_list_list var_ranges,
                                        lydia_symbol location,
                                        int fg_function)
{
    if (var_ranges->arr[0] != extent_listNIL) {
        leh_error(ERR_TYPE_NOT_ARRAY,
                  fg_function,
                  var_name->arr[0]->org,
                  location->name,
                  var_name->arr[0]->sym->name);
        return 0;
    }
    if (var_name->sz > 1) {
        leh_error(ERR_TYPE_NOT_STRUCT,
                  fg_function,
                  var_name->arr[0]->org,
                  location->name,
                  var_name->arr[0]->sym->name);
        return 0;
    }
    var->type = to_type(new_int_type());

    return 1;
}

int infer_types_expr_variable(expr_variable var,
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
    register unsigned int ix, iy;
    unsigned int posx, posy;

    orig_symbol_list var_name;
    extent_list_list var_ranges;

    type_list declaration_type;
    extent_list_list declaration_ranges;
    int_list *declaration_ref_count = NULL;
    origin declaration_org = originNIL;

    index_entry_list indices;

    int result = 1;

/* Compute the number of parts of which the variable name comprises. */
    unsigned int var_parts = 1;
    if (var->name->qualifiers != variable_qualifier_listNIL) {
        var_parts += var->name->qualifiers->sz;
    }

/* Get the variable name and qualifiers in a canonic order. */
    var_name = setroom_orig_symbol_list(new_orig_symbol_list(), var_parts);
    var_ranges = setroom_extent_list_list(new_extent_list_list(), var_parts);
    var_name->sz = var_parts;
    var_ranges->sz = var_parts;
    for (ix = 0; ix < var_parts - 1; ix++) {
        assert(orig_symbolNIL != var->name->qualifiers->arr[ix]->name);
        var_name->arr[ix] = var->name->qualifiers->arr[ix]->name;
        var_ranges->arr[ix] = var->name->qualifiers->arr[ix]->ranges;
    }
    assert(orig_symbolNIL != var->name->name);
    var_name->arr[var_parts - 1] = var->name->name;
    var_ranges->arr[var_parts - 1] = var->name->ranges;

/*
 * Make sure that there are no zero-sized range lists to eliminate
 * checks in the future.
 */
    for (ix = 0; ix < var_parts; ix++) {
        if (extent_listNIL != var_ranges->arr[ix] &&
            0 == var_ranges->arr[ix]->sz) {
            rfre_extent_list(var_ranges->arr[ix]);
            var_ranges->arr[ix] = extent_listNIL;
        }
    }

/* Now recursively check the expressions in all of the array indices. */
    for (ix = 0; ix < var_parts; ix++) {
        if (extent_listNIL != var_ranges->arr[ix]) {
            extent_list ranges = var_ranges->arr[ix];
            for (iy = 0; iy < ranges->sz; iy++) {
                extent range = ranges->arr[iy];
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
    }

/*
 * At this point we're pretty damn sure that we have a variable name
 * consisting of at least one part.
 */
    assert(var_parts > 0);
    assert(var_name->arr[0] != orig_symbolNIL);
    assert(var_name->arr[0]->sym != lydia_symbolNIL);

/* Now, let's see if this is not a quantifier index. */
    if (search_quantifier_entry_list(quantifier_table, var_name->arr[0]->sym, &posx)) {
        result = infer_types_quantifier_index(var,
                                              var_name,
                                              var_ranges,
                                              location,
                                              fg_function);
        goto final;
    }
/* Not a quantifier index, but maybe a constant. Let's see. */
    if (var_parts == 1 && search_user_type_entry_list_with_tag(user_type_table, var_name->arr[0]->sym, TAGconstant_user_type_entry, &posx)) {
/* Yep. Success. */
        var->type = to_type(new_user_type(rdup_orig_symbol(var_name->arr[0])));
        goto final;
    }
/* Check if this is an enumeration constant. */
    if (search_user_type_entry_list_with_tag(user_type_table, var_name->arr[0]->sym, TAGenum_user_type_entry, &posx)) {
        result = infer_types_enum_constant(var,
                                           var_name,
                                           var_ranges,
                                           to_enum_user_type_entry(user_type_table->arr[posx]),
                                           location,
                                           fg_function);
        goto final;
    }
/*
 * There are three remaining options. The variable is either a scalar
 * or an array or structure. If it is an array, of course, it can be
 * an array of structures, and in each structure we may have arrays as
 * well.
 */

    declaration_type = setroom_type_list(new_type_list(), var_parts);
    declaration_type->sz = var_parts;

    declaration_ranges = setroom_extent_list_list(new_extent_list_list(), var_parts);
    declaration_ranges->sz = var_parts;

    declaration_ref_count = NULL;
    declaration_org = originNIL;

/* Look for the declaration of our variable now. It must be there. */
    if (!search_formal_and_local_lists(formals,
                                       locals,
                                       var->name,
                                       &declaration_type->arr[0],
                                       &declaration_ranges->arr[0],
                                       &declaration_ref_count,
                                       &declaration_org)) {
        if (!member_lydia_symbol_list(undefined, var_name->arr[0]->sym)) {
            leh_error(ERR_UNDEFINED_IDENTIFIER,
                      fg_function,
                      var_name->arr[0]->org,
                      location->name,
                      var_name->arr[0]->sym->name);
            append_lydia_symbol_list(undefined, var_name->arr[0]->sym);
        }
        fre_extent_list_list(declaration_ranges);
        fre_type_list(declaration_type);
        result = 0;
        goto final;
    }

    assert(declaration_type->arr[0] != typeNIL);
    assert(declaration_ref_count != NULL);

/* Now determine the types of all the qualifiers. */
    for (ix = 1; ix < var_name->sz; ix++) {
        if (TAGuser_type != declaration_type->arr[ix - 1]->tag ||
            !search_user_type_entry_list_with_tag(user_type_table,
                                                  to_user_type(declaration_type->arr[ix - 1])->name->sym,
                                                  TAGstruct_user_type_entry,
                                                  &posx)) {
            leh_error(ERR_TYPE_NOT_STRUCT,
                      fg_function,
                      var_name->arr[ix - 1]->org,
                      location->name,
                      var_name->arr[ix - 1]->sym->name);
/*
 * Don't do reference counting for this type anymore as we don't want
 * to see chained errors.
 */
            rfre_int_list(*declaration_ref_count);
            *declaration_ref_count = int_listNIL;
            fre_extent_list_list(declaration_ranges);
            fre_type_list(declaration_type);
            result = 0;
            goto final;
        }
        if (!search_orig_symbol_list(to_struct_user_type_entry(user_type_table->arr[posx])->entries,
                                     var_name->arr[ix]->sym,
                                     &posy)) {
            leh_error(ERR_TYPE_STRUCT_NO_ENTRY,
                      fg_function,
                      var_name->arr[ix]->org,
                      location->name,
                      var_name->arr[ix]->sym->name,
                      to_struct_user_type_entry(user_type_table->arr[posx])->name->sym->name);
/* Ditto. */
            rfre_int_list(*declaration_ref_count);
            *declaration_ref_count = int_listNIL;
            fre_extent_list_list(declaration_ranges);
            fre_type_list(declaration_type);
            result = 0;
            goto final;
        }
        declaration_type->arr[ix] = to_struct_user_type_entry(user_type_table->arr[posx])->types->arr[posy];
        declaration_ranges->arr[ix] = to_struct_user_type_entry(user_type_table->arr[posx])->ranges->arr[posy];
    }

    for (ix = 0; ix < declaration_ranges->sz; ix++) {
        if (extent_listNIL != var_ranges->arr[ix]) {
            extent_list ranges = var_ranges->arr[ix];
            for (iy = 0; iy < ranges->sz; iy++) {
                extent range = ranges->arr[iy];
                if (range->to == exprNIL) {
/*
 * This means, take the end of declared array. You remember the
 * parsing, don't you?
 */
                    assert(declaration_ranges->arr[ix]->arr[iy]->to != exprNIL);
                    range->to = rdup_expr(declaration_ranges->arr[ix]->arr[iy]->to);
                }
            }
        }
    }

/* The story, however, is not over yet. */
    if (extent_listNIL == var->name->ranges &&
        extent_listNIL != declaration_ranges->arr[declaration_ranges->sz - 1]) {
/*
 * We have a variable which does not specify extents, hence take the
 * whole range of the variable declaration.
 */
        var->name->ranges = var_ranges->arr[var_parts - 1] = rdup_extent_list(declaration_ranges->arr[declaration_ranges->sz - 1]);
    }
/* Check all the indices. First the dimensions. */
    for (ix = 0; ix < var_ranges->sz; ix++) {
        if (!check_array_dimensions(var_name->arr[ix],
                                    var_ranges->arr[ix],
                                    declaration_org,
                                    declaration_ranges->arr[ix],
                                    1,
                                    fg_function,
                                    location)) {
            rfre_int_list(*declaration_ref_count);
            *declaration_ref_count = int_listNIL;
            fre_extent_list_list(declaration_ranges);
            fre_type_list(declaration_type);
            result = 0;
            goto final;
        }
        if (!check_array_bounds(var_name->arr[ix]->org,
                                var_ranges->arr[ix],
                                declaration_ranges->arr[ix],
                                fg_function,
                                location,
                                quantifier_table,
                                user_type_table)) {
            rfre_int_list(*declaration_ref_count);
            *declaration_ref_count = int_listNIL;
            fre_extent_list_list(declaration_ranges);
            fre_type_list(declaration_type);
            result = 0;
            goto final;
        }
    }

/* Finally, this is the type of our variable. */
    assert(typeNIL != declaration_type->arr[declaration_type->sz - 1]);
    var->type = rdup_type(declaration_type->arr[declaration_type->sz - 1]);

    if (quantifier_table->sz == 0) {
        increase_reference_count(var_name,
                                 declaration_type,
                                 var_ranges,
                                 declaration_ranges,
                                 0,
                                 0,
                                 *declaration_ref_count,
                                 index_entry_listNIL,
                                 user_type_table);

        fre_extent_list_list(declaration_ranges);
        fre_type_list(declaration_type);
        goto final;
    }
    indices = init_quantifier_table(quantifier_table, user_type_table);
    do {
        increase_reference_count(var_name,
                                 declaration_type,
                                 var_ranges,
                                 declaration_ranges,
                                 0,
                                 0,
                                 *declaration_ref_count,
                                 indices,
                                 user_type_table);
    } while (index_entry_listNIL != (indices = advance_quantifier_table(indices, quantifier_table, user_type_table)));

    fre_extent_list_list(declaration_ranges);
    fre_type_list(declaration_type);

final:
    fre_extent_list_list(var_ranges);
    fre_orig_symbol_list(var_name);

    return result;
}
