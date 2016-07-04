#include "tv.h"
#include "lsss.h"
#include "stat.h"
#include "util.h"
#include "flat_kb.h"
#include "cnf2dnf.h"
#include "variable.h"
#include "hierarchy.h"
#include "serializable.h"

#include <assert.h>

int cnf2dnf(const_serializable input, serializable *output)
{
    register unsigned int ix;

    start_stopwatch("compilation time");
    if (input->tag == TAGtv_cnf_flat_kb) {
	tv_cnf cnf = to_tv_cnf(to_tv_cnf_flat_kb(input)->constraints);
	tv_term_list terms = (tv_clause_listNIL == cnf->clauses ? tv_term_listNIL : new_tv_term_list());
	*output = to_serializable(new_tv_dnf_flat_kb(to_tv_cnf_flat_kb(input)->name,
                                                     to_kb(new_tv_dnf(rdup_values_set_list(cnf->domains),
                                                                      rdup_variable_list(cnf->variables),
                                                                      rdup_variable_list(cnf->encoded_variables),
                                                                      rdup_constant_list(cnf->constants),
                                                                      cnf->encoding,
                                                                      terms))));
	lsss_get_all_solutions(cnf->clauses, cnf->variables->sz, terms);
    } else if (input->tag == TAGtv_cnf_hierarchy) {
	node_list nodes = new_node_list();
	*output = to_serializable(new_tv_dnf_hierarchy(nodes));
	for (ix = 0; ix < to_hierarchy(input)->nodes->sz; ix++) {
	    tv_cnf node_input = to_tv_cnf(to_hierarchy(input)->nodes->arr[ix]->constraints);
	    tv_dnf node_output = new_tv_dnf(rdup_values_set_list(node_input->domains),
					    rdup_variable_list(node_input->variables),
					    rdup_variable_list(node_input->encoded_variables),
					    rdup_constant_list(node_input->constants),
					    node_input->encoding,
					    node_input->clauses == tv_clause_listNIL ? tv_term_listNIL : new_tv_term_list());
	    node result;
	    lsss_get_all_solutions(node_input->clauses,
                                   node_input->variables->sz,
                                   node_output->terms);
	    result = new_node(rdup_lydia_symbol(to_hierarchy(input)->nodes->arr[ix]->type),
			      rdup_edge_list(to_hierarchy(input)->nodes->arr[ix]->edges),
			      to_kb(node_output));
	    append_node_list(nodes, result);
	}
    } else {
	assert(0);
    }
    stop_stopwatch("compilation time");

    return 1;
}

void cnf2dnf_init()
{
    stat_init();
    init_stopwatch("compilation time", "CNF to DNF conversion time: %d s %d.%d ms", "compilation");
}

void cnf2dnf_destroy()
{
    stat_destroy();
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
