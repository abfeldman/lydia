#include "lc.h"
#include "list.h"
#include "search.h"
#include "config.h"

#include <assert.h>

int search_quantifier_entry_list(const_quantifier_entry_list haystack, lydia_symbol needle, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < haystack->sz; ix++) {
	if (needle == haystack->arr[ix]->name) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int search_index_entry_list(const_index_entry_list haystack, lydia_symbol needle, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < haystack->sz; ix++) {
	if (needle == haystack->arr[ix]->name) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int search_attribute_entry_list(const_attribute_entry_list attribute_table, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < attribute_table->sz; ix++) {
	if (name == attribute_table->arr[ix]->name->sym) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int member_attribute_entry_list(const_attribute_entry_list attribute_table, lydia_symbol name)
{
    unsigned int ix;
    for (ix = 0; ix < attribute_table->sz; ix++) {
	if (name == attribute_table->arr[ix]->name->sym) {
	    return 1;
	}
    }
    return 0;
}

int search_user_type_entry_list(const_user_type_entry_list user_type_table, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < user_type_table->sz; ix++) {
	if (name == user_type_table->arr[ix]->name->sym) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int member_user_type_entry_list(const_user_type_entry_list user_type_table, lydia_symbol name)
{
    unsigned int ix;
    for (ix = 0; ix < user_type_table->sz; ix++) {
	if (name == user_type_table->arr[ix]->name->sym) {
	    return 1;
	}
    }
    return 0;
}

int search_user_type_entry_list_with_tag(const_user_type_entry_list user_type_table, lydia_symbol name, unsigned int type, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < user_type_table->sz; ix++) {
	if (user_type_table->arr[ix]->tag == type && name == user_type_table->arr[ix]->name->sym) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int member_user_type_entry_list_with_tag(const_user_type_entry_list user_type_table, lydia_symbol name, unsigned int type)
{
    unsigned int ix;
    for (ix = 0; ix < user_type_table->sz; ix++) {
	if (user_type_table->arr[ix]->tag == type && name == user_type_table->arr[ix]->name->sym) {
	    return 1;
	}
    }
    return 0;
}

int search_orig_symbol_list(const_orig_symbol_list orig_symbols, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < orig_symbols->sz; ix++) {
	if (orig_symbols->arr[ix]->sym == name) {
            *pos = ix;
	    return 1;
        }
    }
    return 0;
}

int member_orig_symbol_list(const_orig_symbol_list orig_symbols, lydia_symbol name)
{
    unsigned int ix;
    for (ix = 0; ix < orig_symbols->sz; ix++) {
	if (orig_symbols->arr[ix]->sym == name) {
	    return 1;
        }
    }
    return 0;
}

int member_lydia_symbol_list(const_lydia_symbol_list symbols, lydia_symbol symbol)
{
    unsigned int ix;
    for (ix = 0; ix < symbols->sz; ix++) {
	if (symbols->arr[ix] == symbol) {
            return 1;
        }
    }
    return 0;
}

int search_formal_list(const_formal_list formals,
		       const_variable_identifier id,
		       unsigned int *pos)
{
    register unsigned int ix;

    if (formal_listNIL == formals) {
	return 0;
    }

    for (ix = 0; ix < formals->sz; ix++) {
	if (id->qualifiers == variable_qualifier_listNIL || id->qualifiers->sz == 0) {
	    if (formals->arr[ix]->name->name->sym == id->name->sym) {
		*pos = ix;
		return 1;
	    }
	} else {
	    if (formals->arr[ix]->name->name->sym == id->qualifiers->arr[0]->name->sym) {
		*pos = ix;
		return 1;
	    }
	}
    }
    return 0;
}

int search_local_list(const_local_list locals,
		      const_variable_identifier id,
		      unsigned int *pos)
{
    register unsigned int ix;

    if (local_listNIL == locals) {
	return 0;
    }

    for (ix = 0; ix < locals->sz; ix++) {
	if (id->qualifiers == variable_qualifier_listNIL || id->qualifiers->sz == 0) {
	    if (locals->arr[ix]->name->name->sym == id->name->sym) {
		*pos = ix;
		return 1;
	    }
	} else {
	    if (locals->arr[ix]->name->name->sym == id->qualifiers->arr[0]->name->sym) {
		*pos = ix;
		return 1;
	    }
	}
    }
    return 0;
}

int search_reference_list(const_reference_list references, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < references->sz; ix++) {
	if (references->arr[ix]->name->sym == name) {
	    *pos = ix;
            return 1;
        }
    }
    return 0;
}

int member_reference_list(const_reference_list references, lydia_symbol name)
{
    unsigned int ix;
    for (ix = 0; ix < references->sz; ix++) {
	if (references->arr[ix]->name->sym == name) {
            return 1;
        }
    }
    return 0;
}

int search_system_definition_list(const_definition_list defs, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = 0; ix < defs->sz; ix++) {
	if (defs->arr[ix]->tag == TAGsystem_definition && to_const_system_definition(defs->arr[ix])->name->sym == name) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int member_system_definition_list(const_definition_list defs, lydia_symbol name)
{
    unsigned int ix;
    for (ix = 0; ix < defs->sz; ix++) {
	if (defs->arr[ix]->tag == TAGsystem_definition && to_const_system_definition(defs->arr[ix])->name->sym == name) {
	    return 1;
	}
    }
    return 0;
}

int reverse_search_reference_list(const_reference_list references, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = references->sz - 1; ix < references->sz; ix--) {
	if (references->arr[ix]->name->sym == name) {
	    *pos = ix;
            return 1;
        }
    }
    return 0;
}

int reverse_search_local_list(const_local_list locals, lydia_symbol name, unsigned int *pos)
{
    register unsigned int ix;

    for (ix = locals->sz - 1; ix < locals->sz; ix--) {
        if (locals->arr[ix]->name->name->sym == name) {
            *pos = ix;
            return 1;
        }
    }

    return 0;
}

int reverse_search_formal_list(const_formal_list formals, lydia_symbol name, unsigned int *pos)
{
    register unsigned int ix;

    for (ix = formals->sz - 1; ix < formals->sz; ix--) {
        if (formals->arr[ix]->name->name->sym == name) {
            *pos = ix;
            return 1;
        }
    }

    return 0;
}

int reverse_search_beginning_formal_list(const_formal_list formals, variable_identifier id, unsigned int from, unsigned int *pos)
{
    unsigned int ix;

    assert(from < formals->sz);
    for (ix = from - 1; ix < from; ix--) {
	if (id->qualifiers == variable_qualifier_listNIL || id->qualifiers->sz == 0) {
	    if (formals->arr[ix]->name->name->sym == id->name->sym) {
		*pos = ix;
		return 1;
	    }
	} else {
	    if (formals->arr[ix]->name->name->sym == id->qualifiers->arr[0]->name->sym) {
		*pos = ix;
		return 1;
	    }
	}
    }
    return 0;
}

int reverse_search_user_type_entry_list(user_type_entry_list user_type_table, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = user_type_table->sz - 1; ix < user_type_table->sz; ix--) {
	if (name == user_type_table->arr[ix]->name->sym) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int reverse_search_user_type_entry_list_with_tag(user_type_entry_list user_type_table, lydia_symbol name, unsigned int type, unsigned int *pos)
{
    unsigned int ix;
    for (ix = user_type_table->sz - 1; ix < user_type_table->sz; ix--) {
	if (user_type_table->arr[ix]->tag == type && name == user_type_table->arr[ix]->name->sym) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int reverse_search_attribute_entry_list(attribute_entry_list attribute_table, lydia_symbol name, unsigned int *pos)
{
    unsigned int ix;
    for (ix = attribute_table->sz - 1; ix < attribute_table->sz; ix--) {
	if (name == attribute_table->arr[ix]->name->sym) {
	    *pos = ix;
	    return 1;
	}
    }
    return 0;
}

int reverse_search_attribute_list(const_attribute_list attributes, variable_identifier id, lydia_symbol type, unsigned int *pos)
{
    unsigned int ix;
    for (ix = attributes->sz - 1; ix < attributes->sz; ix--) {
	assert(attributes->arr[ix]->type->tag == TAGuser_type);
	if (isequal_variable_identifier(attributes->arr[ix]->var->name, id) &&
	    to_user_type(attributes->arr[ix]->type)->name->sym == type) {
            *pos = ix;
            return 1;
        }
    }

    return 0;
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
