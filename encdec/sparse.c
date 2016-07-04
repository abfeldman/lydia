#include <assert.h>
#include <math.h>

#include "tv.h"
#include "mv.h"
#include "util.h"
#include "variable.h"
#include "hierarchy.h"

static void process_attribute(variable_attribute attribute, unsigned int entry)
{
/* Special treatment of the built-in attributes. */
    if (attribute->name == add_lydia_symbol("observable")) {
        lydia_bool value;
        lydia_bool_list truncation;

/* This is a constant attribute. */
        assert(attribute->tag == TAGbool_variable_attribute);
        assert(to_bool_variable_attribute(attribute)->values->sz > 0);

        value = to_bool_variable_attribute(attribute)->values->arr[0];
        truncation = append_lydia_bool_list(append_lydia_bool_list(new_lydia_bool_list(), value), value);
        rfre_lydia_bool_list(to_bool_variable_attribute(attribute)->values);
        to_bool_variable_attribute(attribute)->values = truncation;
        return;
    }
    if (attribute->name == add_lydia_symbol("health")) {
        lydia_bool value;
        lydia_bool_list health;

        assert(attribute->tag == TAGbool_variable_attribute);
        assert(to_bool_variable_attribute(attribute)->values->sz > entry);

        value = to_bool_variable_attribute(attribute)->values->arr[entry];
        health = append_lydia_bool_list(append_lydia_bool_list(new_lydia_bool_list(), !value), value);
        rfre_lydia_bool_list(to_bool_variable_attribute(attribute)->values);
        to_bool_variable_attribute(attribute)->values = health;
        return;
    }
    if (attribute->name == add_lydia_symbol("probability")) {
        double value;
        double_list probabilities;

        assert(attribute->tag == TAGfloat_variable_attribute);
        assert(to_float_variable_attribute(attribute)->values->sz > entry);

        value = to_float_variable_attribute(attribute)->values->arr[entry];
        probabilities = append_double_list(append_double_list(new_double_list(), 1 - value), value);
        rfre_double_list(to_float_variable_attribute(attribute)->values);
        to_float_variable_attribute(attribute)->values = probabilities;
        return;
    }
}

static tv_wff_expr walk_expression(mv_wff_expr e, const_int_list offsets)
{
    switch (e->tag) {
        case TAGmv_wff_e_not:
            return to_tv_wff_expr(new_tv_wff_e_not(walk_expression(to_mv_wff_e_not(e)->n, offsets)));
        case TAGmv_wff_e_and:
            return to_tv_wff_expr(new_tv_wff_e_and(walk_expression(to_mv_wff_e_and(e)->lhs, offsets), walk_expression(to_mv_wff_e_and(e)->rhs, offsets)));
        case TAGmv_wff_e_or:
            return to_tv_wff_expr(new_tv_wff_e_or(walk_expression(to_mv_wff_e_or(e)->lhs, offsets), walk_expression(to_mv_wff_e_or(e)->rhs, offsets)));
        case TAGmv_wff_e_equiv:
            return to_tv_wff_expr(new_tv_wff_e_equiv(walk_expression(to_mv_wff_e_equiv(e)->lhs, offsets), walk_expression(to_mv_wff_e_equiv(e)->rhs, offsets)));
        case TAGmv_wff_e_impl:
            return to_tv_wff_expr(new_tv_wff_e_impl(walk_expression(to_mv_wff_e_impl(e)->lhs, offsets), walk_expression(to_mv_wff_e_impl(e)->rhs, offsets)));
        case TAGmv_wff_e_var:
            return to_tv_wff_expr(new_tv_wff_e_var(offsets->arr[to_mv_wff_e_var(e)->var] + to_mv_wff_e_var(e)->val));
        case TAGmv_wff_e_const:
            return to_tv_wff_expr(new_tv_wff_e_const(to_mv_wff_e_const(e)->c));
    }

    assert(0);
    return tv_wff_exprNIL;
}

static void convert_expressions(const_mv_wff_expr_list input,
                                tv_wff_expr_list output,
                                const_int_list offsets)
{
    register unsigned int ix;

    for (ix = 0; ix < input->sz; ix++) {
        append_tv_wff_expr_list(output,
                                walk_expression(input->arr[ix],
                                                offsets));
    }
}

static void convert_variables(const_variable_list input,
                              const_values_set_list domains,
                              variable_list output,
                              int_list offsets)
{
    register unsigned int ix, iy, iz;

    assert(variable_listNIL != output); /* Should be created. */
    assert(int_listNIL != offsets);     /* Ditto. */

    for (ix = 0; ix < input->sz; ix++) {
        append_int_list(offsets, output->sz);
        if (TAGbool_variable == input->arr[ix]->tag) {
            append_variable_list(output, rdup_variable(input->arr[ix]));
            continue;
        }
        if (TAGenum_variable == input->arr[ix]->tag) {
            int domain_index = to_enum_variable(input->arr[ix])->values_set;
            const_values_set domain = domains->arr[domain_index];
            for (iy = 0; iy < domain->entries->sz; iy++) {
                char *name = malloc(((size_t)log10(iy + 10) + strlen(input->arr[ix]->name->name->name) + 2) * sizeof(char));

                variable_attribute_list attributes;
                int_list indices;
                qualifier_list qualifiers;

                sprintf(name, "%s$%d", input->arr[ix]->name->name->name, iy);
                attributes = rdup_variable_attribute_list(input->arr[ix]->attributes);
                indices = rdup_int_list(input->arr[ix]->name->indices);
                qualifiers = rdup_qualifier_list(input->arr[ix]->name->qualifiers);
                for (iz = 0; iz < attributes->sz; iz++) {
                    process_attribute(attributes->arr[iz], iy);
                }
                append_variable_list(output, to_variable(new_bool_variable(new_identifier(add_lydia_symbol(name), indices, qualifiers), attributes, ix)));
                free(name);
            }
            continue;
        }
    }
}

static void add_constraints(const_variable_list variables,
                            const_values_set_list domains,
                            tv_wff_expr_list output,
                            const_int_list offsets)
{
    register unsigned int ix, iy, iz;

    for (ix = 0; ix < variables->sz; ix++) {
        if (TAGenum_variable == variables->arr[ix]->tag) {
            int offset = offsets->arr[ix];
            const_values_set domain = domains->arr[to_enum_variable(variables->arr[ix])->values_set];

            tv_wff_expr alo = tv_wff_exprNIL;
            tv_wff_expr amo = tv_wff_exprNIL;
            for (iy = 0; iy < domain->entries->sz; iy++) {
                tv_wff_expr expr = to_tv_wff_expr(new_tv_wff_e_var(offset + iy));
                alo = ((alo == tv_wff_exprNIL) ? expr : to_tv_wff_expr(new_tv_wff_e_or(alo, expr)));
                for (iz = iy + 1; iz < domain->entries->sz; iz++) {
                    tv_wff_expr expr = to_tv_wff_expr(new_tv_wff_e_or(to_tv_wff_expr(new_tv_wff_e_not(to_tv_wff_expr(new_tv_wff_e_var(offset + iy)))), to_tv_wff_expr(new_tv_wff_e_not(to_tv_wff_expr(new_tv_wff_e_var(offset + iz))))));
                    amo = ((amo == tv_wff_exprNIL) ? expr : to_tv_wff_expr(new_tv_wff_e_and(amo, expr)));
                }
            }
            if (alo != tv_wff_exprNIL) {
                append_tv_wff_expr_list(output, alo);
            }
            if (amo != tv_wff_exprNIL) {
                append_tv_wff_expr_list(output, amo);
            }
        }
    }
}

tv_wff sparse_encode_wff(const_mv_wff input)
{
    int_list offsets;

    tv_wff output;

    output = new_tv_wff(rdup_values_set_list(input->domains),
                        new_variable_list(),
                        rdup_variable_list(input->variables),
                        new_constant_list(),
                        ENCODING_SPARSE,
                        new_tv_wff_expr_list());

    offsets = new_int_list();
    convert_variables(input->variables,
                      input->domains,
                      output->variables,
                      offsets);
    convert_expressions(input->e, output->e, offsets);
    add_constraints(input->variables,
                    input->domains,
                    output->e,
                    offsets);
    rfre_int_list(offsets);

    return output;
}

static identifier qualify_identifier(const_identifier id, int value)
{
    identifier result;

    char *buf = (char *)malloc(strlen(id->name->name) + (size_t)log10(value + 10) + 2);
    sprintf(buf, "%s$%d", id->name->name, value);

    result = new_identifier(add_lydia_symbol(buf),
                            rdup_int_list(id->indices),
                            rdup_qualifier_list(id->qualifiers));
    free(buf);

    return result;
}

extern edge_list sparse_encode_edges(const_edge_list edges_input,
                                     const_variable_list variables,
                                     const_values_set_list domains)
{
    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;

    unsigned int pos;

    edge_list edges_output;

    if (edges_input == edge_listNIL) {
        return edge_listNIL;
    }

    edges_output = new_edge_list();

    for (ix = 0; ix < edges_input->sz; ix++) {
        edge edge_input = edges_input->arr[ix];
        edge edge_output = new_edge(edge_input->type,
                                    edge_input->name,
                                    rdup_int_list(edge_input->indices),
                                    rdup_mapping_list(edge_input->bindings));
        for (iy = 0; iy < edge_input->bindings->sz; iy++) {
            identifier from = edge_input->bindings->arr[iy]->from;
            identifier to = edge_input->bindings->arr[iy]->to;

            lydia_symbol_list domain_from;

            if (!search_variable_list(variables, from, &pos)) {
                assert(0);
                abort();
            }
            assert(TAGenum_variable == variables->arr[pos]->tag);
            domain_from = domains->arr[to_enum_variable(variables->arr[pos])->values_set]->entries;
            for (iz = 0; iz < domain_from->sz; iz++) {
                append_mapping_list(edge_output->bindings,
                                    new_mapping(qualify_identifier(from, iz),
                                                qualify_identifier(to, iz)));
            }
        }
        append_edge_list(edges_output, edge_output);
    }

    return edges_output;
}
