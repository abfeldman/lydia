#include "config.h"

#include <assert.h>
#include <math.h>

#include "lc.h"
#include "expr.h"
#include "iter.h"
#include "error.h"
#include "search.h"

index_entry_list init_quantifier_table(const_quantifier_entry_list quantifier_table, const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    index_entry_list result = new_index_entry_list();

    for (ix = 0; ix < quantifier_table->sz; ix++) {
	append_index_entry_list(result,
				new_index_entry(quantifier_table->arr[ix]->name, eval_int_expr(quantifier_table->arr[ix]->from, result, user_type_table)));
    }

    return result;
}

index_entry_list advance_quantifier_table(index_entry_list indices,
					  const_quantifier_entry_list quantifier_table,
					  const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    for (ix = quantifier_table->sz - 1; ix < quantifier_table->sz; ix--) {
	if (indices->arr[ix]->value < eval_int_expr(quantifier_table->arr[ix]->to, indices, user_type_table)) {
	    indices->arr[ix]->value += 1;
	    break;
	}
	indices->arr[ix]->value = eval_int_expr(quantifier_table->arr[ix]->from, indices, user_type_table);
    }

    if (ix >= indices->sz) {
	rfre_index_entry_list(indices);
	return index_entry_listNIL;
    }

    return indices;
}

int_list init_index(const_extent_list ranges,
		    const_index_entry_list quantifier_indices,
		    const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    int_list indices = new_int_list();
    for (ix = 0; ix < ranges->sz; ix++) {
	append_int_list(indices,
			eval_int_expr(ranges->arr[ix]->from,
				      quantifier_indices,
				      user_type_table));
    }
    return indices;
}

/* Finds the next array index, given a current index and the array extents. */
int_list advance_index(int_list indices,
		       const_extent_list ranges,
		       const_index_entry_list quantifier_indices,
		       const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    assert(indices->sz == ranges->sz);

    for (ix = indices->sz - 1; ix < indices->sz; ix--) {
	if (indices->arr[ix] < eval_int_expr(ranges->arr[ix]->to, quantifier_indices, user_type_table)) {
	    indices->arr[ix] += 1;
	    break;
	}
	indices->arr[ix] = eval_int_expr(ranges->arr[ix]->from, quantifier_indices, user_type_table);
    }

    if (ix >= indices->sz) {
	rfre_int_list(indices);
	return int_listNIL;
    }

    return indices;
}

extent_list init_extent(const_extent_list ranges,
			const_index_entry_list quantifier_indices,
			const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    extent_list extent = new_extent_list();
    for (ix = 0; ix < ranges->sz; ix++) {
	int v = eval_int_expr(ranges->arr[ix]->from, quantifier_indices, user_type_table);
	append_extent_list(extent,
			   new_extent(to_expr(new_expr_int(to_type(new_int_type()), v)),
				      to_expr(new_expr_int(to_type(new_int_type()), v))));
    }
    return extent;
}

extent_list advance_extent(extent_list extent,
			   const_extent_list ranges,
			   const_index_entry_list quantifier_indices,
			   const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    assert(extent->sz == ranges->sz);

    for (ix = extent->sz - 1; ix < extent->sz; ix--) {
	int p = eval_int_expr(extent->arr[ix]->from, quantifier_indices, user_type_table);
	int s = eval_int_expr(extent->arr[ix]->to, quantifier_indices, user_type_table);
	int r = eval_int_expr(ranges->arr[ix]->from, quantifier_indices, user_type_table);
	int q = eval_int_expr(ranges->arr[ix]->to, quantifier_indices, user_type_table);

	rfre_expr(extent->arr[ix]->from);
	rfre_expr(extent->arr[ix]->to);
	if (r < q) {
	    if (p < q) {
		p += 1;
		s += 1;
		extent->arr[ix]->from = to_expr(new_expr_int(to_type(new_int_type()), p));
		extent->arr[ix]->to = to_expr(new_expr_int(to_type(new_int_type()), s));
		break;
	    }
	} else {
	    if (p > q) {
		p -= 1;
		s -= 1;
		extent->arr[ix]->from = to_expr(new_expr_int(to_type(new_int_type()), p));
		extent->arr[ix]->to = to_expr(new_expr_int(to_type(new_int_type()), s));
		break;
	    }
	}
	extent->arr[ix]->from = to_expr(new_expr_int(to_type(new_int_type()), r));
	extent->arr[ix]->to = to_expr(new_expr_int(to_type(new_int_type()), r));
    }

    if (ix >= extent->sz) {
	rfre_extent_list(extent);
	return extent_listNIL;
    }

    return extent;
}

/*
 * local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
