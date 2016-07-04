#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "sorted_int_list.h"
#include "serializable.h"
#include "hierarchy.h"
#include "flat_kb.h"
#include "inline.h"
#include "util.h"
#include "stat.h"
#include "defs.h"
#include "lcm.h"
#include "mv.h"

#include "flatten_mvwff.h"
#include "flatten_mvcnf.h"
#include "flatten_mvdnf.h"

static int option_simplify;
static int option_verbose;

static void inline_nodes(const_hierarchy input,
                         const_node root,
                         kb result,
                         int_list_list variable_mappings,
                         int_list_list constant_mappings,
                         unsigned int *current_map)
{
    register unsigned int ix;

    switch (input->tag) {
        case TAGmv_cnf_hierarchy:
            to_mv_cnf(result)->clauses = 
                multiply_mv_cnf(to_mv_cnf(result)->clauses,
                                to_mv_cnf(root->constraints)->clauses,
                                variable_mappings,
                                *current_map);
            break;
        case TAGmv_dnf_hierarchy:
            to_mv_dnf(result)->terms = 
                multiply_mv_dnf(to_mv_dnf(result)->terms,
                                to_mv_dnf(root->constraints)->terms,
                                variable_mappings,
                                *current_map);
            break;
        case TAGmv_wff_hierarchy:
            to_mv_wff(result)->e = 
                multiply_mv_wff(to_mv_wff(result)->e,
                                to_mv_wff(root->constraints)->e,
                                variable_mappings,
                                *current_map);
            break;
        case TAGtv_cnf_hierarchy:
        case TAGtv_dnf_hierarchy:
        case TAGtv_wff_hierarchy:
        case TAGcsp_hierarchy:
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
        inline_nodes(input, 
                     kid, 
                     result, 
                     variable_mappings, 
                     constant_mappings, 
                     current_map);
    }
}

static flat_kb inline_hierarchy(const_hierarchy input, const_node root)
{

    int_list_list variable_mappings;
    int_list_list constant_mappings;

    unsigned int current_map;

    flat_kb result = flat_kbNIL;

    if (nodeNIL == root) {
        return flat_kbNIL;
    }

    switch (input->tag) {
        case TAGmv_cnf_hierarchy:
            result = to_flat_kb(new_mv_cnf_flat_kb(rdup_lydia_symbol(root->type),
                                                   to_kb(new_mv_cnf(new_values_set_list(),
                                                                    new_variable_list(),
                                                                    new_variable_list(),
                                                                    new_constant_list(),
                                                                    ENCODING_NONE,
                                                                    new_mv_clause_list()))));
            break;
        case TAGmv_dnf_hierarchy:
            result = to_flat_kb(new_mv_dnf_flat_kb(rdup_lydia_symbol(root->type),
                                                   to_kb(new_mv_dnf(new_values_set_list(),
                                                                    new_variable_list(),
                                                                    new_variable_list(),
                                                                    new_constant_list(),
                                                                    ENCODING_NONE,
                                                                    append_mv_term_list(new_mv_term_list(), new_mv_term(new_mv_literal_list(), new_mv_literal_list()))))));
            break;
        case TAGmv_wff_hierarchy:
            result = to_flat_kb(new_mv_wff_flat_kb(rdup_lydia_symbol(root->type),
                                                   to_kb(new_mv_wff(new_values_set_list(),
                                                                    new_variable_list(),
                                                                    new_variable_list(),
                                                                    new_constant_list(),
                                                                    ENCODING_NONE,
                                                                    new_mv_wff_expr_list()))));
            break;
        case TAGtv_cnf_hierarchy:
        case TAGtv_dnf_hierarchy:
        case TAGtv_wff_hierarchy:
        case TAGcsp_hierarchy:
        default:
            assert(0);
            abort();
    }

    variable_mappings = new_int_list_list();
    constant_mappings = new_int_list_list();

    current_map = 0;

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
    node root = nodeNIL;

    int_list used;

    register unsigned int ix;

    start_stopwatch("inlining time");

    option_verbose = verbose;
    option_simplify = simplify;

    switch (input->tag) {
        case TAGmv_wff_hierarchy:
        case TAGmv_cnf_hierarchy:
        case TAGmv_dnf_hierarchy:
            if (hier) {
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
                *output = to_serializable(inline_hierarchy(to_hierarchy(input),
                                                           root));
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
            fprintf(stderr, "mvsmoothy: not implemented\n");
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
