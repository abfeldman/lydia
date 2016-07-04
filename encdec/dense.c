#include "sorted_int_list.h"
#include "hierarchy.h"
#include "variable.h"
#include "util.h"
#include "tv.h"
#include "mv.h"

#include <assert.h>
#include <math.h>

static unsigned int get_bits(const unsigned int n,
                             unsigned int *k)
{
    register unsigned int m = n - 1, bits = 1, q = 2;

    assert(n > 0);
    while (m > 1) {
        bits += 1;
        m = m >> 1;
        q = q << 1;
    }
    *k = q - n;

    return bits;
}

static tv_wff_expr encode_variable(const_mv_wff_e_var var,
                                   const unsigned int n,
                                   const unsigned int offset)
{
    tv_wff_expr result = tv_wff_exprNIL;

    register unsigned int iy, v, bits;
    unsigned int k, l;

    v = var->val;
    bits = get_bits(n, &k);

    l = (v < k) ? v : v + k;

    assert(v < n);

    if (l < k) {
        bits -= 1;
    }
    for (iy = 0; iy < bits; iy++) {
        tv_wff_expr bv = to_tv_wff_expr(new_tv_wff_e_var(iy + offset + ((v < k) ? 1 : 0)));
        if (!(l & 1)) {
            bv = to_tv_wff_expr(new_tv_wff_e_not(bv));
        }
        if (tv_wff_exprNIL != result) {
            result = to_tv_wff_expr(new_tv_wff_e_and(bv, result));
        } else {
            result = bv;
        }
        l = l >> 1;
    }

    return result;
}

static int is_control(const_variable var)
{
    const_variable_attribute attr = find_attribute(var->attributes, add_lydia_symbol("control"));
    if (attr == variable_attributeNIL) {
        return 0;
    }
    assert(attr->tag == TAGbool_variable_attribute);
    return to_bool_variable_attribute(attr)->values->arr[0];
}

static void process_observable_attribute(const_variable var,
                                         variable_list encoding_vars)
{
    bool_variable_attribute observable;
    bool_variable_attribute control;
    bool_variable_attribute output;

    if (is_observable(var)) {
        register unsigned int ix;

        for (ix = 0; ix < encoding_vars->sz; ix++) {
            observable = new_bool_variable_attribute(add_lydia_symbol("observable"),
                                                     new_lydia_bool_list());
            append_lydia_bool_list(observable->values, LYDIA_TRUE);
            append_lydia_bool_list(observable->values, LYDIA_TRUE);
            if (variable_attribute_listNIL == encoding_vars->arr[ix]->attributes) {
                encoding_vars->arr[ix]->attributes = new_variable_attribute_list();
            }
            append_variable_attribute_list(encoding_vars->arr[ix]->attributes, to_variable_attribute(observable));
        }
    }
    if (is_control(var)) {
        register unsigned int ix;

        for (ix = 0; ix < encoding_vars->sz; ix++) {
            control = new_bool_variable_attribute(add_lydia_symbol("control"),
                                                     new_lydia_bool_list());
            append_lydia_bool_list(control->values, LYDIA_TRUE);
            append_lydia_bool_list(control->values, LYDIA_TRUE);
            if (variable_attribute_listNIL == encoding_vars->arr[ix]->attributes) {
                encoding_vars->arr[ix]->attributes = new_variable_attribute_list();
            }
            append_variable_attribute_list(encoding_vars->arr[ix]->attributes, to_variable_attribute(control));
        }
    }
    if (is_output(var)) {
        register unsigned int ix;

        for (ix = 0; ix < encoding_vars->sz; ix++) {
            output = new_bool_variable_attribute(add_lydia_symbol("output"),
                                                     new_lydia_bool_list());
            append_lydia_bool_list(output->values, LYDIA_TRUE);
            append_lydia_bool_list(output->values, LYDIA_TRUE);
            if (variable_attribute_listNIL == encoding_vars->arr[ix]->attributes) {
                encoding_vars->arr[ix]->attributes = new_variable_attribute_list();
            }
            append_variable_attribute_list(encoding_vars->arr[ix]->attributes, to_variable_attribute(output));
        }
    }
}

static void process_health_attribute(const_enum_variable var,
                                     variable_list encoding_vars)
{
    bool_variable_attribute health;
    lydia_symbol health_symbol;

    if (is_health(to_variable(var))) {
        register unsigned int ix;

        health_symbol = add_lydia_symbol("assumable");

        for (ix = 0; ix < encoding_vars->sz; ix++) {
            health = new_bool_variable_attribute(health_symbol,
                                                 new_lydia_bool_list());
            append_lydia_bool_list(health->values, LYDIA_TRUE);
            append_lydia_bool_list(health->values, LYDIA_TRUE);
            if (variable_attribute_listNIL == encoding_vars->arr[ix]->attributes) {
                encoding_vars->arr[ix]->attributes = new_variable_attribute_list();
            }
            append_variable_attribute_list(encoding_vars->arr[ix]->attributes,
                                           to_variable_attribute(health));
        }
    }
}

static tv_wff_expr walk_expression(mv_wff_expr e,
                                   const_int_list domains,
                                   const_int_list offsets)
{
    switch (e->tag) {
        case TAGmv_wff_e_not:
            return to_tv_wff_expr(new_tv_wff_e_not(walk_expression(to_mv_wff_e_not(e)->n, domains, offsets)));
        case TAGmv_wff_e_and:
            return to_tv_wff_expr(new_tv_wff_e_and(walk_expression(to_mv_wff_e_and(e)->lhs, domains, offsets), walk_expression(to_mv_wff_e_and(e)->rhs, domains, offsets)));
        case TAGmv_wff_e_or:
            return to_tv_wff_expr(new_tv_wff_e_or(walk_expression(to_mv_wff_e_or(e)->lhs, domains, offsets), walk_expression(to_mv_wff_e_or(e)->rhs, domains, offsets)));
        case TAGmv_wff_e_equiv:
            return to_tv_wff_expr(new_tv_wff_e_equiv(walk_expression(to_mv_wff_e_equiv(e)->lhs, domains, offsets), walk_expression(to_mv_wff_e_equiv(e)->rhs, domains, offsets)));
        case TAGmv_wff_e_impl:
            return to_tv_wff_expr(new_tv_wff_e_impl(walk_expression(to_mv_wff_e_impl(e)->lhs, domains, offsets), walk_expression(to_mv_wff_e_impl(e)->rhs, domains, offsets)));
        case TAGmv_wff_e_var:
            return encode_variable(to_mv_wff_e_var(e), domains->arr[to_mv_wff_e_var(e)->var], offsets->arr[to_mv_wff_e_var(e)->var]);
        case TAGmv_wff_e_const:
            return to_tv_wff_expr(new_tv_wff_e_const(to_mv_wff_e_const(e)->c));
    }

    assert(0);
    return tv_wff_exprNIL;
}

static void convert_expressions(const_mv_wff_expr_list input,
                                tv_wff_expr_list output,
                                const_int_list domains,
                                const_int_list offsets)
{
    register unsigned int ix;

    for (ix = 0; ix < input->sz; ix++) {
        append_tv_wff_expr_list(output,
                                walk_expression(input->arr[ix],
                                                domains,
                                                offsets));
    }
}

/**
 * Convert each multi-valued variable in the input to log(n) Boolean
 * variables, where n is the domain size of the respective
 * variable. Each Boolean variable has the name of the multi-valued
 * variable followed by $k, where k is the bit offset in the
 * corresponding dense encoding. This function also handles the case
 * in which the domain size is not a power of 2. For more information
 * cf. the article "A Multi-Valued SAT-Based Algorithm for Faster
 * Model-Based Diagnosis" from the Lydia web site.
 *
 * @param input the input variables;
 * @param domains the input domains;
 * @param output an empty list where the resulting variables will be
 *        created;
 * @param domain_sizes an empty list of integers where for each
 *        original variable its domain size will be stored;
 * @param offsets an empty list of integers where for each variable
 *        an offset to its first encoding variable will be stored.
 */
static void convert_variables(const_variable_list input,
                              const_values_set_list domains,
                              variable_list output,
                              int_list domain_sizes,
                              int_list offsets)
{
    variable_list encoding_vars;
    const_values_set domain;
    int_list indices;
    qualifier_list qualifiers;
    bool_variable encoded_variable;

    register unsigned int ix, iy;
    unsigned int k, bits, domain_size;

    assert(variable_listNIL != output);  /* Should be created. */
    assert(int_listNIL != domain_sizes); /* Ditto. */
    assert(int_listNIL != offsets);      /* Ditto. */

    for (ix = 0; ix < input->sz; ix++) {
        append_int_list(offsets, output->sz);
        if (TAGbool_variable == input->arr[ix]->tag) {
            append_variable_list(output, rdup_variable(input->arr[ix]));
            append_int_list(domain_sizes, 2);
            continue;
        }
        if (TAGenum_variable == input->arr[ix]->tag) {
            domain = domains->arr[to_enum_variable(input->arr[ix])->values_set];

            domain_size = domain->entries->sz;
            append_int_list(domain_sizes, domain_size);

            bits = get_bits(domain_size, &k);
            encoding_vars = new_variable_list();

            for (iy = 0; iy < bits; iy++) {
                char *name = (char *)malloc(((size_t)log10(iy + 10) + strlen(input->arr[ix]->name->name->name) + 2) * sizeof(char));
                sprintf(name, "%s$%d", input->arr[ix]->name->name->name, iy);

                indices = rdup_int_list(input->arr[ix]->name->indices);
                qualifiers = rdup_qualifier_list(input->arr[ix]->name->qualifiers);
                encoded_variable = new_bool_variable(new_identifier(add_lydia_symbol(name), indices, qualifiers), variable_attribute_listNIL, ix);
                append_variable_list(output, to_variable(encoded_variable));
                append_variable_list(encoding_vars, to_variable(encoded_variable));

                free(name);
            }
            process_observable_attribute(input->arr[ix], encoding_vars);
            process_health_attribute(to_enum_variable(input->arr[ix]),
                                     encoding_vars);

            fre_variable_list(encoding_vars);
        }
    }
}

static void add_constraints(const_variable_list variables,
                            tv_wff_expr_list output,
                            const_int_list domain_sizes,
                            const_int_list offsets)
{
    register unsigned int ix, iy, iz;
    unsigned int k, bits, domain_size, offset;

    for (ix = 0; ix < variables->sz; ix++) {
        if (TAGenum_variable == variables->arr[ix]->tag) {
            domain_size = domain_sizes->arr[ix];
            offset = offsets->arr[ix];

            bits = get_bits(domain_size, &k);

            for (iy = 0; iy < k; iy++) {
                tv_wff_expr lhs = tv_wff_exprNIL;
                tv_wff_expr rhs = to_tv_wff_expr(new_tv_wff_e_var(offset));
                for (iz = 0; iz < bits - 1; iz++) {
                    unsigned int l = bits - iz - 1;
                    tv_wff_expr bit = to_tv_wff_expr(new_tv_wff_e_var(offset + l));
                    if (!((iy >> (l - 1)) & 1)) {
                        bit = to_tv_wff_expr(new_tv_wff_e_not(bit));
                    }
                    lhs = (lhs == tv_wff_exprNIL ? bit : to_tv_wff_expr(new_tv_wff_e_and(bit, rdup_tv_wff_expr(lhs))));
                }
                append_tv_wff_expr_list(output, to_tv_wff_expr(new_tv_wff_e_impl(lhs, rhs)));
            }
        }
    }
}

tv_wff dense_encode_wff(const_mv_wff input)
{
    int_list offsets;
    int_list domain_sizes;

    tv_wff output;

    output = new_tv_wff(rdup_values_set_list(input->domains),
                        new_variable_list(),
                        rdup_variable_list(input->variables),
                        new_constant_list(),
                        ENCODING_DENSE,
                        new_tv_wff_expr_list());

    offsets = new_int_list();
    domain_sizes = new_int_list();
    convert_variables(input->variables,
                      input->domains,
                      output->variables,
                      domain_sizes,
                      offsets);
    convert_expressions(input->e, output->e, domain_sizes, offsets);
    add_constraints(input->variables,
                    output->e,
                    domain_sizes,
                    offsets);
    rfre_int_list(domain_sizes);
    rfre_int_list(offsets);

    return output;
}

static identifier qualify_identifier(const_identifier id, const unsigned int value)
{
    identifier result;

    char *buf = (char *)malloc(((size_t)log10(value + 10) + strlen(id->name->name) + 2) * sizeof(char));
    sprintf(buf, "%s$%d", id->name->name, value);
    result = new_identifier(add_lydia_symbol(buf),
                            rdup_int_list(id->indices),
                            rdup_qualifier_list(id->qualifiers));
    free(buf);
    return result;
}

extern edge_list dense_encode_edges(const_edge_list edges_input,
                                    const_variable_list variables,
                                    const_values_set_list domains)
{
    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;

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

            unsigned int pos, k, bits;
            if (!search_variable_list(variables, from, &pos)) {
                assert(0);
                abort();
            }
            assert(TAGenum_variable == variables->arr[pos]->tag);
            domain_from = domains->arr[to_enum_variable(variables->arr[pos])->values_set]->entries;

            bits = get_bits(domain_from->sz, &k);
            for (iz = 0; iz < bits; iz++) {
                append_mapping_list(edge_output->bindings,
                                    new_mapping(qualify_identifier(from, iz),
                                                qualify_identifier(to, iz)));
            }
        }
        append_edge_list(edges_output, edge_output);
    }

    return edges_output;
}
