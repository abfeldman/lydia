/**
 *  \file inline.c
 *  \brief Variable flattening library. Flattening is the process of
 *         transforming a hierarchical representation to a flat one.
 *         This library contains routines for preparing a flat list of the
 *         variables, and the respective maps for reindexing the variables
 *         in the different knowledge bases (Wff, CNF, DNF, etc.).
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hierarchy.h"
#include "inline.h"
#include "util.h"

static int search_identifier_list(const_identifier_list haystack, identifier needle, unsigned int *pos)
{
    unsigned int i;

    for (i = 0; i < haystack->sz; i++) {
        if (isequal_identifier(needle, haystack->arr[i])) {
            *pos = i;
            return 1;
        }
    }
    return 0;
}

static void rename_edges(edge_list edges, identifier_list from, identifier_list to)
{
    unsigned int ix, iy, pos;

    if (edge_listNIL == edges) {
        return;
    }

    for (ix = 0; ix < edges->sz; ix++) {
        edge edge = edges->arr[ix];
        for (iy = 0; iy < edge->bindings->sz; iy++) {
            if (search_identifier_list(from, edge->bindings->arr[iy]->from, &pos)) {
                rfre_identifier(edge->bindings->arr[iy]->from);
                edge->bindings->arr[iy]->from = rdup_identifier(to->arr[pos]);
            }
        }
    }
}

static int is_formal(mapping_list bindings, identifier id, unsigned int *pos)
{
    unsigned int ix;
    if (mapping_listNIL == bindings) {
        return 0;
    }
    for (ix = 0; ix < bindings->sz; ix++) {
        if (isequal_identifier(bindings->arr[ix]->to, id)) {
            if (pos != NULL) {
                *pos = ix;
            }
            return 1;
        }
    }
    return 0;
}

static void rename_variable(identifier_list edges_from,
                            identifier_list edges_to,
                            variable var,
                            variable_list vars,
                            mapping_list bindings,
                            int_list edge_mappings,
                            qualifier_list path)
{
    unsigned int pos;

    if (is_formal(bindings, var->name, &pos)) {
        append_identifier_list(edges_from, rdup_identifier(var->name));
        append_identifier_list(edges_to, rdup_identifier(bindings->arr[pos]->from));
        rfre_identifier(var->name);
        var->name = rdup_identifier(bindings->arr[pos]->from);

        if (!search_variable_list(vars, var->name, &pos)) {
            assert(0);
            abort();
        }
        append_int_list(edge_mappings, pos);
        rfre_variable(var);
    } else {
        if (qualifier_listNIL != path) {
            identifier new_name = new_identifier(rdup_lydia_symbol(var->name->name),
                                                 rdup_int_list(var->name->indices),
                                                 var->name->qualifiers == qualifier_listNIL ? rdup_qualifier_list(path) : concat_qualifier_list(rdup_qualifier_list(path), rdup_qualifier_list(var->name->qualifiers)));
            append_identifier_list(edges_from, rdup_identifier(var->name));
            append_identifier_list(edges_to, rdup_identifier(new_name));
            rfre_identifier(var->name);
            var->name = new_name;
        }
        append_int_list(edge_mappings, vars->sz);
        append_variable_list(vars, var);
    }
}

/**
 * Given a hierarchy, builds flat lists of variables and constants and
 * their respective mappings. The variable and constant domains are
 * flattened as well if such exist.
 *
 * @param hier the input multidigraph;
 * @param root the current node, initially the root node;
 * @param bindings a list of variable mappings from the parent node to the
 *        current node, #mapping_listNIL for the root node as it has no
 *        parents;
 * @param domains a result list for the variable domains, initially an empty
 *                list. If there are no domains (e.g. in a Boolean model)
 *                a value of #values_set_listNIL can be specified;
 * @param variables an output list for the model variables, initially an
 *                  empty list;
 * @param encoded_variables an output list for the encoded model variables,
 *                          initially an empty list;
 * @param constants an output list for the model constants, initially an 
 *                  empty list. If there are no constants, #constant_listNIL
 *                  can be specified;
 * @param variable_mappings a two-dimensional array for the variable indexes
 *                          in each node. For each node in depth-first order
 *                          we have an integer array with length equal the
 *                          number of variables in the respective node.
 *                          The values specify the variable offset in the
 *                          variables list;
 * @param constant_mappings similar to variable_mappings, but for constants;
 * @param path an internal parameter used by the recursion, initially #qualifier_listNIL.
 */
int inline_variables(const_hierarchy hier,
                     const_node root,
                     mapping_list bindings,
                     values_set_list domains,
                     variable_list variables,
                     variable_list encoded_variables,
                     constant_list constants,
                     int_list_list variable_mappings,
                     int_list_list constant_mappings,
                     qualifier_list path)
{
    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;

    unsigned int pos;

    variable_attribute attr;

    int_list node_domain_mappings = int_listNIL;
    int_list node_encoded_variable_mappings = int_listNIL;
    int_list node_variable_mappings = int_listNIL;
    int_list node_constant_mappings = int_listNIL;

    int_list indices;

    edge_list edges = rdup_edge_list(root->edges);

    identifier_list edges_from;
    identifier_list edges_to;
    qualifier_list new_path;
    
    qualifier qual;

    values_set domain;
    variable var;
    constant con;

    node kid;

    if (edge_listNIL == edges) {
        return 0;
    }

    node_domain_mappings = new_int_list();
    if (variable_listNIL != encoded_variables) {
        if (int_listNIL == (node_encoded_variable_mappings = new_int_list())) {
            goto error;
        }
    }
    if (variable_listNIL != variables) {
        if (int_listNIL == (node_variable_mappings = new_int_list())) {
            goto error;
        }
    }
    if (constant_listNIL != constants) {
        if (int_listNIL == (node_constant_mappings = new_int_list())) {
            goto error;
        }
    }

/* Inline the domains. */
    if (values_set_listNIL != root->constraints->domains) {
        for (ix = 0; ix < root->constraints->domains->sz; ix++) {
            if (search_values_set_list(domains, root->constraints->domains->arr[ix]->name, &pos)) {
                if (!append_int_list(node_domain_mappings, pos)) {
                    goto error;
                }
                continue;
            }
            if (NULL == (domain = rdup_values_set(root->constraints->domains->arr[ix]))) {
                goto error;
            }
            if (!append_values_set_list(domains, domain)) {
                rfre_values_set(domain);
                goto error;
            }
            if (!append_int_list(node_domain_mappings, domains->sz - 1)) {
                goto error;
            }
        }
    }
/* First the encoded variables. */
    if (variable_listNIL != encoded_variables && variable_listNIL != root->constraints->encoded_variables) {
        if (identifier_listNIL == (edges_from = new_identifier_list())) {
            goto error;
        }
        if (identifier_listNIL == (edges_to = new_identifier_list())) {
            rfre_identifier_list(edges_from);
            goto error;
        }
        pos = encoded_variables->sz;
        for (ix = 0; ix < root->constraints->encoded_variables->sz; ix++) {
            if (variableNIL == (var = rdup_variable(root->constraints->encoded_variables->arr[ix]))) {
                rfre_identifier_list(edges_to);
                rfre_identifier_list(edges_from);
                goto error;
            }
            if (TAGenum_variable == var->tag) {
                to_enum_variable(var)->values_set = node_domain_mappings->arr[to_enum_variable(var)->values_set];
            }
            rename_variable(edges_from, edges_to, var, encoded_variables, bindings, node_encoded_variable_mappings, path);
        }
        rename_edges(edges, edges_from, edges_to);
        rfre_identifier_list(edges_from);
        rfre_identifier_list(edges_to);
    }
    if (variable_listNIL != variables && variable_listNIL != root->constraints->variables) {
        if (identifier_listNIL == (edges_from = new_identifier_list())) {
            goto error;
        }
        if (identifier_listNIL == (edges_to = new_identifier_list())) {
            rfre_identifier_list(edges_from);
            goto error;
        }
        pos = variables->sz;
        for (ix = 0; ix < root->constraints->variables->sz; ix++) {
            if (variableNIL == (var = rdup_variable(root->constraints->variables->arr[ix]))) {
                rfre_identifier_list(edges_to);
                rfre_identifier_list(edges_from);
                goto error;
            }
            if (TAGenum_variable == var->tag) {
                to_enum_variable(var)->values_set = node_domain_mappings->arr[to_enum_variable(var)->values_set];
            }
            if (TAGbool_variable == var->tag && to_bool_variable(var)->encoded_variable != -1) {
                to_bool_variable(var)->encoded_variable = node_encoded_variable_mappings->arr[to_bool_variable(var)->encoded_variable];
            }
            rename_variable(edges_from, edges_to, var, variables, bindings, node_variable_mappings, path);
        }
        rename_edges(edges, edges_from, edges_to);
        rfre_identifier_list(edges_from);
        rfre_identifier_list(edges_to);
        for (ix = pos; ix < variables->sz; ix++) {
            var = variables->arr[ix];
            if (NULL == var->attributes) {
                continue;
            }
        }
    }
    if (constant_listNIL != constants && constant_listNIL != root->constraints->constants) {
        for (ix = 0; ix < root->constraints->constants->sz; ix++) {
            if (constantNIL == (con = rdup_constant(root->constraints->constants->arr[ix]))) {
                goto error;
            }
            if (con->tag == TAGenum_constant) {
                to_enum_constant(con)->values_set = node_domain_mappings->arr[to_enum_constant(con)->values_set];
            }
            if (!append_constant_list(constants, con)) {
                rfre_constant(con);
                goto error;
            }
            if (!append_int_list(node_constant_mappings, constants->sz - 1)) {
                goto error;
            }
        }
    }

    if (int_list_listNIL != constant_mappings) {
        if (!append_int_list_list(constant_mappings, node_constant_mappings)) {
            goto error;
        }
    }
    if (int_list_listNIL != variable_mappings) {
        if (!append_int_list_list(variable_mappings, node_variable_mappings)) {
            goto error;
        }
    }
    rfre_int_list(node_encoded_variable_mappings);
    rfre_int_list(node_domain_mappings);

    for (ix = 0; ix < edges->sz; ix++) {
        if (nodeNIL == (kid = find_node(hier, edges->arr[ix]->type))) {
            assert(0);
            break;
        }
        if (path == qualifier_listNIL) {
            new_path = new_qualifier_list();
        } else {
            new_path = rdup_qualifier_list(path);
        }
        if (qualifier_listNIL == new_path) {
            rfre_edge_list(edges);
            return 0;
        }
        if (int_listNIL == edges->arr[ix]->indices) {
            indices = int_listNIL;
        } else {
            if (int_listNIL == (indices = rdup_int_list(edges->arr[ix]->indices))) {
                rfre_qualifier_list(new_path);
                rfre_edge_list(edges);
                return 0;       
            }     
        }
        if (qualifierNIL == (qual = new_qualifier(edges->arr[ix]->name, indices))) {
            rfre_int_list(indices);
            rfre_edge_list(edges);
            return 0;            
        }
        if (!append_qualifier_list(new_path, qual)) {
            rfre_qualifier(qual);
            rfre_edge_list(edges);
            return 0;            
        }
        if (!inline_variables(hier,
                              kid,
                              edges->arr[ix]->bindings,
                              domains,
                              variables,
                              encoded_variables,
                              constants,
                              variable_mappings,
                              constant_mappings,
                              new_path)) {
            rfre_qualifier_list(new_path);
            rfre_edge_list(edges);
            return 0;
        }
        rfre_qualifier_list(new_path);
    }

    rfre_edge_list(edges);

    return 1;

error:
    if (edge_listNIL != edges) {
        rfre_edge_list(edges);
    }
    if (int_listNIL != node_domain_mappings) {
        rfre_int_list(node_domain_mappings);
    }
    if (int_listNIL != node_encoded_variable_mappings) {
        rfre_int_list(node_encoded_variable_mappings);
    }
    if (int_listNIL != node_variable_mappings) {
        rfre_int_list(node_variable_mappings);
    }
    if (int_listNIL != node_constant_mappings) {
        rfre_int_list(node_constant_mappings);
    }

    return 0;
}

/**
 * Given two nodes, which have to be a parent and a child node,
 * inlines the variables of the child node in the parent one. The
 * variable names in the edges of the child are also renamed, so they
 * can be moved to the parent. The variables from the child node are
 * appended to the variables in the parent node.
 *
 * @param node the parent node;
 * @param child the child node;
 * @param edge the edge connecting the parent and child nodes;
 * @param variable_mappings an output list for two integer lists where the
 *        variable mappings for the parent and child nodes will go. Must
 *        be created and empty;
 * @param constant_mappings an output list for two integer lists where the
 *        constant mappings for the parent and child nodes will go. Must
 *        be created and empty.
 */
int combine_parent_child_variables(node parent,
                                   const_node child,
                                   const_edge edge,
                                   int_list_list variable_mappings,
                                   int_list_list constant_mappings)
{
    register unsigned int ix;

    values_set_list domains = parent->constraints->domains;
    variable_list variables = parent->constraints->variables;
    constant_list constants = parent->constraints->constants;

    int_list domain_mappings;
    qualifier_list path;

    append_int_list_list(append_int_list_list(variable_mappings, new_int_list()), new_int_list());
    append_int_list_list(append_int_list_list(constant_mappings, new_int_list()), new_int_list());

    domain_mappings = new_int_list();

    path = append_qualifier_list(new_qualifier_list(),
                                 new_qualifier(rdup_lydia_symbol(edge->name),
                                               rdup_int_list(edge->indices)));

    domain_mappings = new_int_list();

    if (values_set_listNIL != domains) {
        for (ix = 0; ix < domains->sz; ix++) {
            append_int_list(domain_mappings, ix);
        }
    }
    if (values_set_listNIL != child->constraints->domains) {
        for (ix = 0; ix < child->constraints->domains->sz; ix++) {
            unsigned int pos;
            if (search_values_set_list(domains,
                                       child->constraints->domains->arr[ix]->name,
                                       &pos)) {
                append_int_list(domain_mappings, pos);
                continue;
            }
            append_values_set_list(domains, rdup_values_set(child->constraints->domains->arr[ix]));
            append_int_list(domain_mappings, domains->sz - 1);
        }
    }
    if (variable_listNIL != variables) {
        identifier_list edges_from, edges_to;

        for (ix = 0; ix < variables->sz; ix++) {
            append_int_list(variable_mappings->arr[0], ix);
        }

        edges_from = new_identifier_list();
        edges_to = new_identifier_list();
        for (ix = 0; ix < child->constraints->variables->sz; ix++) {
            variable var = rdup_variable(child->constraints->variables->arr[ix]);
            if (TAGenum_variable == var->tag) {
                to_enum_variable(var)->values_set = domain_mappings->arr[to_enum_variable(var)->values_set];
            }
            rename_variable(edges_from,
                            edges_to,
                            var,
                            variables,
                            edge->bindings,
                            variable_mappings->arr[1],
                            path);
        }
        rename_edges(child->edges, edges_from, edges_to);
        rfre_identifier_list(edges_from);
        rfre_identifier_list(edges_to);
    }
    if (constant_listNIL != constants) {
        for (ix = 0; ix < constants->sz; ix++) {
            append_int_list(constant_mappings->arr[0], ix);
        }
        for (ix = 0; ix < child->constraints->constants->sz; ix++) {
            constant con = rdup_constant(child->constraints->constants->arr[ix]);
            if (con->tag == TAGenum_constant) {
                to_enum_constant(con)->values_set = domain_mappings->arr[to_enum_constant(con)->values_set];
            }
            append_int_list(constant_mappings->arr[1], constants->sz);
            append_constant_list(constants, con);
        }
    }
    rfre_int_list(domain_mappings);

    return 1;
}
