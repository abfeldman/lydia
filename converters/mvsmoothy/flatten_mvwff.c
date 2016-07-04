#include "flatten_mvwff.h"
#include "mv.h"
#include "sorted_int_list.h"
#include "inline.h"

#include <assert.h>

mv_wff_expr map_mv_wff_expr(mv_wff_expr e,
				   int_list_list variable_mappings,
				   const unsigned int current_map)
{
    int_list map = int_listNIL;
    switch (e->tag) {
	case TAGmv_wff_e_or:
	    to_mv_wff_e_or(e)->lhs = map_mv_wff_expr(to_mv_wff_e_or(e)->lhs, variable_mappings, current_map);
	    to_mv_wff_e_or(e)->rhs = map_mv_wff_expr(to_mv_wff_e_or(e)->rhs, variable_mappings, current_map);
	    break;
	case TAGmv_wff_e_not:
	    to_mv_wff_e_not(e)->n = map_mv_wff_expr(to_mv_wff_e_not(e)->n, variable_mappings, current_map);
	    break;
	case TAGmv_wff_e_and:
	    to_mv_wff_e_and(e)->lhs = map_mv_wff_expr(to_mv_wff_e_and(e)->lhs, variable_mappings, current_map);
	    to_mv_wff_e_and(e)->rhs = map_mv_wff_expr(to_mv_wff_e_and(e)->rhs, variable_mappings, current_map);
	    break;
	case TAGmv_wff_e_equiv:
	    to_mv_wff_e_equiv(e)->lhs = map_mv_wff_expr(to_mv_wff_e_equiv(e)->lhs, variable_mappings, current_map);
	    to_mv_wff_e_equiv(e)->rhs = map_mv_wff_expr(to_mv_wff_e_equiv(e)->rhs, variable_mappings, current_map);
	    break;
	case TAGmv_wff_e_impl:
	    to_mv_wff_e_impl(e)->lhs = map_mv_wff_expr(to_mv_wff_e_impl(e)->lhs, variable_mappings, current_map);
	    to_mv_wff_e_impl(e)->rhs = map_mv_wff_expr(to_mv_wff_e_impl(e)->rhs, variable_mappings, current_map);
	    break;
	case TAGmv_wff_e_var:
	    map = variable_mappings->arr[current_map];
	    to_mv_wff_e_var(e)->var = map->arr[to_mv_wff_e_var(e)->var];
	    break;
	case TAGmv_wff_e_const:
/* Noop. */
	    break;
    }
    return e;
}

mv_wff_expr flatten_mv_wff_expr_list(mv_wff_expr_list e)
{
    register unsigned int ix;
    mv_wff_expr result = mv_wff_exprNIL;

    if (e->sz > 0) {
	result = rdup_mv_wff_expr(e->arr[0]);
    }
    for (ix = 1; ix < e->sz; ix++) {
	result = to_mv_wff_expr(new_mv_wff_e_and(result, rdup_mv_wff_expr(e->arr[ix])));
    }
    return result;
}

mv_wff_expr_list map_mv_wff(mv_wff_expr_list list,
				   int_list_list variable_mappings,
				   /*int_list_list UNUSED(constant_mappings),*/
				   const unsigned int current_map)
{
    register unsigned int ix;

    mv_wff_expr_list result = new_mv_wff_expr_list();
    mv_wff_expr expr = flatten_mv_wff_expr_list(list);
    if (expr != mv_wff_exprNIL) {
	append_mv_wff_expr_list(result, expr);
    }

    for (ix = 0; ix < result->sz; ix++) {
	map_mv_wff_expr(result->arr[ix], variable_mappings, current_map);
    }

    return result;
}

mv_wff_expr_list multiply_mv_wff(mv_wff_expr_list left,
					mv_wff_expr_list right,
					int_list_list variable_mappings,
					const unsigned int current_map)
{
    return concat_mv_wff_expr_list(left,
				   map_mv_wff(right,
					      variable_mappings,
					      current_map));

}


/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset:4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */


