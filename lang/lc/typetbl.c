#include "config.h"

#include <assert.h>

#include "typetbl.h"
#include "search.h"
#include "error.h"
#include "list.h"
#include "lc.h"

static lydia_symbol_list cyclic_types = lydia_symbol_listNIL;

static int check_type(type type, user_type_entry_list user_type_table)
{
    if (type->tag != TAGuser_type) {
	return 1;
    }
    if (!member_user_type_entry_list(user_type_table, to_user_type(type)->name->sym)) {
	leh_error(ERR_UNDEFINED_TYPE,
		  LEH_LOCATION_GLOBAL,
		  to_user_type(type)->name->org,
		  to_user_type(type)->name->sym->name);
	return 0;
    }
    return 1;
}

static int check_alias_redefinition(orig_symbol name, user_type_entry_list user_type_table)
{
    unsigned int pos;

    if (reverse_search_user_type_entry_list(user_type_table, name->sym, &pos)) {
	leh_error(ERR_REDEFINED_IDENTIFIER,
		  LEH_LOCATION_GLOBAL,
		  name->org,
		  name->sym->name,
		  user_type_table->arr[pos]->name->org,
		  name->sym->name);
	return 0;
    }

    return 1;
}

static int check_user_type_redefinition(orig_symbol name, user_type_entry_list user_type_table)
{
    unsigned int pos;

    if (reverse_search_user_type_entry_list(user_type_table, name->sym, &pos)) {
	leh_error(ERR_REDEFINED_IDENTIFIER,
		  LEH_LOCATION_GLOBAL,
		  name->org,
		  name->sym->name,
		  user_type_table->arr[pos]->name->org,
		  name->sym->name);
	return 0;
    }

    return 1;
}

static int add_enum_type(enum_definition def, user_type_entry_list user_type_table)
{
    enum_user_type_entry e = new_enum_user_type_entry(rdup_orig_symbol(def->name), new_orig_symbol_list());

    unsigned int ix, pos;
    for (ix = 0; ix < def->members->sz; ix++) {
	if (search_orig_symbol_list(e->entries, def->members->arr[ix]->sym, &pos)) {
	    leh_error(WARN_REDEFINED_ENUM_VALUE,
		      LEH_LOCATION_GLOBAL,
		      def->members->arr[ix]->org,
		      def->members->arr[ix]->sym->name,
		      e->entries->arr[pos]->org,
		      def->members->arr[ix]->sym->name);
	}
	e->entries = append_orig_symbol_list(e->entries, rdup_orig_symbol(def->members->arr[ix]));
    }
    append_user_type_entry_list(user_type_table, to_user_type_entry(e));

    return 1;
}

static int add_struct_type(struct_definition def, user_type_entry_list user_type_table)
{
    int result = 1;

    struct_user_type_entry s = new_struct_user_type_entry(rdup_orig_symbol(def->name),
							  new_type_list(),
							  new_orig_symbol_list(),
							  new_extent_list_list());

    unsigned int ix, pos;
    for (ix = 0; ix < def->members->sz; ix++) {
	if (search_orig_symbol_list(s->entries, def->members->arr[ix]->name->sym, &pos)) {
	    leh_error(ERR_REDEFINED_STRUCT_VALUE,
		      LEH_LOCATION_GLOBAL,
		      def->members->arr[ix]->name->org,
		      def->members->arr[ix]->name->sym->name,
		      s->entries->arr[pos]->org,
		      def->members->arr[ix]->name->sym->name);
	    result = 0;
	    continue;
	}
	append_orig_symbol_list(s->entries, rdup_orig_symbol(def->members->arr[ix]->name));
	append_type_list(s->types, rdup_type(def->members->arr[ix]->type));
	append_extent_list_list(s->ranges, rdup_extent_list(def->members->arr[ix]->ranges));
    }

    append_user_type_entry_list(user_type_table, to_user_type_entry(s));

    return result;
}

static int check_struct_members_types(struct_definition def,
				      user_type_entry_list user_type_table)
{
    int result = 1;

    register unsigned int ix;
    for (ix = 0; ix < def->members->sz; ix++) {
	if (!check_type(def->members->arr[ix]->type, user_type_table)) {
	    result = 0;
	    continue;
	}
    }

    return result;
}

int build_user_type_table(model m, user_type_entry_list user_type_table)
{
    system_user_type_entry sys;
    unsigned int ix, iy, pos;
    int result = 1;

    cyclic_types = new_lydia_symbol_list();

    for (ix = 0; ix < m->defs->sz; ix++) {
	switch (m->defs->arr[ix]->tag) {
	    case TAGattribute_definition:
/* Noop. */
		break;
	    case TAGenum_definition:
		result &= check_user_type_redefinition(to_enum_definition(m->defs->arr[ix])->name, user_type_table);
		result &= add_enum_type(to_enum_definition(m->defs->arr[ix]), user_type_table);
		break;
	    case TAGstruct_definition:
		result &= check_user_type_redefinition(to_struct_definition(m->defs->arr[ix])->name, user_type_table);
		result &= add_struct_type(to_struct_definition(m->defs->arr[ix]), user_type_table);
		break;
	    case TAGsystem_definition:
		result &= check_user_type_redefinition(to_system_definition(m->defs->arr[ix])->name, user_type_table);
		sys = new_system_user_type_entry(rdup_orig_symbol(to_system_definition(m->defs->arr[ix])->name),
						 to_system_definition(m->defs->arr[ix])->formals, 
						 to_system_definition(m->defs->arr[ix])->references,
						 to_system_definition(m->defs->arr[ix])->type);
		append_user_type_entry_list(user_type_table, to_user_type_entry(sys));
		break;
	    case TAGfunction_definition:
		result &= check_user_type_redefinition(to_function_definition(m->defs->arr[ix])->name, user_type_table);
		append_user_type_entry_list(user_type_table, to_user_type_entry(new_function_user_type_entry(rdup_orig_symbol(to_function_definition(m->defs->arr[ix])->name), rdup_type(to_function_definition(m->defs->arr[ix])->type), to_function_definition(m->defs->arr[ix])->val, to_function_definition(m->defs->arr[ix])->formals)));
		break;
	    case TAGtype_definition:
		result &= check_alias_redefinition(to_type_definition(m->defs->arr[ix])->name, user_type_table);
		append_user_type_entry_list(user_type_table, to_user_type_entry(new_alias_user_type_entry(rdup_orig_symbol(to_type_definition(m->defs->arr[ix])->name), rdup_type(to_type_definition(m->defs->arr[ix])->type))));
		break;
	    case TAGconstant_definition:
		result &= check_user_type_redefinition(to_constant_definition(m->defs->arr[ix])->name, user_type_table);
		append_user_type_entry_list(user_type_table, to_user_type_entry(new_constant_user_type_entry(rdup_orig_symbol(to_constant_definition(m->defs->arr[ix])->name), rdup_type(to_constant_definition(m->defs->arr[ix])->type), to_constant_definition(m->defs->arr[ix])->val)));
		break;
	}
    }

/* Now check for circular alias definitions. */
    for (ix = 0; ix < user_type_table->sz; ix++) {
	if (user_type_table->arr[ix]->tag == TAGfunction_user_type_entry) {
	    result &= check_type(to_function_user_type_entry(user_type_table->arr[ix])->type, user_type_table);
	}
	if (user_type_table->arr[ix]->tag == TAGconstant_user_type_entry) {
	    result &= check_type(to_constant_user_type_entry(user_type_table->arr[ix])->type, user_type_table);
	}
	if (user_type_table->arr[ix]->tag == TAGalias_user_type_entry) {
	    user_type_entry alias = user_type_table->arr[ix];
	    orig_symbol previous = alias->name;
	    orig_symbol_list chain;

	    result &= check_type(to_alias_user_type_entry(alias)->type, user_type_table);

	    chain = new_orig_symbol_list();
	    while (alias->tag == TAGalias_user_type_entry && to_alias_user_type_entry(alias)->type->tag == TAGuser_type) {
		orig_symbol name = to_user_type(to_alias_user_type_entry(alias)->type)->name;
		if (member_orig_symbol_list(chain, name->sym)) {
		    if (!member_lydia_symbol_list(cyclic_types, previous->sym)) {
			leh_error(ERR_TYPE_CIRCULAR,
				  LEH_LOCATION_GLOBAL,
				  previous->org,
				  previous->sym->name);
			for (iy = 0; iy < chain->sz; iy++) {
			    leh_error(ERR_TYPE_CIRCULAR_REPORT,
				      LEH_LOCATION_GLOBAL,
				      chain->arr[iy]->org,
				      previous->sym->name,
				      iy == chain->sz - 1 ? "again " : "",
				      chain->arr[iy]->sym->name);
			    cyclic_types = append_lydia_symbol_list(cyclic_types, chain->arr[iy]->sym);
			    previous = chain->arr[iy];
			}
		    }
		    result = 0;
		    break;
		}
		chain = append_orig_symbol_list(chain, name);
		if (!search_user_type_entry_list(user_type_table, name->sym, &pos)) {
		    break;
		}
		alias = user_type_table->arr[pos];
	    }
	    fre_orig_symbol_list(chain); /* References only in this list. */
	}
    }

    for (ix = 0; ix < m->defs->sz; ix++) {
	if (TAGstruct_definition == m->defs->arr[ix]->tag) {
	    result &= check_struct_members_types(to_struct_definition(m->defs->arr[ix]), user_type_table);
	}
    }

    return result;
}

/* Dereference all the aliases so they point only to built-in types. */
void dereference_aliases(user_type_entry_list user_type_table)
{
    orig_symbol name;
    alias_user_type_entry alias;
    unsigned int pos, ix, fg;
    for (ix = 0; ix < user_type_table->sz; ix++) {
	if (TAGalias_user_type_entry != user_type_table->arr[ix]->tag ||
	    member_lydia_symbol_list(cyclic_types, user_type_table->arr[ix]->name->sym)) {
	    continue;
	}
	name = user_type_table->arr[ix]->name;
	alias = to_alias_user_type_entry(user_type_table->arr[ix]);
	fg = 1;
	while (TAGuser_type == alias->type->tag) {
	    if (!search_user_type_entry_list(user_type_table, to_user_type(alias->type)->name->sym, &pos)) {
		fg = 0;
		break;
	    }
	    if (TAGalias_user_type_entry != user_type_table->arr[pos]->tag) {
		break;
	    }
	    alias = to_alias_user_type_entry(user_type_table->arr[pos]);
	}
	if (fg && user_type_table->arr[ix] != to_user_type_entry(alias)) {
	    user_type_table->arr[ix] = to_user_type_entry(rdup_alias_user_type_entry(alias));
	    rfre_orig_symbol(user_type_table->arr[ix]->name);
	    user_type_table->arr[ix]->name = rdup_orig_symbol(name);
	}
    }

    rfre_lydia_symbol_list(cyclic_types);
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
