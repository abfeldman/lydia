#include "config.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "tv.h"
#include "defs.h"
#include "util.h"
#include "obdd.h"
#include "obdd2dnf.h"
#include "variable.h"
#include "hierarchy.h"
#include "sorted_int_list.h"

tv_term_list all_sat(const_obdd_node_list nodes, const unsigned int node_index)
{
    obdd_non_terminal_node node;
    tv_term_list low, high;

    register unsigned int ix;

/* To Do: It seems to me that the trivial cases are not handled correctly. */
    if (nodes->arr[node_index]->tag == TAGobdd_terminal_node &&
	LYDIA_FALSE == to_obdd_terminal_node(nodes->arr[node_index])->value) {
	return new_tv_term_list();
    }
    if (nodes->arr[node_index]->tag == TAGobdd_terminal_node &&
	LYDIA_TRUE == to_obdd_terminal_node(nodes->arr[node_index])->value) {
	return append_tv_term_list(new_tv_term_list(), new_tv_term(new_int_list(), new_int_list()));
    }
    node = to_obdd_non_terminal_node(nodes->arr[node_index]);
    low = all_sat(nodes, node->low);
    high = all_sat(nodes, node->high);

    for (ix = 0; ix < low->sz; ix++) {
	append_int_list(low->arr[ix]->neg, node->var);
    }
    for (ix = 0; ix < high->sz; ix++) {
	append_int_list(high->arr[ix]->pos, node->var);
    }

    return concat_tv_term_list(low, high);
}

static tv_term_list obdd_sort_terms(tv_term_list terms)
{
    register unsigned int ix;
    for (ix = 0; ix < terms->sz; ix++) {
	sort_int_list(terms->arr[ix]->pos);
	sort_int_list(terms->arr[ix]->neg);
    }
    return terms;
}

int obdd2dnf(const_serializable input, serializable *output)
{
    register unsigned int ix;
    if (input->tag == TAGobdd_flat_kb) {
        obdd input_obdd = to_obdd(to_obdd_flat_kb(input)->constraints);
	*output = to_serializable(new_tv_dnf_flat_kb(to_obdd_flat_kb(input)->name,
                                                     to_kb(new_tv_dnf(rdup_values_set_list(input_obdd->domains),
                                                                      rdup_variable_list(input_obdd->variables),
                                                                      rdup_variable_list(input_obdd->encoded_variables),
                                                                      rdup_constant_list(input_obdd->constants),
                                                                      input_obdd->encoding,
                                                                      obdd_sort_terms(all_sat(input_obdd->nodes,
                                                                                              input_obdd->nodes->sz - 1))))));
    } else if (input->tag == TAGobdd_hierarchy) {
	node_list nodes = new_node_list();
	*output = to_serializable(new_tv_dnf_hierarchy(nodes));
	for (ix = 0; ix < to_hierarchy(input)->nodes->sz; ix++) {
	    obdd node_input = to_obdd(to_hierarchy(input)->nodes->arr[ix]->constraints);
	    tv_dnf node_output = new_tv_dnf(rdup_values_set_list(node_input->domains),
					    rdup_variable_list(node_input->variables),
					    rdup_variable_list(node_input->encoded_variables),
					    rdup_constant_list(node_input->constants),
					    node_input->encoding,
					    obdd_sort_terms(all_sat(node_input->nodes,
                                                                    node_input->nodes->sz - 1)));
	    node result = new_node(rdup_lydia_symbol(to_hierarchy(input)->nodes->arr[ix]->type),
				   rdup_edge_list(to_hierarchy(input)->nodes->arr[ix]->edges),
				   to_kb(node_output));
	    append_node_list(nodes, result);
	}
    } else {
	assert(0);
    }
    return 1;
}

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
