#include "sorted_int_list.h"
#include "hierarchy.h"
#include "variable.h"
#include "sparse.h"
#include "dense.h"
#include "enc.h"
#include "tv.h"
#include "mv.h"

#include <assert.h>

int mvwff2tvwff(const_mv_wff_hierarchy input,
                tv_wff_hierarchy *output,
                const int encoding)
{
    register unsigned int ix;

    mv_wff node_input;
    tv_wff node_output;
    edge_list edges_input;
    edge_list edges_output;

    node result;

    *output = new_tv_wff_hierarchy(new_node_list());

    for (ix = 0; ix < input->nodes->sz; ix++) {
        node_input = to_mv_wff(input->nodes->arr[ix]->constraints);
        edges_input = input->nodes->arr[ix]->edges;

        if (ENCODING_SPARSE == encoding) {
            node_output = sparse_encode_wff(node_input);
            edges_output = sparse_encode_edges(edges_input,
                                               node_input->variables,
                                               node_input->domains);
        } else if (ENCODING_DENSE == encoding) {
            node_output = dense_encode_wff(node_input);
            edges_output = dense_encode_edges(edges_input,
                                              node_input->variables,
                                              node_input->domains);
        } else {
            assert(0);
            abort();
        }
        result = new_node(input->nodes->arr[ix]->type,
                          edges_output,
                          to_kb(node_output));
        append_node_list((*output)->nodes, result);
    }

    return 1;
}

tv_term encode_term(const_mv_term term,
                    const_variable_list variables,
                    const_variable_list encoded_variables,
                    const_values_set_list domains,
                    const int encoding)
{
    unsigned int ix;
    unsigned int iy;

    tv_term alpha;

    int_list offsets;

    assert(term->neg->sz == 0);

    alpha = new_tv_term(new_int_list(), new_int_list());
    if (encoding == ENCODING_NONE) {
        assert(0);
        abort();
    }

    offsets = new_int_list();
    for (ix = 0; ix < variables->sz; ix++) {
        assert(TAGbool_variable == variables->arr[ix]->tag);
        if (ix == 0 || to_bool_variable(variables->arr[ix])->encoded_variable != to_bool_variable(variables->arr[ix - 1])->encoded_variable) {
            assert(to_bool_variable(variables->arr[ix])->encoded_variable != -1);
            append_int_list(offsets, ix);
        }
    }
    assert(offsets->sz == encoded_variables->sz);

    if (encoding == ENCODING_SPARSE) {
        for (ix = 0; ix < term->pos->sz; ix++) {
            mv_literal lit;

            lit = term->pos->arr[ix];

            assert(lit->var < (int)offsets->sz);
            append_int_list(alpha->pos, offsets->arr[lit->var] + lit->val);
        }

        rfre_int_list(offsets);

        alpha->pos = sort_int_list(alpha->pos);
        alpha->neg = sort_int_list(alpha->neg);

        return alpha;
    }

    assert(encoding == ENCODING_DENSE);
    for (ix = 0; ix < term->pos->sz; ix++) {
        register unsigned int k;
        register unsigned int l;
        register unsigned int m;
        register unsigned int n;
        register unsigned int q;
        register unsigned int v;
        register unsigned int bits;

        mv_literal lit;

        lit = term->pos->arr[ix];

        v = lit->val;
        n = domains->arr[to_enum_variable(encoded_variables->arr[lit->var])->values_set]->entries->sz;
        m = n - 1, bits = 1, q = 2;
        while (m > 1) {
            bits += 1;
            m = m >> 1;
            q = q << 1;
        }
        k = q - n;
        l = (v < k) ? v : v + k;
        if (l < k) {
            bits -= 1;
        }
        for (iy = 0; iy < bits; iy++) {
            assert(lit->var < (int)offsets->sz);
            if (l & 1) {
                append_int_list(alpha->pos, offsets->arr[lit->var] + iy + ((v < k) ? 1 : 0));
            } else {
                append_int_list(alpha->neg, offsets->arr[lit->var] + iy + ((v < k) ? 1 : 0));
            }
            l = l >> 1;
        }
    }
    rfre_int_list(offsets);

    alpha->pos = sort_int_list(alpha->pos);
    alpha->neg = sort_int_list(alpha->neg);

    return alpha;
}
