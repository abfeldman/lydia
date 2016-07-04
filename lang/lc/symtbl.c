#include "rewrite.h"
#include "config.h"
#include "symtbl.h"
#include "search.h"
#include "array.h"
#include "error.h"
#include "list.h"
#include "attr.h"
#include "lc.h"

#include <assert.h>

static lydia_symbol report = lydia_symbolNIL;
static lydia_symbol_list undefined = lydia_symbol_listNIL;

static int check_type(int reference, type t, user_type_entry_list user_type_table, lydia_symbol where, int function)
{
    unsigned int pos;

    if (typeNIL == t) {
	return 0;
    }
    if (t->tag != TAGuser_type) {
	return 1;
    }
    if (reference) {
	if (search_user_type_entry_list(user_type_table, to_user_type(t)->name->sym, &pos) && (user_type_table->arr[pos]->tag == TAGsystem_user_type_entry || user_type_table->arr[pos]->tag == TAGfunction_user_type_entry)) {
	    return 1;
	}
    } else {
	if (search_user_type_entry_list(user_type_table, to_user_type(t)->name->sym, &pos) && !(user_type_table->arr[pos]->tag == TAGsystem_user_type_entry || user_type_table->arr[pos]->tag == TAGfunction_user_type_entry)) {
	    return 1;
	}
    }
    if (report != where) {
	report = where;
	rfre_lydia_symbol_list(undefined);
	undefined = new_lydia_symbol_list();
    }
    if (member_lydia_symbol_list(undefined, to_user_type(t)->name->sym)) {
	return 0;
    }
    undefined = append_lydia_symbol_list(undefined, to_user_type(t)->name->sym);
    leh_error(ERR_UNDEFINED_IDENTIFIER,
	      function,
	      to_user_type(t)->name->org,
	      where->name,
	      to_user_type(t)->name->sym->name);

    return 0;
}

static int check_attribute(type t, attribute_entry_list attribute_table, lydia_symbol where)
{
    if (typeNIL == t) {
	return 0;
    }
    if (t->tag != TAGuser_type) {
	return 1;
    }
    if (is_attribute(attribute_table, to_user_type(t)->name->sym)) {
	return 1;
    }
    if (report != where) {
	report = where;
	rfre_lydia_symbol_list(undefined);
	undefined = new_lydia_symbol_list();
    }
    if (member_lydia_symbol_list(undefined, to_user_type(t)->name->sym)) {
	return 0;
    }
    undefined = append_lydia_symbol_list(undefined, to_user_type(t)->name->sym);
    leh_error(ERR_UNDEFINED_IDENTIFIER,
	      LEH_LOCATION_SYSTEM,
	      to_user_type(t)->name->org,
	      where->name,
	      to_user_type(t)->name->sym->name);

    return 0;
}

static int build_predicate_symbol_table(predicate p, formal_list formals, local_list locals, reference_list references, attribute_list attributes, lydia_symbol system, user_type_entry_list user_type_table, attribute_entry_list attribute_table)
{
    int result = 1;

    unsigned int pos;
    register unsigned int ix, iy;

    type t, q;
    variable_identifier id;
    int_list ref_count;

    assert(local_listNIL != locals);
    assert(formal_listNIL != formals);
    assert(reference_listNIL != references);

    switch (p->tag) {
	case TAGsimple_predicate:
/* Noop. */
	    break;
	case TAGforall_predicate:
	    result &= build_predicate_symbol_table(to_predicate(to_forall_predicate(p)->body), formals, locals, references, attributes, system, user_type_table, attribute_table);
	    break;
	case TAGexists_predicate:
	    result &= build_predicate_symbol_table(to_predicate(to_exists_predicate(p)->body), formals, locals, references, attributes, system, user_type_table, attribute_table);
	    break;
	case TAGcompound_predicate:
	    for (ix = 0; ix < to_compound_predicate(p)->predicates->sz; ix++) {
		result &= build_predicate_symbol_table(to_compound_predicate(p)->predicates->arr[ix], formals, locals, references, attributes, system, user_type_table, attribute_table);
	    }
	    break;
	case TAGif_predicate:
	    if (to_if_predicate(p)->thenval != compound_predicateNIL) {
		result &= build_predicate_symbol_table(to_predicate(to_if_predicate(p)->thenval), formals, locals, references, attributes, system, user_type_table, attribute_table);
	    }
	    if (to_if_predicate(p)->elseval != compound_predicateNIL) {
		result &= build_predicate_symbol_table(to_predicate(to_if_predicate(p)->elseval), formals, locals, references, attributes, system, user_type_table, attribute_table);
	    }
	    break;
	case TAGswitch_predicate:
	    for (ix = 0; ix < to_switch_predicate(p)->choices->sz; ix++) {
		result &= build_predicate_symbol_table(to_predicate(to_switch_predicate(p)->choices->arr[ix]->predicate), formals, locals, references, attributes, system, user_type_table, attribute_table);
	    }
	    if (to_switch_predicate(p)->deflt != compound_predicateNIL) {
		result &= build_predicate_symbol_table(to_predicate(to_switch_predicate(p)->deflt), formals, locals, references, attributes, system, user_type_table, attribute_table);
	    }
	    break;
	case TAGvariable_declaration:
	    if (!check_type(0, to_variable_declaration(p)->type, user_type_table, system, 0)) {
/*
 * This is a variable which is of undefined user type.
 * In this case we reported that here and then we continue suppressing all the
 * other errors and warning related to that symbol.
 */
		to_variable_declaration(p)->type = typeNIL;
		result = 0;
		break;
	    }
	    for (ix = 0; ix < to_variable_declaration(p)->instances->sz; ix++) {
		int redefined = 0;
		if (reverse_search_local_list(locals, to_variable_declaration(p)->instances->arr[ix]->name->sym, &pos)) {
		    leh_error(ERR_REDEFINED_IDENTIFIER,
			      LEH_LOCATION_SYSTEM,
			      to_variable_declaration(p)->instances->arr[ix]->name->org,
			      system->name,
			      to_variable_declaration(p)->instances->arr[ix]->name->sym->name,
			      locals->arr[pos]->name->name->org,
			      to_variable_declaration(p)->instances->arr[ix]->name->sym->name);
		    result = 0;
		    redefined = 1;
		} else if (reverse_search_formal_list(formals, to_variable_declaration(p)->instances->arr[ix]->name->sym, &pos)) {
		    leh_error(ERR_REDEFINED_IDENTIFIER,
			      LEH_LOCATION_SYSTEM,
			      to_variable_declaration(p)->instances->arr[ix]->name->org,
			      system->name,
			      to_variable_declaration(p)->instances->arr[ix]->name->sym->name,
			      formals->arr[pos]->name->name->org,
			      to_variable_declaration(p)->instances->arr[ix]->name->sym->name);
		    result = 0;
		    redefined = 1;
		}
		id = new_variable_identifier(rdup_orig_symbol(to_variable_declaration(p)->instances->arr[ix]->name),
					     rdup_extent_list(to_variable_declaration(p)->instances->arr[ix]->ranges),
					     variable_qualifier_listNIL);
		q = to_variable_declaration(p)->type;
		ref_count = int_listNIL;
		if (!redefined) {
		    ref_count = init_reference_count(to_variable_declaration(p)->instances->arr[ix]->ranges,
						     q,
						     user_type_table);
		}
		append_local_list(locals,
				  new_local(id,
					    rdup_type(q),
					    ref_count));
		if (to_variable_declaration(p)->instances->arr[ix]->val != exprNIL) {
		    increase_reference_count_array(locals->arr[locals->sz - 1]->name->ranges,
						   locals->arr[locals->sz - 1]->name->ranges,
						   locals->arr[locals->sz - 1]->ref_count,
						   0,
						   index_entry_listNIL,
						   user_type_table);
		}
	    }
	    break;
	case TAGattribute_declaration:
	    for (ix = 0; ix < to_attribute_declaration(p)->instances->sz; ix++) {
		type attr_type = to_attribute_declaration(p)->instances->arr[ix]->type;
		assert(attr_type->tag == TAGuser_type);
		if (!check_attribute(attr_type, attribute_table, system)) {
		    to_attribute_declaration(p)->instances->arr[ix]->type = typeNIL;
		    result = 0;
		    continue;
		}
		for (iy = 0; iy < to_attribute_declaration(p)->instances->arr[ix]->variables->sz; iy++) {
		    expr_variable var = to_attribute_declaration(p)->instances->arr[ix]->variables->arr[iy];
		    if (reverse_search_attribute_list(attributes,
						      var->name,
						      to_user_type(attr_type)->name->sym,
						      &pos)) {
			leh_error(ERR_REDEFINED_ATTRIBUTE,
				  LEH_LOCATION_SYSTEM,
				  var->name->name->org,
				  system->name,
				  to_user_type(attr_type)->name->sym->name,
				  var->name->name->sym->name,
				  attributes->arr[pos]->var->name->name->org,
				  to_user_type(attr_type)->name->sym->name,
				  var->name->name->sym->name);
			result = 0;
			continue;
		    }

		    append_attribute_list(attributes,
					  new_attribute(rdup_type(attr_type),
							rdup_expr_variable(var),
							rdup_orig_symbol(to_attribute_declaration(p)->instances->arr[ix]->alias),
							rdup_expr(to_attribute_declaration(p)->instances->arr[ix]->val)));
		}
	    }
	    break;
	case TAGsystem_declaration:
	    t = to_type(new_user_type(rdup_orig_symbol(to_system_declaration(p)->name)));
	    if (!check_type(1, t, user_type_table, system, 0)) {
		t = typeNIL;
		result = 0;
	    }
	    for (ix = 0; ix < to_system_declaration(p)->instances->sz; ix++) {
		orig_symbol name = to_system_declaration(p)->instances->arr[ix]->name;
		extent_list ranges = to_system_declaration(p)->instances->arr[ix]->ranges;
		int_list ref_count;

		int redefined = 0;
		if (reverse_search_reference_list(references, name->sym, &pos)) {
		    leh_error(ERR_REDEFINED_IDENTIFIER,
			      LEH_LOCATION_SYSTEM,
			      name->org,
			      system->name,
			      name->sym->name,
			      references->arr[pos]->name->org,
			      name->sym->name);
		    result = 0;
		    redefined = 1;
		}
		ref_count = int_listNIL;
		if (!redefined) {
		    ref_count = init_reference_count(ranges,
						     t,
						     user_type_table);
		}
		append_reference_list(references,
				      new_reference(rdup_orig_symbol(name),
						    rdup_extent_list(ranges),
						    rdup_type(t),
						    ref_count));
		if (to_system_declaration(p)->instances->arr[ix]->arguments != expr_listNIL) {
		    increase_reference_count_array(references->arr[references->sz - 1]->ranges,
						   references->arr[references->sz - 1]->ranges,
						   references->arr[references->sz - 1]->ref_count,
						   0,
						   index_entry_listNIL,
						   user_type_table);
		}
	    }
	    rfre_type(t);
	    break;
    }

    return result;
}

static int build_system_symbol_table(system_definition sys, user_type_entry_list user_type_table, attribute_entry_list attribute_table)
{
    int result = 1;

    lydia_symbol system;

    unsigned int pos;
    unsigned int ix;

    system = sys->name->sym;

    assert(formal_listNIL != sys->formals);

    for (ix = 0; ix < sys->formals->sz; ix++) {
	if (reverse_search_beginning_formal_list(sys->formals,
						 sys->formals->arr[ix]->name,
						 ix,
						 &pos)) {
	    leh_error(ERR_REDEFINED_IDENTIFIER,
		      LEH_LOCATION_SYSTEM,
		      sys->formals->arr[ix]->name->name->org,
		      system->name,
		      sys->formals->arr[ix]->name->name->sym->name,
		      sys->formals->arr[pos]->name->name->org,
		      sys->formals->arr[ix]->name->name->sym->name);
	    result = 0;
/* Do not count referneces on a redeclared varaible. */
	    rfre_int_list(sys->formals->arr[pos]->ref_count);
	    sys->formals->arr[pos]->ref_count = int_listNIL;
	    continue;
	}
	if (typeNIL == sys->formals->arr[ix]->type) {
	    continue;
	}
	sys->formals->arr[ix]->ref_count = init_reference_count(sys->formals->arr[ix]->name->ranges,
								sys->formals->arr[ix]->type,
								user_type_table);
    }

    result &= build_predicate_symbol_table(to_predicate(sys->predicates), sys->formals, sys->locals, sys->references, sys->attributes, system, user_type_table, attribute_table);

    for (ix = 0; ix < sys->formals->sz; ix++) {
	if (typeNIL != sys->formals->arr[ix]->type) {
/* Can bin NIL as we will later take the type of the formal which is on the left. */
	    result &= check_type(0, sys->formals->arr[ix]->type, user_type_table, system, 0);
	}
    }
    for (ix = 0; ix < sys->locals->sz; ix++) {
	result &= check_type(0, sys->locals->arr[ix]->type, user_type_table, system, 0);
    }
    for (ix = 0; ix < sys->references->sz; ix++) {
	result &= check_type(1, sys->references->arr[ix]->type, user_type_table, system, 0);
    }

    return result;
}

static int build_function_symbol_table(function_definition func, user_type_entry_list user_type_table)
{
/*
 * Functions have only formal parameters, no locally declared variables and
 * system references.
 */
    int result = 1;

    lydia_symbol function = func->name->sym;

    unsigned int pos;
    unsigned int ix;

    assert(formal_listNIL != func->formals);
    for (ix = 0; ix < func->formals->sz; ix++) {
	if (reverse_search_beginning_formal_list(func->formals, func->formals->arr[ix]->name, ix, &pos)) {
	    leh_error(ERR_REDEFINED_IDENTIFIER,
		      LEH_LOCATION_FUNCTION,
		      func->formals->arr[ix]->name->name->org,
		      function->name,
		      func->formals->arr[ix]->name->name->sym->name,
		      func->formals->arr[pos]->name->name->org,
		      func->formals->arr[ix]->name->name->sym->name);
	    result = 0;
/* Do not count referneces on a redeclared varaible. */
	    rfre_int_list(func->formals->arr[pos]->ref_count);
	    func->formals->arr[pos]->ref_count = int_listNIL;
	    continue;
	}
	func->formals->arr[ix]->ref_count = init_reference_count(func->formals->arr[ix]->name->ranges,
								 func->formals->arr[ix]->type,
								 user_type_table);
    }

    for (ix = 0; ix < func->formals->sz; ix++) {
	if (typeNIL != func->formals->arr[ix]->type) {
	    result &= check_type(0, func->formals->arr[ix]->type, user_type_table, function, 1);
	}
    }

    return result;
}

int build_symbol_tables(model m, user_type_entry_list user_type_table, attribute_entry_list attribute_table)
{
    register unsigned int ix;

    int result = 1;

    if (m == modelNIL) {
	return 1;
    }

    assert(definition_listNIL != m->defs);

    for (ix = 0; ix < m->defs->sz; ix++) {
	if (m->defs->arr[ix]->tag == TAGsystem_definition) {
	    result &= build_system_symbol_table(to_system_definition(m->defs->arr[ix]), user_type_table, attribute_table);
	}
	if (m->defs->arr[ix]->tag == TAGfunction_definition) {
	    result &= build_function_symbol_table(to_function_definition(m->defs->arr[ix]), user_type_table);
	}
    }
    if (lydia_symbol_listNIL != undefined) {
	rfre_lydia_symbol_list(undefined);
    }

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
