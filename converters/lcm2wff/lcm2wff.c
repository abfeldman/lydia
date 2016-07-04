#include "config.h"

#include <math.h>
#include <assert.h>

#include "hierarchy.h"
#include "variable.h"
#include "walk.h"

static node convert_tv_model(const_node sys)
{
    node result = new_node(sys->type,
			   edge_listNIL,
			   to_kb(new_tv_wff(new_values_set_list(),
					    new_variable_list(),
					    new_variable_list(),
					    new_constant_list(),
					    ENCODING_NONE,
					    new_tv_wff_expr_list())));

    unsigned int i;
    for (i = 0; i < sys->constraints->variables->sz; i++) {
	if (sys->constraints->variables->arr[i]->tag != TAGbool_variable) {
	    rfre_node(result);
	    return nodeNIL;
	}
	result->constraints->variables = append_variable_list(result->constraints->variables, rdup_variable(sys->constraints->variables->arr[i]));
    }
    for (i = 0; i < sys->constraints->constants->sz; i++) {
	if (sys->constraints->constants->arr[i]->tag != TAGbool_constant) {
	    rfre_node(result);
	    return nodeNIL;
	}
    }
    if (sys->edges != edge_listNIL) {
	result->edges = rdup_edge_list(sys->edges);
    }
    for (i = 0; i < to_csp(sys->constraints)->sentences->sz; i++) {
	tv_wff_expr expr = walk_tv_sentence(to_csp(sys->constraints)->sentences->arr[i], sys->constraints->variables, sys->constraints->constants);
	if (tv_wff_exprNIL == expr) {
	    rfre_node(result);
	    return nodeNIL;
	}
	append_tv_wff_expr_list(to_tv_wff(result->constraints)->e, expr);
    }
    return result;
}

int lcm2wff(const_csp_hierarchy input, tv_wff_hierarchy *output)
{
    unsigned int ix;

    *output = new_tv_wff_hierarchy(new_node_list());

    for (ix = 0; ix < input->nodes->sz; ix++) {
	const_node node_input = input->nodes->arr[ix];
	node node_output = convert_tv_model(node_input);
	if (node_output == nodeNIL) {
	    rfre_tv_wff_hierarchy(*output);
	    return 0;
	}
	append_node_list((*output)->nodes, to_node(node_output));
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
