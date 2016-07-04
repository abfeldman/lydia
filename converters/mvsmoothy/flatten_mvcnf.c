#include "flatten_mvcnf.h"
#include "mv.h"
#include "sorted_int_list.h"
#include "inline.h"

#include <assert.h>

mv_clause_list map_mv_cnf(mv_clause_list clauses,
				 int_list_list variable_mappings,
				 const unsigned int current_map)
{
    register unsigned int ix, iy;

    mv_clause_list result = rdup_mv_clause_list(clauses);

    int_list map = variable_mappings->arr[current_map];
    for (ix = 0; ix < result->sz; ix++) {
	mv_literal_list pos = result->arr[ix]->pos;
	mv_literal_list neg = result->arr[ix]->neg;
	for (iy = 0; iy < pos->sz; iy++) {
	    pos->arr[iy]->var = map->arr[pos->arr[iy]->var];
	}
	for (iy = 0; iy < neg->sz; iy++) {
	    neg->arr[iy]->var = map->arr[neg->arr[iy]->var];
	}
    }

    return result;
}

mv_clause_list multiply_mv_cnf(mv_clause_list left,
				      mv_clause_list right,
				      int_list_list variable_mappings,
				      const unsigned int current_map)
{
    return concat_mv_clause_list(left,
				 map_mv_cnf(right,
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


