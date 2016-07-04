#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "sorted_int_list.h"
#include "serializable.h"
#include "hierarchy.h"
#include "simplify.h"
#include "flat_kb.h"
#include "inline.h"
#include "util.h"
#include "stat.h"
#include "defs.h"
#include "lcm.h"
#include "mv.h"
#include "tv.h"

static int option_simplify;
static int option_verbose;

static csp_term map_csp_term(csp_term term,
                             int_list_list v_mappings,
                             int_list_list c_mappings,
                             const unsigned int current_map)
{
    unsigned int ix;

    switch (term->tag) {
        case TAGcsp_function_term:
            for (ix = 0; ix < to_csp_function_term(term)->args->sz; ix++) {
                if (csp_termNIL != to_csp_function_term(term)->args->arr[ix]) {
                    map_csp_term(to_csp_function_term(term)->args->arr[ix], v_mappings, c_mappings, current_map);
                }
            }
            break;
        case TAGcsp_constant_term:
            to_csp_constant_term(term)->c = c_mappings->arr[current_map]->arr[to_csp_constant_term(term)->c];
            break;
        case TAGcsp_variable_term:
            to_csp_variable_term(term)->v = v_mappings->arr[current_map]->arr[to_csp_variable_term(term)->v];
            break;
        default:
            assert(0);
            break;
    }
    return term;
}

static csp_sentence map_csp_sentence(csp_sentence sentence,
                                     int_list_list v_mappings,
                                     int_list_list c_mappings,
                                     const unsigned int current_map)
{
    switch (sentence->tag) {
        case TAGcsp_not_sentence:
            map_csp_sentence(to_csp_not_sentence(sentence)->n, v_mappings, c_mappings, current_map);
            break;
        case TAGcsp_and_sentence:
            map_csp_sentence(to_csp_and_sentence(sentence)->lhs, v_mappings, c_mappings, current_map);
            map_csp_sentence(to_csp_and_sentence(sentence)->rhs, v_mappings, c_mappings, current_map);
            break;
        case TAGcsp_or_sentence:
            map_csp_sentence(to_csp_or_sentence(sentence)->lhs, v_mappings, c_mappings, current_map);
            map_csp_sentence(to_csp_or_sentence(sentence)->rhs, v_mappings, c_mappings, current_map);
            break;
        case TAGcsp_impl_sentence:
            map_csp_sentence(to_csp_impl_sentence(sentence)->lhs, v_mappings, c_mappings, current_map);
            map_csp_sentence(to_csp_impl_sentence(sentence)->rhs, v_mappings, c_mappings, current_map);
            break;
        case TAGcsp_equiv_sentence:
            map_csp_sentence(to_csp_equiv_sentence(sentence)->lhs, v_mappings, c_mappings, current_map);
            map_csp_sentence(to_csp_equiv_sentence(sentence)->rhs, v_mappings, c_mappings, current_map);
            break;
        case TAGcsp_atomic_sentence:
            map_csp_term(to_csp_atomic_sentence(sentence)->a, v_mappings, c_mappings, current_map);
            break;
        default:
            assert(0);
            break;
    }
    return sentence;
}

static csp_sentence_list map_csp(csp_sentence_list sentences,
                                 int_list_list variable_mappings,
                                 int_list_list constant_mappings,
                                 const unsigned int current_map)
{
    register unsigned int ix;

    csp_sentence_list result = rdup_csp_sentence_list(sentences);

    for (ix = 0; ix < result->sz; ix++) {
        map_csp_sentence(result->arr[ix],
                         variable_mappings,
                         constant_mappings,
                         current_map);
    }

    return result;
}

static csp_sentence_list multiply_csp(csp_sentence_list left,
                                      csp_sentence_list right,
                                      int_list_list variable_mappings,
                                      int_list_list constant_mappings,
                                      const unsigned int current_map)
{
    csp_sentence_list result = rdup_csp_sentence_list(left);

    result = concat_csp_sentence_list(result, map_csp(right,
                                                      variable_mappings,
                                                      constant_mappings,
                                                      current_map));

    return result;
}

static tv_wff_expr map_tv_wff_expr(tv_wff_expr e,
                                   int_list_list v_mappings,
                                   const unsigned int current_map)
{
    switch (e->tag) {
        case TAGtv_wff_e_or:
            to_tv_wff_e_or(e)->lhs = map_tv_wff_expr(to_tv_wff_e_or(e)->lhs, v_mappings, current_map);
            to_tv_wff_e_or(e)->rhs = map_tv_wff_expr(to_tv_wff_e_or(e)->rhs, v_mappings, current_map);
            break;
        case TAGtv_wff_e_not:
            to_tv_wff_e_not(e)->n = map_tv_wff_expr(to_tv_wff_e_not(e)->n, v_mappings, current_map);
            break;
        case TAGtv_wff_e_and:
            to_tv_wff_e_and(e)->lhs = map_tv_wff_expr(to_tv_wff_e_and(e)->lhs, v_mappings, current_map);
            to_tv_wff_e_and(e)->rhs = map_tv_wff_expr(to_tv_wff_e_and(e)->rhs, v_mappings, current_map);
            break;
        case TAGtv_wff_e_equiv:
            to_tv_wff_e_equiv(e)->lhs = map_tv_wff_expr(to_tv_wff_e_equiv(e)->lhs, v_mappings, current_map);
            to_tv_wff_e_equiv(e)->rhs = map_tv_wff_expr(to_tv_wff_e_equiv(e)->rhs, v_mappings, current_map);
            break;
        case TAGtv_wff_e_impl:
            to_tv_wff_e_impl(e)->lhs = map_tv_wff_expr(to_tv_wff_e_impl(e)->lhs, v_mappings, current_map);
            to_tv_wff_e_impl(e)->rhs = map_tv_wff_expr(to_tv_wff_e_impl(e)->rhs, v_mappings, current_map);
            break;
        case TAGtv_wff_e_var:
            to_tv_wff_e_var(e)->v = v_mappings->arr[current_map]->arr[to_tv_wff_e_var(e)->v];
            break;
        case TAGtv_wff_e_const:
/* Noop. */
            break;
    }
    return e;
}

static tv_wff_expr flatten_tv_wff_expr_list(tv_wff_expr_list e)
{
    register unsigned int ix;
    tv_wff_expr result = tv_wff_exprNIL;

    if (e->sz > 0) {
        result = rdup_tv_wff_expr(e->arr[0]);
    }
    for (ix = 1; ix < e->sz; ix++) {
        result = to_tv_wff_expr(new_tv_wff_e_and(result, rdup_tv_wff_expr(e->arr[ix])));
    }
    return result;
}

static tv_wff_expr_list map_tv_wff(tv_wff_expr_list list,
                                   int_list_list variable_mappings,
                                   int_list_list UNUSED(constant_mappings),
                                   const unsigned int current_map)
{
    register unsigned int ix;

    tv_wff_expr_list result = new_tv_wff_expr_list();
    tv_wff_expr expr = flatten_tv_wff_expr_list(list);
    if (expr != tv_wff_exprNIL) {
        append_tv_wff_expr_list(result, expr);
    }

    for (ix = 0; ix < result->sz; ix++) {
        map_tv_wff_expr(result->arr[ix], variable_mappings, current_map);
    }

    return result;
}

static tv_wff_expr_list multiply_tv_wff(tv_wff_expr_list left,
                                        tv_wff_expr_list right,
                                        int_list_list variable_mappings,
                                        int_list_list constant_mappings,
                                        const unsigned int current_map)
{
    return concat_tv_wff_expr_list(left,
                                   map_tv_wff(right,
                                              variable_mappings,
                                              constant_mappings,
                                              current_map));

}

static void detect_constants(const_tv_term_list terms, int *pos_constants, int *neg_constants, variable_list variables)
{
    unsigned int ix, iy;
    unsigned int *pos_counters, *neg_counters;

    if (0 == terms->sz) {
        return;
    }

    pos_counters = (unsigned int *)malloc(sizeof(unsigned int) * variables->sz);
    neg_counters = (unsigned int *)malloc(sizeof(unsigned int) * variables->sz);

    for (ix = 0; ix < variables->sz; ix++) {
        pos_counters[ix] = 0;
        neg_counters[ix] = 0;
        pos_constants[ix] = 0;
        neg_constants[ix] = 0;
    }

    for (ix = 0; ix < terms->sz; ix++) {
        const_int_list pos = terms->arr[ix]->pos;
        const_int_list neg = terms->arr[ix]->neg;
        for (iy = 0; iy < pos->sz; iy++) {
            pos_counters[pos->arr[iy]] += 1;
        }
        for (iy = 0; iy < neg->sz; iy++) {
            neg_counters[neg->arr[iy]] += 1;
        }
    }

    for (ix = 0; ix < variables->sz; ix++) {
        if (pos_counters[ix] == terms->sz) {
            pos_constants[ix] = 1;
        }
        if (neg_counters[ix] == terms->sz) {
            neg_constants[ix] = 1;
        }
    }    

    free(pos_counters);
    free(neg_counters);
}

static tv_clause_list map_tv_cnf(tv_clause_list clauses,
                                 int_list_list variable_mappings,
                                 int_list_list UNUSED(constant_mappings),
                                 const unsigned int current_map)
{
    register unsigned int ix, iy;

    tv_clause_list result = rdup_tv_clause_list(clauses);

/* Now remap all the variables in clauses to have global numbering. */
    int_list map = variable_mappings->arr[current_map];
    for (ix = 0; ix < result->sz; ix++) {
        int_list pos = result->arr[ix]->pos;
        int_list neg = result->arr[ix]->neg;
        for (iy = 0; iy < pos->sz; iy++) {
            pos->arr[iy] = map->arr[pos->arr[iy]];
        }
        for (iy = 0; iy < neg->sz; iy++) {
            neg->arr[iy] = map->arr[neg->arr[iy]];
        }
        sort_int_list(pos);
        sort_int_list(neg);
    }

    return result;
}

static tv_term_list map_tv_dnf(tv_term_list terms,
                               int_list_list variable_mappings,
                               int_list_list UNUSED(constant_mappings),
                               const unsigned int current_map)
{
    register unsigned int ix, iy;

    tv_term_list result = rdup_tv_term_list(terms);

/* Now remap all the variables in terms to have global numbering. */
    int_list map = variable_mappings->arr[current_map];
    for (ix = 0; ix < result->sz; ix++) {
        int_list pos = result->arr[ix]->pos;
        int_list neg = result->arr[ix]->neg;
        for (iy = 0; iy < pos->sz; iy++) {
            pos->arr[iy] = map->arr[pos->arr[iy]];
        }
        for (iy = 0; iy < neg->sz; iy++) {
            neg->arr[iy] = map->arr[neg->arr[iy]];
        }
        sort_int_list(pos);
        sort_int_list(neg);
    }

    return result;
}

tv_term_list multiply_tv_dnf(tv_term_list left,
                             tv_term_list right,
                             variable_list variables,
                             int_list_list variable_mappings,
                             int_list_list constant_mappings,
                             const unsigned int current_map)
{
    unsigned int ix, iy;

    tv_term_list l = rdup_tv_term_list(left);
    tv_term_list r = map_tv_dnf(right, variable_mappings, constant_mappings, current_map);
    tv_term_list res = new_tv_term_list();

    int *l_pos_constants, *l_neg_constants;
    int *r_pos_constants, *r_neg_constants;
    int unsat = 0;

    l_pos_constants = (int *)malloc(variables->sz * sizeof(int));
    l_neg_constants = (int *)malloc(variables->sz * sizeof(int));
    r_pos_constants = (int *)malloc(variables->sz * sizeof(int));
    r_neg_constants = (int *)malloc(variables->sz * sizeof(int));

    memset(l_pos_constants, 0, variables->sz * sizeof(int));
    memset(r_pos_constants, 0, variables->sz * sizeof(int));
    memset(l_neg_constants, 0, variables->sz * sizeof(int));
    memset(r_neg_constants, 0, variables->sz * sizeof(int));

    detect_constants(l, l_pos_constants, l_neg_constants, variables);
    detect_constants(r, r_pos_constants, r_neg_constants, variables);

    for (ix = 0; ix < variables->sz; ix++) {
        if ((l_pos_constants[ix] && r_neg_constants[ix]) ||
            (l_neg_constants[ix] && r_pos_constants[ix])) {
            unsat = 1;
            break;
        }
    }

    free(l_pos_constants);
    free(l_neg_constants);
    free(r_pos_constants);
    free(r_neg_constants);

    if (unsat) {
        return res;
    }

    if (0 == r->sz || 0 == l->sz) {
        rfre_tv_term_list(l);
        rfre_tv_term_list(r);
        return res;
    }

    for (ix = 0; ix < l->sz; ix++) {
        for (iy = 0; iy < r->sz; iy++) {
            int_list pos, neg;

            tv_term l_sol = l->arr[ix];
            tv_term r_sol = r->arr[iy];
            if (!is_sorted_term_consistent(l_sol->pos, r_sol->neg) ||
                !is_sorted_term_consistent(l_sol->neg, r_sol->pos)) {
                continue;
            }

            pos = merge_sorted_int_list(l_sol->pos, r_sol->pos);
            neg = merge_sorted_int_list(l_sol->neg, r_sol->neg);
#if 0
            {
                register unsigned int ia, ib = 0;
                for (ia = 0; ia < neg->sz; ia++) {
                    if (is_health(variables->arr[neg->arr[ia]])) {
                        ib += 1;
                    }
                }
                if (ib > 0) {
                    rfre_int_list(neg);
                    rfre_int_list(pos);
                    continue;
                }
            }
#endif
            append_tv_term_list(res, new_tv_term(pos, neg));
        }
    }

    rfre_tv_term_list(l);
    rfre_tv_term_list(r);

    return res;
}

tv_clause_list multiply_tv_cnf(tv_clause_list left,
                               tv_clause_list right,
                               int_list_list variable_mappings,
                               int_list_list constant_mappings,
                               const unsigned int current_map)
{
    return concat_tv_clause_list(left,
                                 map_tv_cnf(right,
                                            variable_mappings,
                                            constant_mappings,
                                            current_map));
}

static void inline_nodes(hierarchy input,
                         node root,
                         kb result,
                         int_list_list variable_mappings,
                         int_list_list constant_mappings,
                         unsigned int *current_map)
{
    register unsigned int ix;

    switch (input->tag) {
        case TAGmv_wff_hierarchy:
/* To Do: Implement. */
            break;
        case TAGtv_cnf_hierarchy:
            to_tv_cnf(result)->clauses = multiply_tv_cnf(to_tv_cnf(result)->clauses,
                                                         to_tv_cnf(root->constraints)->clauses,
                                                         variable_mappings,
                                                         constant_mappings,
                                                         *current_map);
            if (option_simplify) {
                to_tv_cnf(result)->clauses = (void *)simplify_literal_sets((void *)to_tv_cnf(result)->clauses);
            }
            break;
        case TAGtv_dnf_hierarchy:
            to_tv_dnf(result)->terms = multiply_tv_dnf(to_tv_dnf(result)->terms,
                                                       to_tv_dnf(root->constraints)->terms,
                                                       result->variables,
                                                       variable_mappings,
                                                       constant_mappings,
                                                       *current_map);
            if (option_simplify) {
                assert(to_tv_dnf(result)->terms != tv_term_listNIL);

                to_tv_dnf(result)->terms = (void *)simplify_literal_sets((void *)to_tv_dnf(result)->terms);
            }
            break;
        case TAGtv_wff_hierarchy:
            to_tv_wff(result)->e = multiply_tv_wff(to_tv_wff(result)->e,
                                                   to_tv_wff(root->constraints)->e,
                                                   variable_mappings,
                                                   constant_mappings,
                                                   *current_map);
            break;
        case TAGcsp_hierarchy:
            to_csp(result)->sentences = multiply_csp(to_csp(result)->sentences,
                                                     to_csp(root->constraints)->sentences,
                                                     variable_mappings,
                                                     constant_mappings,
                                                     *current_map);
            break;
        case TAGmv_cnf_hierarchy:
        case TAGmv_dnf_hierarchy:
        default:
            assert(0);
    }

    *current_map += 1;

    for (ix = 0; ix < root->edges->sz; ix++) {
        node kid;
        if (nodeNIL == (kid = find_node(input, root->edges->arr[ix]->type))) {
            assert(0);
            break;
        }
/*
        if (option_verbose) {
            fprintf(stderr, "Inlining node '%s'...\n", root->edges->arr[ix]->name->name);
        }
*/
        inline_nodes(input, kid, result, variable_mappings, constant_mappings, current_map);
    }
}

static flat_kb inline_hierarchy(hierarchy input, node root)
{
    int_list_list variable_mappings;
    int_list_list constant_mappings;

    unsigned int current_map = 0;

    flat_kb result = flat_kbNIL;

    if (nodeNIL == root) {
        return flat_kbNIL;
    }

    switch (input->tag) {
        case TAGtv_cnf_hierarchy:
            result = to_flat_kb(new_tv_cnf_flat_kb(rdup_lydia_symbol(root->type),
                                                   to_kb(new_tv_cnf(new_values_set_list(),
                                                                    new_variable_list(),
                                                                    new_variable_list(),
                                                                    new_constant_list(),
                                                                    ENCODING_NONE,
                                                                    new_tv_clause_list()))));
            break;
        case TAGtv_dnf_hierarchy:
            result = to_flat_kb(new_tv_dnf_flat_kb(rdup_lydia_symbol(root->type),
                                                   to_kb(new_tv_dnf(new_values_set_list(),
                                                                    new_variable_list(),
                                                                    new_variable_list(),
                                                                    new_constant_list(),
                                                                    ENCODING_NONE,
                                                                    append_tv_term_list(new_tv_term_list(), new_tv_term(new_int_list(), new_int_list()))))));
            break;
        case TAGtv_wff_hierarchy:
            result = to_flat_kb(new_tv_wff_flat_kb(rdup_lydia_symbol(root->type),
                                                   to_kb(new_tv_wff(new_values_set_list(),
                                                                    new_variable_list(),
                                                                    new_variable_list(),
                                                                    new_constant_list(),
                                                                    ENCODING_NONE,
                                                                    new_tv_wff_expr_list()))));
            break;
        case TAGcsp_hierarchy:
            result = to_flat_kb(new_csp_flat_kb(rdup_lydia_symbol(root->type),
                                                to_kb(new_csp(new_values_set_list(),
                                                              new_variable_list(),
                                                              new_variable_list(),
                                                              new_constant_list(),
                                                              ENCODING_NONE,
                                                              new_csp_sentence_list()))));
            break;
        case TAGmv_wff_hierarchy:
        case TAGmv_cnf_hierarchy:
        case TAGmv_dnf_hierarchy:
        default:
            assert(0);
            abort();
    }

    variable_mappings = new_int_list_list();
    constant_mappings = new_int_list_list();

    inline_variables(input,
                     root,
                     mapping_listNIL,
                     result->constraints->domains,
                     result->constraints->variables,
                     result->constraints->encoded_variables,
                     result->constraints->constants,
                     variable_mappings,
                     constant_mappings,
                     qualifier_listNIL);
    result->constraints->encoding = root->constraints->encoding;
/*
    if (option_verbose) {
        fprintf(stderr, "Inlining the root node...\n");
    }
*/
    inline_nodes(input,
                 root,
                 result->constraints,
                 variable_mappings,
                 constant_mappings,
                 &current_map);

    rfre_int_list_list(variable_mappings);
    rfre_int_list_list(constant_mappings);

    return result;
}

static void partially_inline_hierarchy(hierarchy input,
                                       node root,
                                       unsigned int level,
                                       unsigned int current_level)
{
    unsigned int ix;
    if (current_level == level) {
        flat_kb node = inline_hierarchy(input, root);
        rfre_edge_list(root->edges);
        rfre_kb(root->constraints);
        root->edges = new_edge_list();
        root->constraints = node->constraints;
        fre_flat_kb(node);
    }
    for (ix = 0; ix < root->edges->sz; ix++) {
        node kid;
        if (nodeNIL == (kid = find_node(input, root->edges->arr[ix]->type))) {
            assert(0);
            break;
        }
        partially_inline_hierarchy(input, kid, level, current_level + 1);
    }
}

void mark_used(hierarchy input, node root, int_list result)
{
    register unsigned int ix;
    unsigned int pos;

    if (!search_node_list(input->nodes, root->type, &pos)) {
        return;
    }
    if (!member_int_list(result, pos)) {
        append_int_list(result, pos);
    }

    for (ix = 0; ix < root->edges->sz; ix++) {
        node kid;
        if (nodeNIL == (kid = find_node(input, root->edges->arr[ix]->type))) {
            assert(0);
            break;
        }
        mark_used(input, kid, result);
    }
}

int smoothy(const_serializable input,
            int hier,
            unsigned int depth,
            const int verbose,
            const int simplify,
            serializable *output)
{
    register unsigned int ix;

    node root = nodeNIL;

    start_stopwatch("inlining time");

    option_verbose = verbose;
    option_simplify = simplify;

    switch (input->tag) {
        case TAGcsp_hierarchy:
        case TAGtv_wff_hierarchy:
        case TAGtv_cnf_hierarchy:
        case TAGtv_dnf_hierarchy:
            if (hier) {
                int_list used;

                *output = rdup_serializable(input);
                root = find_root_node(to_hierarchy(*output));
                partially_inline_hierarchy(to_hierarchy(*output),
                                           root,
                                           depth,
                                           0);
                used = new_int_list();
                mark_used(to_hierarchy(*output), root, used);
                for (ix = to_hierarchy(*output)->nodes->sz - 1;
                     ix < to_hierarchy(*output)->nodes->sz;
                     ix--) {
                    if (!member_int_list(used, ix)) {
                        delete_node_list(to_hierarchy(*output)->nodes, ix);
                    }
                }
                rfre_int_list(used);
                break;
            }
            if (nodeNIL != (root = find_root_node(to_hierarchy(input)))) {
                *output = to_serializable(inline_hierarchy(to_hierarchy(input), root));
            }
            break;
        case TAGcsp_flat_kb:
        case TAGtv_nnf_flat_kb:
        case TAGtv_wff_flat_kb:
        case TAGtv_cnf_flat_kb:
        case TAGtv_dnf_flat_kb:
        case TAGobdd_flat_kb:
        case TAGmv_wff_flat_kb:
        case TAGmv_cnf_flat_kb:
        case TAGmv_dnf_flat_kb:
        case TAGmdd_flat_kb:
            /* Already flattened. */
            *output = rdup_serializable(input);
            break;
        default:
            fprintf(stderr, "smoothy: not implemented\n");
            return 0;
    }

    stop_stopwatch("inlining time");

    return 1;
}

void smoothy_init()
{
    stat_init();
    init_stopwatch("inlining time", "inlining time: %d s %d.%d ms", "compilation");
}

void smoothy_destroy()
{
    stat_destroy();
}
