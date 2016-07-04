#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "tv.h"
#include "defs.h"
#include "hash.h"
#include "util.h"
#include "obdd.h"
#include "cnf2obdd.h"
#include "variable.h"
#include "hierarchy.h"
#include "sorted_int_list.h"

#define pair(i, j) (((i) + (j)) * ((i) + (j) + 1) / 2 + (i))

static unsigned long node_hash_function(const char *key, unsigned int UNUSED(key_length))
{
    obdd_non_terminal_node node = to_obdd_non_terminal_node(key);

    return pair(node->var, pair(node->low, node->high)) % 15485863L;
}

/* Use this for non-terminal nodes only. */
int add_obdd_node(obdd_node_list nodes,
		  hash_table *index,
		  int var,
		  int low,
		  int high)
{
    obdd_non_terminal_node node;
    void *ix = NULL;

    if (low == high) {
	return low;
    }

    node = new_obdd_non_terminal_node(var, low, high);

    if (0 == hash_find(index,
		       (char *)node,
		       sizeof(struct str_obdd_non_terminal_node),
		       &ix)) {
	rfre_obdd_non_terminal_node(node);
	return *(int *)ix;
    }
    hash_add(index,
	     (char *)node,
	     sizeof(struct str_obdd_non_terminal_node),
	     (void *)&nodes->sz,
	     sizeof(int),
	     NULL);
    append_obdd_node_list(nodes, to_obdd_node(node));

    return nodes->sz - 1;
}

static tv_clause_list cond_tv_clause_list(tv_clause_list clauses,
					  int var,
					  lydia_bool val)
{
    register unsigned int ix;
    unsigned int iy;

    tv_clause_list result = rdup_tv_clause_list(clauses);
    for (ix = 0; ix < result->sz; ix++) {
	if (search_sorted_int_list(result->arr[ix]->pos, var, &iy)) {
	    if (LYDIA_TRUE == val) {
		delete_tv_clause_list(result, ix);
		ix -= 1;
		continue;
	    }
	    delete_int_list(result->arr[ix]->pos, iy);
	}
	if (search_sorted_int_list(result->arr[ix]->neg, var, &iy)) {
	    if (LYDIA_FALSE == val) {
		delete_tv_clause_list(result, ix);
		ix -= 1;
		continue;
	    }
	    delete_int_list(result->arr[ix]->neg, iy);
	}
	if (0 == result->arr[ix]->pos->sz &&
	    0 == result->arr[ix]->neg->sz) {
	    rfre_tv_clause_list(result);
	    return tv_clause_listNIL;
	}
    }
    return result;
}

static int build_obdd(unsigned int var,
		      unsigned int cnt,
		      tv_clause_list clauses,
		      obdd_node_list nodes,
		      hash_table *index)
{
    tv_clause_list clf, clt;
    int vf, vt;

    if (tv_clause_listNIL == clauses) {
	return 0;
    }
    if (0 == clauses->sz) {
	return 1;
    }
    clf = cond_tv_clause_list(rdup_tv_clause_list(clauses), var, LYDIA_FALSE);
    clt = cond_tv_clause_list(rdup_tv_clause_list(clauses), var, LYDIA_TRUE);
    vf = build_obdd(var + 1, cnt, clf, nodes, index);
    vt = build_obdd(var + 1, cnt, clt, nodes, index);
    rfre_tv_clause_list(clf);
    rfre_tv_clause_list(clt);
    return add_obdd_node(nodes, index, var, vf, vt);
}

static void tv_clause_list_to_obdd(const_variable_list variables,
				   tv_clause_list clauses,
				   obdd_node_list nodes)
{
    hash_table index;

/* These are the trivial cases. */
    if (tv_clause_listNIL == clauses) {
	append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
	return;
    }
    if (0 == clauses->sz) {
	append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));
	return;
    }
/* Add the terminal nodes. */
    append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
    append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));

    hash_init(&index, 2, node_hash_function, NULL);

    build_obdd(0, variables->sz, clauses, nodes, &index);

    hash_destroy(&index);
}

int cnf2obdd(const_serializable input, serializable *output)
{
    register unsigned int ix;

    if (input->tag == TAGtv_cnf) {
	*output = to_serializable(new_obdd(rdup_values_set_list(to_tv_cnf(input)->domains),
					   rdup_variable_list(to_tv_cnf(input)->variables),
					   rdup_variable_list(to_tv_cnf(input)->encoded_variables),
					   rdup_constant_list(to_tv_cnf(input)->constants),
					   to_tv_cnf(input)->encoding,
					   new_obdd_node_list(),
					   -1));
	tv_clause_list_to_obdd(to_tv_cnf(input)->variables,
			       to_tv_cnf(input)->clauses,
			       to_obdd(*output)->nodes);
    } else if (input->tag == TAGtv_cnf_hierarchy) {
	node_list nodes = new_node_list();
	*output = to_serializable(new_obdd_hierarchy(nodes));
	for (ix = 0; ix < to_hierarchy(input)->nodes->sz; ix++) {
	    tv_cnf node_input = to_tv_cnf(to_hierarchy(input)->nodes->arr[ix]->constraints);
	    obdd node_output = new_obdd(rdup_values_set_list(node_input->domains),
					rdup_variable_list(node_input->variables),
					rdup_variable_list(node_input->encoded_variables),
					rdup_constant_list(node_input->constants),
					node_input->encoding,
					new_obdd_node_list(),
					-1);
	    node result;
	    tv_clause_list_to_obdd(node_input->variables,
				   node_input->clauses,
				   node_output->nodes);
	    result = new_node(rdup_lydia_symbol(to_hierarchy(input)->nodes->arr[ix]->type),
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
