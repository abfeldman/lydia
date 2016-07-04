#include "config.h"

#include <assert.h>
#include <math.h>

#include "search.h"
#include "error.h"
#include "list.h"
#include "attr.h"
#include "lc.h"

#define EPSILON 10e-7

static int check_type(type type, user_type_entry_list user_type_table)
{
    unsigned int pos;

    if (type->tag != TAGuser_type) {
        return 1;
    }
    if (!search_user_type_entry_list(user_type_table, to_user_type(type)->name->sym, &pos) ||
        user_type_table->arr[pos]->tag == TAGsystem_user_type_entry ||
        user_type_table->arr[pos]->tag == TAGfunction_user_type_entry ||
        user_type_table->arr[pos]->tag == TAGconstant_user_type_entry) {
        leh_error(ERR_UNDEFINED_TYPE,
                  LEH_LOCATION_GLOBAL,
                  to_user_type(type)->name->org,
                  to_user_type(type)->name->sym->name);
        return 0;
    }
    return 1;
}

int build_attribute_table(model m, user_type_entry_list user_type_table, attribute_entry_list attribute_table)
{
    unsigned int pos, ix;

    int result = 1;

/* Register the two built-in attributes. */
    attribute_table = append_attribute_entry_list(attribute_table, new_attribute_entry(new_orig_symbol(add_lydia_symbol("probability"), originNIL), to_type(new_float_type())));
    attribute_table = append_attribute_entry_list(attribute_table, new_attribute_entry(new_orig_symbol(add_lydia_symbol("observable"), originNIL), to_type(new_bool_type())));

    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGattribute_definition) {
            if (m->defs->arr[ix]->name->sym == add_lydia_symbol("observable") ||
                m->defs->arr[ix]->name->sym == add_lydia_symbol("probability")) {
                leh_error(ERR_REDEFINED_INT_ATTR,
                          LEH_LOCATION_GLOBAL,
                          m->defs->arr[ix]->name->org,
                          m->defs->arr[ix]->name->sym->name);
                result = 0;
                continue;
            }
            if (reverse_search_attribute_entry_list(attribute_table, m->defs->arr[ix]->name->sym, &pos)) {
                leh_error(ERR_REDEFINED_IDENTIFIER,
                          LEH_LOCATION_GLOBAL,
                          m->defs->arr[ix]->name->org,
                          m->defs->arr[ix]->name->sym->name,
                          attribute_table->arr[pos]->name->org,
                          m->defs->arr[ix]->name->sym->name);
                result = 0;
            }
            result &= check_type(to_attribute_definition(m->defs->arr[ix])->type, user_type_table);
            attribute_table = append_attribute_entry_list(attribute_table, new_attribute_entry(rdup_orig_symbol(m->defs->arr[ix]->name), rdup_type(to_attribute_definition(m->defs->arr[ix])->type)));
        }
    }

    return result;
}

int is_attribute(const_attribute_entry_list attribute_table, lydia_symbol name)
{
    return member_attribute_entry_list(attribute_table, name) ||
           name == add_lydia_symbol("probability") ||
           name == add_lydia_symbol("observable") ||
           name == add_lydia_symbol("health");
}

void check_internal_attribute(variable_attribute target,
                              attribute entry,
                              lydia_symbol system,
                              const_identifier id)
{
    register unsigned int iy;

    char *name = get_variable_name(id);
    if (target->name == add_lydia_symbol("observable")) {
        assert(TAGbool_variable_attribute == target->tag);
        for (iy = 1; iy < to_bool_variable_attribute(target)->values->sz; iy++) {
            if (to_bool_variable_attribute(target)->values->arr[iy] !=
                to_bool_variable_attribute(target)->values->arr[iy - 1]) {
                leh_error(WARN_NON_CONST_OBSERVABLE,
                          LEH_LOCATION_SYSTEM,
                          entry->var->name->name->org,
                          system->name,
                          name);
            }
        }
    }
/* To Do: Check if there are lacking fault or probability states. */
    if (target->name == add_lydia_symbol("health")) {
        int warn = 1;

        assert(TAGbool_variable_attribute == target->tag);
        for (iy = 1; iy < to_bool_variable_attribute(target)->values->sz; iy++) {
            if (to_bool_variable_attribute(target)->values->arr[iy] !=
                to_bool_variable_attribute(target)->values->arr[iy - 1]) {
                warn = 0;
            }
        }
        if (warn) {
            leh_error(WARN_CONST_HEALTH,
                      LEH_LOCATION_SYSTEM,
                      entry->var->name->name->org,
                      system->name,
                      name);
        }
    }
    if (target->name == add_lydia_symbol("probability")) {
        double sum = 0.0;
        int warn = 0;

        assert(TAGfloat_variable_attribute == target->tag);
        for (iy = 0; iy < to_float_variable_attribute(target)->values->sz; iy++) {
            sum += to_float_variable_attribute(target)->values->arr[iy];
            if (to_float_variable_attribute(target)->values->arr[iy] < 0 ||
                to_float_variable_attribute(target)->values->arr[iy] > 1) {
                leh_error(WARN_BAD_PROBABILITY_ENTRY,
                          LEH_LOCATION_SYSTEM,
                          entry->var->name->name->org,
                          system->name,
                          to_float_variable_attribute(target)->values->arr[iy],
                          name);
                warn = 1;
                break;
            }
        }
        if (!warn && fabs(sum - 1) > EPSILON) {
            leh_error(WARN_BAD_PROBABILITY,
                      LEH_LOCATION_SYSTEM,
                      entry->var->name->name->org,
                      system->name,
                      name);
        }
    }
    free(name);
}

static variable_identifier identifier_to_variable_identifier(identifier id)
{
    register unsigned int ix, iy;

    variable_identifier result = new_variable_identifier(new_orig_symbol(id->name, originNIL),
                                                         extent_listNIL,
                                                         variable_qualifier_listNIL);
    if (int_listNIL != id->indices) {
        result->ranges = new_extent_list();
        for (ix = 0; ix < id->indices->sz; ix++) {
            append_extent_list(result->ranges,
                               new_extent(to_expr(new_expr_int(to_type(new_int_type()), id->indices->arr[ix])),
                                          to_expr(new_expr_int(to_type(new_int_type()), id->indices->arr[ix]))));
        }
    }
    if (qualifier_listNIL != id->qualifiers) {
        result->qualifiers = new_variable_qualifier_list();
        for (ix = 0; ix < id->qualifiers->sz; ix++) {
            append_variable_qualifier_list(result->qualifiers, new_variable_qualifier(new_orig_symbol(id->qualifiers->arr[ix]->name, originNIL), extent_listNIL));
            if (int_listNIL != id->qualifiers->arr[ix]->indices) {
                result->qualifiers->arr[ix]->ranges = new_extent_list();
                for (iy = 0; iy < id->qualifiers->arr[ix]->indices->sz; iy++) {
                    append_extent_list(result->qualifiers->arr[ix]->ranges,
                                       new_extent(to_expr(new_expr_int(to_type(new_int_type()), id->qualifiers->arr[ix]->indices->arr[iy])),
                                                  to_expr(new_expr_int(to_type(new_int_type()), id->qualifiers->arr[ix]->indices->arr[iy]))));
                }
            }
        }
    }
    return result;
}

int cross_check_attributes(lydia_symbol system,
                           variable_list variables,
                           formal_list formals,
                           local_list locals)
{
    register unsigned int ix, result = 1;

    for (ix = 0; ix < variables->sz; ix++) {
        variable var = variables->arr[ix];
        if (is_health(var) &&
            double_listNIL == get_probabilities(var)) {

            char *name;

            origin org = originNIL;
            unsigned int pos;
            variable_identifier id = identifier_to_variable_identifier(var->name);
            if (search_formal_list(formals, id, &pos)) {
                org = formals->arr[pos]->name->name->org;
            }
            if (search_local_list(locals, id, &pos)) {
                org = locals->arr[pos]->name->name->org;
            }
            assert(originNIL != org);
            rfre_variable_identifier(id);

            name = get_variable_name(var->name);
            leh_error(ERR_MISSING_PROBABILITIES,
                      LEH_LOCATION_SYSTEM,
                      org,
                      system->name,
                      name);
            result = 0;
        }
    }

    return result;
}
