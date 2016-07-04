#include "flatten_mvdnf.h"
#include "mv.h"
#include "sorted_int_list.h"
#include "inline.h"

#include <assert.h>

mv_term_list map_mv_dnf(mv_term_list terms,
				 int_list_list variable_mappings,
				 const unsigned int current_map)
{
    register unsigned int ix, iy;

    mv_term_list result = rdup_mv_term_list(terms);

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

mv_term_list multiply_mv_dnf(mv_term_list left,
				      mv_term_list right,
				      int_list_list variable_mappings,
				      const unsigned int current_map)
{
    /* TODO: implement:
     *        concatenate terms 
     *         (making conjunction of disjunctions of conjunctions)
     *        then use demorgan to get dnf back
     */
    register unsigned int ix, iy;
    mv_term newterm;
    mv_term_list mapped_right;
    mv_term_list result;

    if(left->sz==0)
	return rdup_mv_term_list(right);
    if(right->sz==0)
	return rdup_mv_term_list(left);

    mapped_right = map_mv_dnf(right,
	    		      variable_mappings,
			      current_map);
    result = new_mv_term_list();
    for(ix=0; ix<left->sz; ix++)
	for(iy=0; iy<right->sz; iy++) {
	    newterm = rdup_mv_term(left->arr[ix]);
	    concat_mv_literal_list(newterm->pos, rdup_mv_literal_list(mapped_right->arr[iy]->pos));
	    concat_mv_literal_list(newterm->neg, rdup_mv_literal_list(mapped_right->arr[iy]->neg));
	    append_mv_term_list(result, newterm);
	}
    rfre_mv_term_list(mapped_right);
    /* TODO: simplify intermediate dnf */
    return result;
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


