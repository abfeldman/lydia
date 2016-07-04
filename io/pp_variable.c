#include "variable.h"
#include "fwrite.h"
#include "lcm.h"
#include "mv.h"
#include "tv.h"

#include <assert.h>
#ifndef WIN32
# include <strings.h>
#endif

/**
 * Pretty prints a variable identifier.
 *
 * @param outfile stream to print to;
 * @param fm the variable identifier to print;
 */
void pp_variable_name(FILE *outfile, const_identifier id)
{
    unsigned int ix, iy;

    if (qualifier_listNIL != id->qualifiers) {
        for (ix = 0; ix < id->qualifiers->sz; ix++) {
            fprintf(outfile, "%s", id->qualifiers->arr[ix]->name->name);
            if (int_listNIL != id->qualifiers->arr[ix]->indices) {
                for (iy = 0; iy < id->qualifiers->arr[ix]->indices->sz; iy++) {
                    fprintf(outfile, "[%d]", id->qualifiers->arr[ix]->indices->arr[iy]);
                }
            }
            fprintf(outfile, ".");
        }
    }
    fprintf(outfile, "%s", id->name->name);
    if (int_listNIL != id->indices) {
        for (ix = 0; ix < id->indices->sz; ix++) {
            fprintf(outfile, "[%d]", id->indices->arr[ix]);
        }
    }
}

static void pp_variable_assignment_short(FILE *outfile,
                                         const_variable_assignment assignment,
                                         const_variable_list variables,
                                         const_values_set_list domains)
{
    const_variable var = variables->arr[assignment->var];

    switch (var->tag) {
        case TAGbool_variable:
            assert(TAGbool_variable_assignment == assignment->tag);
            if (!to_bool_variable_assignment(assignment)->value) {
                fprintf(outfile, "!");
            }
            pp_variable_name(outfile, var->name);
            break;
        case TAGint_variable:
            assert(TAGint_variable_assignment == assignment->tag);
            pp_variable_name(outfile, var->name);
            fprintf(outfile, " = ");
            fprintf(outfile,
                    "%d",
                    to_int_variable_assignment(assignment)->value);
            break;
        case TAGfloat_variable:
            assert(TAGfloat_variable_assignment == assignment->tag);
            pp_variable_name(outfile, var->name);
            fprintf(outfile, " = ");
            fprintf(outfile,
                    "%g",
                    to_float_variable_assignment(assignment)->value);
            break;
        case TAGenum_variable:
            assert(TAGenum_variable_assignment == assignment->tag);
            assert(NULL != domains);
            pp_variable_name(outfile, var->name);
            fprintf(outfile, " = ");
            fprintf(outfile,
                    "%s",
                    domains->arr[to_enum_variable(var)->values_set]->entries->arr[to_enum_variable_assignment(assignment)->value]->name);
            break;
        default:
            assert(0);
    }
}

/**
 * Pretty prints a fault-mode. This function is normally invoked when the
 * user types "fm" in any of the diagnostic engines. The function uses
 * the subroutines pp_variable_assignment_short() and pp_variable_name().
 *
 * @param outfile stream to print to;
 * @param fm the fault-mode to print;
 * @param variables a list of variables to use for the variable names;
 * @param domains a list of variable domains to use for the variable domains.
 */
/* To Do: Solve this with both Boolean and FDI domains. */
void pp_faultmode(FILE *outfile,
                  const_faultmode fm,
                  const_variable_list variables,
                  const_values_set_list domains)
{
    register unsigned int ix;

    int fg = 0;

    for (ix = 0; ix < fm->assignments->sz; ix++) {
        const_variable_assignment assignment = fm->assignments->arr[ix];
        const_variable var = variables->arr[assignment->var];
        int value;
        const_values_set domain;

        switch (var->tag) {
            case TAGbool_variable:
                assert(TAGbool_variable_assignment == assignment->tag);
                value = to_bool_variable_assignment(assignment)->value;

/* @@@ temp @@@ */
                if (!is_health(var)) {
                    if (fg) {
                        fprintf(outfile, ", ");
                    } else {
                        fg = 1;
                    }

                    fprintf(outfile, "!");
                    pp_variable_name(outfile, var->name);
                    continue;
                }
/* @@@ temp @@@ */
                if (!get_health_bool(var, value)) {

                    if (fg) {
                        fprintf(outfile, ", ");
                    } else {
                        fg = 1;
                    }

                    if (!value) {
                        fprintf(outfile, "!");
                    }
                    pp_variable_name(outfile, var->name);
                }
                break;
            case TAGenum_variable:
                assert(TAGenum_variable_assignment == assignment->tag);
                assert(NULL != domains);

                domain = domains->arr[to_enum_variable(var)->values_set];
                value = to_enum_variable_assignment(assignment)->value;

                if (!get_health_bool(var, value)) {

                    if (fg) {
                        fprintf(outfile, ", ");
                    } else {
                        fg = 1;
                    }

                    pp_variable_name(outfile, var->name);
                    fprintf(outfile, " = ");
                    fprintf(outfile, "%s", domain->entries->arr[value]->name);
                }
                break;
            case TAGint_variable:
            case TAGfloat_variable:
                break;
            default:
                assert(0);
        }
    }
}

void pp_variable_domain(FILE *outfile, const_variable var, const_values_set_list domains)
{
    register unsigned int iy;

    switch (var->tag) {
        case TAGbool_variable:
            fprintf(outfile, "false, true");
            break;
        case TAGint_variable:
/* TODO: Not yet implemented. */
            assert(0);
            break;
        case TAGfloat_variable:
/* TODO: Not yet implemented. */
            assert(0);
            break;
        case TAGenum_variable:
            if (NULL != domains) {
                for (iy = 0; iy < domains->arr[to_enum_variable(var)->values_set]->entries->sz; iy++) {
                    if (iy != 0) {
                        fprintf(outfile, ", ");
                    }
                    fprintf(outfile, "%s", domains->arr[to_enum_variable(var)->values_set]->entries->arr[iy]->name);
                }
            }
            break;
        default:
            assert(0);
    }
}

void pp_variable_attribute(FILE *outfile, const_variable var, const_variable_attribute attr, const_values_set_list domains)
{
    register unsigned int ix;

    fprintf(outfile, "  ");

    switch (attr->tag) {
        case TAGbool_variable_attribute:
            fprintf(outfile, "bool %s(", attr->name->name);
            pp_variable_domain(outfile, var, domains);
            fprintf(outfile, ") = (");
            for (ix = 0; ix < to_bool_variable_attribute(attr)->values->sz; ix++) {
                if (ix != 0) {
                    fprintf(outfile, ", ");
                }
                fprintf(outfile, "%s", to_bool_variable_attribute(attr)->values->arr[ix] ? "true" : "false");
            }
            break;
        case TAGint_variable_attribute:
            fprintf(outfile, "int %s(", attr->name->name);
            pp_variable_domain(outfile, var, domains);
            fprintf(outfile, ") = (");
            for (ix = 0; ix < to_int_variable_attribute(attr)->values->sz; ix++) {
                if (ix != 0) {
                    fprintf(outfile, ", ");
                }
                fprintf(outfile, "%d", to_int_variable_attribute(attr)->values->arr[ix]);
            }
            break;
        case TAGfloat_variable_attribute:
            fprintf(outfile, "float %s(", attr->name->name);
            pp_variable_domain(outfile, var, domains);
            fprintf(outfile, ") = (");
            for (ix = 0; ix < to_float_variable_attribute(attr)->values->sz; ix++) {
                if (ix != 0) {
                    fprintf(outfile, ", ");
                }
                fprintf(outfile, "%g", to_float_variable_attribute(attr)->values->arr[ix]);
            }
            break;
        case TAGenum_variable_attribute:
            fprintf(outfile, "enum %s(", attr->name->name);
            pp_variable_domain(outfile, var, domains);
            fprintf(outfile, ") = (");
            break;
        default:
            assert(0);
    }
    fprintf(outfile, ")\n");
}

void pp_variable_attribute_list(FILE *outfile, const_variable var, const_variable_attribute_list attributes, const_values_set_list domains)
{
    register unsigned int ix;

    for (ix = 0; ix < attributes->sz; ix++) {
        pp_variable_attribute(outfile, var, attributes->arr[ix], domains);
    }
}

/**
 * Prints a variable, its type, all its attributes and the values of the
 * attributes. This is usually used by the "vars" commands of the
 * diagnostic and simulation engines.
 *
 * @param outfile stream to print to;
 * @param var the variable to print;
 * @param domains the domains of all the variables.
 */
void pp_variable(FILE *outfile, const_variable var, const_values_set_list domains)
{
    switch (var->tag) {
        case TAGbool_variable:
            fprintf(outfile, "bool ");
            break;
        case TAGint_variable:
            fprintf(outfile, "int ");
            break;
        case TAGfloat_variable:
            fprintf(outfile, "float ");
            break;
        case TAGenum_variable:
            fprintf(outfile, "enum ");
            break;
        default:
            assert(0);
    }
    pp_variable_name(outfile, var->name);
    if (var->tag == TAGenum_variable && values_set_listNIL != domains) {
        fprintf(outfile, " [");
        pp_variable_domain(outfile, var, domains);
        fprintf(outfile, "]");
    }

    fprintf(outfile, "\n");
    pp_variable_attribute_list(outfile, var, var->attributes, domains);
}

void pp_variable_list(FILE *outfile,
                      const_variable_list variables,
                      const_values_set_list domains,
                      const char *query)
{
/**
 * To Do: Currently "query" can be an attribute name, and then
 * "pp_variable_list" would print these variable only that contain the
 * attribute specified in "query". It would be handy if, here, we
 * parse a small attribute query language.
 */
    register unsigned int ix;

    for (ix = 0; ix < variables->sz; ix++) {
        const_variable v = variables->arr[ix];
        if (NULL == query || find_attribute(v->attributes, add_lydia_symbol(query))) {
            pp_variable(outfile, v, domains);
        }
    }
}

void pp_variable_domain_value(FILE *outfile, const_variable var, const_values_set_list domains, int value)
{
    switch (var->tag) {
        case TAGbool_variable:
            fprintf(outfile, "%s", value ? "true" : "false");
            break;
        case TAGint_variable:
            assert(0);
            break;
        case TAGfloat_variable:
            assert(0);
            break;
        case TAGenum_variable:
            if (NULL != domains) {
                fprintf(outfile, "%s", domains->arr[to_enum_variable(var)->values_set]->entries->arr[value]->name);
            }
            break;
        default:
            assert(0);
    }
}

void pp_variable_attribute_value(FILE *outfile, const_variable var, const_variable_attribute attr, const_values_set_list domains, int value)
{
    fprintf(outfile, "  ");

    switch (attr->tag) {
        case TAGbool_variable_attribute:
            fprintf(outfile, "bool %s(", attr->name->name);
            pp_variable_domain_value(outfile, var, domains, value);
            fprintf(outfile, ") = %s\n", to_bool_variable_attribute(attr)->values->arr[value] ? "true" : "false");
            break;
        case TAGint_variable_attribute:
            fprintf(outfile, "int %s(", attr->name->name);
            pp_variable_domain_value(outfile, var, domains, value);
            fprintf(outfile, ") = %d\n", to_int_variable_attribute(attr)->values->arr[value]);
            break;
        case TAGfloat_variable_attribute:
            fprintf(outfile, "float %s(", attr->name->name);
            pp_variable_domain_value(outfile, var, domains, value);
            fprintf(outfile, ") = %g\n", to_float_variable_attribute(attr)->values->arr[value]);
            break;
        case TAGenum_variable_attribute:
            if (domains != NULL) {
                fprintf(outfile, "enum %s(", attr->name->name);
                pp_variable_domain_value(outfile, var, domains, value);
                fprintf(outfile, ") = %s\n", to_enum_variable_attribute(attr)->values->arr[value]->name);
            }
            break;
        default:
            assert(0);
    }
}

void pp_variable_attribute_value_list(FILE *outfile, const_variable var, const_variable_attribute_list attributes, const_values_set_list domains, int value)
{
    register unsigned int ix;

    for (ix = 0; ix < attributes->sz; ix++) {
        if (add_lydia_symbol("health") == attributes->arr[ix]->name ||
            add_lydia_symbol("observable") == attributes->arr[ix]->name) {
            continue;
        }
        pp_variable_attribute_value(outfile, var, attributes->arr[ix], domains, value);
    }
}

void pp_variable_assignment(FILE *outfile,
                            const_variable_assignment assignment,
                            const_variable_list variables,
                            const_values_set_list domains)
{
    const_variable var = variables->arr[assignment->var];

    if (is_health(var)) {
        fprintf(outfile, "health ");
    }
    if (is_observable(var)) {
        fprintf(outfile, "observable ");
    }
    switch (var->tag) {
        case TAGbool_variable:
            assert(TAGbool_variable_assignment == assignment->tag);
            fprintf(outfile, "bool ");
            pp_variable_assignment_short(outfile,
                                         assignment,
                                         variables,
                                         domains);
            fprintf(outfile, "\n");
            pp_variable_attribute_value_list(outfile, var, var->attributes, domains, to_bool_variable_assignment(assignment)->value ? 1 : 0);
            break;
        case TAGint_variable:
            assert(TAGint_variable_assignment == assignment->tag);
            fprintf(outfile, "int ");
            pp_variable_assignment_short(outfile, assignment, variables, domains);
            fprintf(outfile, "\n");
/* TODO: Print the attributes. */
            break;
        case TAGfloat_variable:
            assert(TAGfloat_variable_assignment == assignment->tag);
            fprintf(outfile, "float ");
            pp_variable_assignment_short(outfile, assignment, variables, domains);
            fprintf(outfile, "\n");
/* TODO: Print the attributes. */
            break;
        case TAGenum_variable:
            assert(TAGenum_variable_assignment == assignment->tag);
            assert(NULL != domains);
            fprintf(outfile, "enum ");
            pp_variable_assignment_short(outfile, assignment, variables, domains);
            fprintf(outfile, "\n");
            pp_variable_attribute_value_list(outfile, var, var->attributes, domains, to_enum_variable_assignment(assignment)->value);
            break;
        default:
            assert(0);
    }
}

void pp_variable_assignment_list(FILE *outfile, const_variable_assignment_list assignments, const_variable_list variables, const_values_set_list domains)
{
    register unsigned int ix;

    for (ix = 0; ix < assignments->sz; ix++) {
        pp_variable_assignment(outfile, assignments->arr[ix], variables, domains);
    }
}

unsigned int pp_tv_term(FILE *outfile, const_variable_list variables, const_tv_term term)
{
    unsigned int ix, iy = 0;

    const_int_list pos = term->pos;
    const_int_list neg = term->neg;

    const_variable var;

    for (ix = 0; ix < pos->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[term->pos->arr[ix]];
        pp_variable_name(outfile, var->name);
        fprintf(outfile, " = true");
    }
    for (ix = 0; ix < neg->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[term->neg->arr[ix]];
        pp_variable_name(outfile, var->name);
        fprintf(outfile, " = false");
    }

    return 1;
}

unsigned int pp_mv_term(FILE *outfile, const_variable_list variables, const_values_set_list values_sets, const_mv_term term /* sorted */)
{
    const_mv_literal_list pos = term->pos;
    const_mv_literal_list neg = term->neg;

    register unsigned int ix, iy, iz = 0, entries;

    for (ix = iy = 0; ix < pos->sz; ix++) {
        while (iy < neg->sz && neg->arr[iy]->var < pos->arr[ix]->var) {
            entries = to_enum_variable(variables->arr[neg->arr[iy]->var])->values_set;
            if (iz != 0) {
                fprintf(outfile, ", ");
            }
            pp_variable_name(outfile, variables->arr[neg->arr[iy]->var]->name);
/*
            fprintf(outfile,
                    " != %s.%s",
                    values_sets->arr[entries]->name->name,
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
*/
            fprintf(outfile,
                    " != %s",
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
            iy += 1;
            iz += 1;
        }
        entries = to_enum_variable(variables->arr[pos->arr[ix]->var])->values_set;
        if (iz != 0) {
            fprintf(outfile, ", ");
        }
        pp_variable_name(outfile, variables->arr[pos->arr[ix]->var]->name);
/*
        fprintf(outfile,
                " = %s.%s",
                values_sets->arr[entries]->name->name,
                values_sets->arr[entries]->entries->arr[pos->arr[ix]->val]->name);
*/
        fprintf(outfile,
                " = %s",
                values_sets->arr[entries]->entries->arr[pos->arr[ix]->val]->name);
        iz += 1;
        while (iy < neg->sz && neg->arr[iy]->var == pos->arr[ix]->var) {
            entries = to_enum_variable(variables->arr[neg->arr[iy]->var])->values_set;
            if (iz != 0) {
                fprintf(outfile, ", ");
            }
            pp_variable_name(outfile, variables->arr[neg->arr[iy]->var]->name);
/*
            fprintf(outfile,
                    " != %s.%s",
                    values_sets->arr[entries]->name->name,
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
*/
            fprintf(outfile,
                    " != %s",
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
            iy += 1;
            iz += 1;
        }
    }
    while (iy < neg->sz) {
        entries = to_enum_variable(variables->arr[neg->arr[iy]->var])->values_set;
        pp_variable_name(outfile, variables->arr[neg->arr[iy]->var]->name);
/*
        fprintf(outfile,
                " != %s.%s",
                values_sets->arr[entries]->name->name,
                values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
*/
        fprintf(outfile,
                " != %s",
                values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
        if (iz != 0) {
            fprintf(outfile, ", ");
        }
        iz += 1;
        iy += 1;
    }

    return 1;
}

unsigned int pp_tv_clause_short(FILE *outfile, const_variable_list variables, const_tv_clause clause)
{
    unsigned int ix, iy = 0;

    const_int_list pos = clause->pos;
    const_int_list neg = clause->neg;

    const_variable var;

    for (ix = 0; ix < pos->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[clause->pos->arr[ix]];
        pp_variable_name(outfile, var->name);
    }
    for (ix = 0; ix < neg->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[clause->neg->arr[ix]];
        fprintf(outfile, "!");
        pp_variable_name(outfile, var->name);
    }

    return 1;
}

unsigned int pp_tv_clause(FILE *outfile, const_variable_list variables, const_tv_clause clause)
{
    unsigned int ix, iy = 0;

    const_int_list pos = clause->pos;
    const_int_list neg = clause->neg;

    const_variable var;

    for (ix = 0; ix < pos->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[clause->pos->arr[ix]];
        pp_variable_name(outfile, var->name);
        fprintf(outfile, " = true");
    }
    for (ix = 0; ix < neg->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[clause->neg->arr[ix]];
        pp_variable_name(outfile, var->name);
        fprintf(outfile, " = false");
    }

    return 1;
}

unsigned int pp_mv_clause(FILE *outfile, const_variable_list variables, const_values_set_list values_sets, const_mv_clause clause /* sorted */)
{
    const_mv_literal_list pos = clause->pos;
    const_mv_literal_list neg = clause->neg;

    register unsigned int ix, iy, iz = 0, entries;

    for (ix = iy = 0; ix < pos->sz; ix++) {
        while (iy < neg->sz && neg->arr[iy]->var < pos->arr[ix]->var) {
            entries = to_enum_variable(variables->arr[neg->arr[iy]->var])->values_set;
            if (iz != 0) {
                fprintf(outfile, ", ");
            }
            pp_variable_name(outfile, variables->arr[neg->arr[iy]->var]->name);
/*
            fprintf(outfile,
                    " != %s.%s",
                    values_sets->arr[entries]->name->name,
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
*/
            fprintf(outfile,
                    " != %s",
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
            iy += 1;
            iz += 1;
        }
        entries = to_enum_variable(variables->arr[pos->arr[ix]->var])->values_set;
        if (iz != 0) {
            fprintf(outfile, ", ");
        }
        pp_variable_name(outfile, variables->arr[pos->arr[ix]->var]->name);
/*
        fprintf(outfile,
                " = %s.%s",
                values_sets->arr[entries]->name->name,
                values_sets->arr[entries]->entries->arr[pos->arr[ix]->val]->name);
*/
        fprintf(outfile,
                " = %s",
                values_sets->arr[entries]->entries->arr[pos->arr[ix]->val]->name);
        iz += 1;
        while (iy < neg->sz && neg->arr[iy]->var == pos->arr[ix]->var) {
            entries = to_enum_variable(variables->arr[neg->arr[iy]->var])->values_set;
            if (iz != 0) {
                fprintf(outfile, ", ");
            }
            pp_variable_name(outfile, variables->arr[neg->arr[iy]->var]->name);
/*
            fprintf(outfile,
                    " != %s.%s",
                    values_sets->arr[entries]->name->name,
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
*/
            fprintf(outfile,
                    " != %s",
                    values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
            iy += 1;
            iz += 1;
        }
    }
    while (iy < neg->sz) {
        entries = to_enum_variable(variables->arr[neg->arr[iy]->var])->values_set;
        pp_variable_name(outfile, variables->arr[neg->arr[iy]->var]->name);
/*
        fprintf(outfile,
                " != %s.%s",
                values_sets->arr[entries]->name->name,
                values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
*/
        fprintf(outfile,
                " != %s",
                values_sets->arr[entries]->entries->arr[neg->arr[iy]->val]->name);
        if (iz != 0) {
            fprintf(outfile, ", ");
        }
        iz += 1;
        iy += 1;
    }
    fprintf(outfile, "\n");

    return 1;
}

unsigned int pp_tv_term_short(FILE *outfile, const_variable_list variables, const_tv_term term)
{
    unsigned int ix, iy = 0;

    const_int_list pos = term->pos;
    const_int_list neg = term->neg;

    const_variable var;
/*
    for (ix = 0; ix < pos->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[term->pos->arr[ix]];
        pp_variable_name(outfile, var->name);
    }
    for (ix = 0; ix < neg->sz; ix++, iy++) {
        if (iy != 0) {
            fprintf(outfile, ", ");
        }
        var = variables->arr[term->neg->arr[ix]];
        fprintf(outfile, "!");
        pp_variable_name(outfile, var->name);
    }
*/

    for (ix = 0; ix < variables->sz; ix++) {
        var = variables->arr[ix];
        if (member_int_list(neg, ix)) {
            if (iy != 0) {
                fprintf(outfile, ", ");
            }
            fprintf(outfile, "!");
            pp_variable_name(outfile, var->name);
            iy += 1;
        }
        if (member_int_list(pos, ix)) {
            if (iy != 0) {
                fprintf(outfile, ", ");
            }
            pp_variable_name(outfile, var->name);
            iy += 1;
        }
    }

    return 1;
}
