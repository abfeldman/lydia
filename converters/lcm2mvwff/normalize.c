#include <assert.h>

#include "normalize.h"

#include "hierarchy.h"
#include "variable.h"
#include "mv.h"

static int is_variable_or_constant_sentence(const_csp_sentence sentence)
{
    return ((TAGcsp_atomic_sentence == sentence->tag) &&
	    ((to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_variable_term) ||
	     (to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_constant_term)));
}

static int is_variable_or_constant_term(const_csp_term term)
{
    return ((term->tag == TAGcsp_variable_term) ||
	    (term->tag == TAGcsp_constant_term));
}

static int is_arith_if_sentence(const_csp_sentence sentence)
{
    return ((TAGcsp_atomic_sentence == sentence->tag) &&
	    to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_function_term &&
	    to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->name == add_lydia_symbol("arith_if"));
}

static int is_arith_if_term(const_csp_term term)
{
    return (term->tag == TAGcsp_function_term &&
	    to_csp_function_term(term)->name == add_lydia_symbol("arith_if"));
}

static int is_arith_switch_sentence(const_csp_sentence sentence)
{
    return ((TAGcsp_atomic_sentence == sentence->tag) &&
	    to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_function_term &&
	    to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->name == add_lydia_symbol("arith_switch"));
}

static int is_arith_switch_term(const_csp_term term)
{
    return (term->tag == TAGcsp_function_term &&
	    to_csp_function_term(term)->name == add_lydia_symbol("arith_switch"));
}

static csp_sentence get_arith_if_sentence_condition(const_csp_sentence sentence)
{
    assert(TAGcsp_atomic_sentence == sentence->tag &&
	   to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_function_term &&
	   to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->name == add_lydia_symbol("arith_if") &&
	   to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->args->sz == 3);
    return to_csp_sentence(new_csp_atomic_sentence(rdup_csp_term(to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->args->arr[0])));
}

static csp_term get_arith_if_term_condition(const_csp_term term)
{
    assert(term->tag == TAGcsp_function_term &&
	   to_csp_function_term(term)->name == add_lydia_symbol("arith_if") &&
	   to_csp_function_term(term)->args->sz == 3);
    return rdup_csp_term(to_csp_function_term(term)->args->arr[0]);
}

static csp_sentence get_arith_if_sentence_positive_expression(const_csp_sentence sentence)
{
    assert(TAGcsp_atomic_sentence == sentence->tag &&
	   to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_function_term &&
	   to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->name == add_lydia_symbol("arith_if") &&
	   to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->args->sz == 3);
    return to_csp_sentence(new_csp_atomic_sentence(rdup_csp_term(to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->args->arr[1])));
}

static csp_term get_arith_if_term_positive_expression(const_csp_term term)
{
    assert(term->tag == TAGcsp_function_term &&
	   to_csp_function_term(term)->name == add_lydia_symbol("arith_if") &&
	   to_csp_function_term(term)->args->sz == 3);
    return rdup_csp_term(to_csp_function_term(term)->args->arr[1]);
}

static csp_sentence get_arith_if_sentence_negative_expression(const_csp_sentence sentence)
{
    assert(TAGcsp_atomic_sentence == sentence->tag &&
	   to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_function_term &&
	   to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->name == add_lydia_symbol("arith_if") &&
	   to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->args->sz == 3);
    return to_csp_sentence(new_csp_atomic_sentence(rdup_csp_term(to_csp_function_term(to_csp_atomic_sentence(sentence)->a)->args->arr[2])));
}

static csp_term get_arith_if_term_negative_expression(const_csp_term term)
{
    assert(term->tag == TAGcsp_function_term &&
	   to_csp_function_term(term)->name == add_lydia_symbol("arith_if") &&
	   to_csp_function_term(term)->args->sz == 3);
    return rdup_csp_term(to_csp_function_term(term)->args->arr[1]);
}

static csp_term make_equiv_function_term(csp_term lhs, csp_term rhs)
{
    return to_csp_term(new_csp_function_term(add_lydia_symbol("equiv"),
					     append_csp_term_list(append_csp_term_list(new_csp_term_list(), lhs), rhs)));
}

static csp_term make_and_function_term(csp_term lhs, csp_term rhs)
{
    return to_csp_term(new_csp_function_term(add_lydia_symbol("and"),
					     append_csp_term_list(append_csp_term_list(new_csp_term_list(), lhs), rhs)));
}

static csp_term make_or_function_term(csp_term lhs, csp_term rhs)
{
    return to_csp_term(new_csp_function_term(add_lydia_symbol("or"),
					     append_csp_term_list(append_csp_term_list(new_csp_term_list(), lhs), rhs)));
}

static csp_term make_not_function_term(csp_term term)
{
    return to_csp_term(new_csp_function_term(add_lydia_symbol("not"),
					     append_csp_term_list(new_csp_term_list(), term)));
}

static csp_term arith_switch_to_ifs(csp_term term)
{
    register unsigned int ix;

    csp_term *tail = NULL, result = csp_termNIL, cond;

    assert(term->tag == TAGcsp_function_term);
    assert(to_csp_function_term(term)->name == add_lydia_symbol("arith_switch"));
    assert(to_csp_function_term(term)->args->sz > 2);

    cond = to_csp_function_term(term)->args->arr[0];
    for (ix = 1; ix < to_csp_function_term(term)->args->sz - 1; ix += 2) {
	csp_term next = to_csp_term(new_csp_function_term(add_lydia_symbol("arith_if"), append_csp_term_list(append_csp_term_list(append_csp_term_list(new_csp_term_list(), csp_termNIL), csp_termNIL), csp_termNIL)));
	to_csp_function_term(next)->args->arr[0] = make_equiv_function_term(rdup_csp_term(cond), rdup_csp_term(to_csp_function_term(term)->args->arr[ix]));
	to_csp_function_term(next)->args->arr[1] = rdup_csp_term(to_csp_function_term(term)->args->arr[ix + 1]);
	if (ix == 1) {
	    result = next;
	} else {
	    *tail = next;
	}
	tail = &to_csp_function_term(next)->args->arr[2];
    }
    if (ix < to_csp_function_term(term)->args->sz) {
	*tail = rdup_csp_term(to_csp_function_term(term)->args->arr[ix]);
    }

    return result;
}

static csp_term walk_term(csp_term term)
{
    lydia_symbol name;
    unsigned int ix;

    switch (term->tag) {
	case TAGcsp_function_term:
	    for (ix = 0; ix < to_csp_function_term(term)->args->sz; ix++) {
		to_csp_function_term(term)->args->arr[ix] = walk_term(to_csp_function_term(term)->args->arr[ix]);
	    }

	    name = to_csp_function_term(term)->name;
	    if (add_lydia_symbol("ne") == name) {
		to_csp_function_term(term)->name = add_lydia_symbol("equiv");
		return walk_term(to_csp_term(new_csp_function_term(add_lydia_symbol("not"), append_csp_term_list(new_csp_term_list(), term))));
	    }
	    if (add_lydia_symbol("arith_switch") == name) {
		csp_term result = arith_switch_to_ifs(term);
		rfre_csp_term(term);
		return walk_term(result);
	    }
	    if (add_lydia_symbol("equiv") == name) {
		csp_term lhs, rhs;

		assert(to_csp_function_term(term)->args->sz == 2);
		lhs = to_csp_function_term(term)->args->arr[0];
		rhs = to_csp_function_term(term)->args->arr[1];

/*
 * Replace "(<cond> ? <pos> : <neg>) = <variable>" with
 * "<variable> = (<cond> ? <pos> : <neg>)".
 */
		if ((is_arith_if_term(lhs) ||
		     is_arith_switch_term(lhs)) &&
		    is_variable_or_constant_term(rhs)) {
		    csp_term temp = lhs;
		    to_csp_function_term(term)->args->arr[0] = lhs = rhs;
		    to_csp_function_term(term)->args->arr[1] = rhs = temp;
		}
/*
 * Now replace "<variable> = (<cond> ? <pos> : <neg>)" with
 * "(<cond> => (<variable> = <pos>)) && (!<cond> => (<variable> = <neg>))".
 */
		if (is_variable_or_constant_term(lhs) && is_arith_if_term(rhs)) {
		    csp_term cond = get_arith_if_term_condition(rhs);
		    csp_term pos = get_arith_if_term_positive_expression(rhs);
		    csp_term neg = get_arith_if_term_negative_expression(rhs);
		    csp_term variable = rdup_csp_term(lhs);
		    csp_term result = make_and_function_term(make_or_function_term(make_not_function_term(cond),
										   make_equiv_function_term(variable, pos)),
							     make_or_function_term(rdup_csp_term(cond),
										   make_equiv_function_term(rdup_csp_term(variable), neg)));
		    rfre_csp_term(term);
		    return walk_term(result);
		}
	    }
	    break;
	case TAGcsp_constant_term:
	case TAGcsp_variable_term:
/* Fall thru. */
	    break;
    }
    return term;
}

static csp_sentence walk_sentence(csp_sentence sentence)
{
    switch (sentence->tag) {
	case TAGcsp_not_sentence:
	    to_csp_not_sentence(sentence)->n = walk_sentence(to_csp_not_sentence(sentence)->n);
	    break;
	case TAGcsp_and_sentence:
	    to_csp_and_sentence(sentence)->lhs = walk_sentence(to_csp_and_sentence(sentence)->lhs);
	    to_csp_and_sentence(sentence)->rhs = walk_sentence(to_csp_and_sentence(sentence)->rhs);
	    break;
	case TAGcsp_or_sentence:
	    to_csp_or_sentence(sentence)->lhs = walk_sentence(to_csp_or_sentence(sentence)->lhs);
	    to_csp_or_sentence(sentence)->rhs = walk_sentence(to_csp_or_sentence(sentence)->rhs);
	    break;
	case TAGcsp_impl_sentence:
	    to_csp_impl_sentence(sentence)->lhs = walk_sentence(to_csp_impl_sentence(sentence)->lhs);
	    to_csp_impl_sentence(sentence)->rhs = walk_sentence(to_csp_impl_sentence(sentence)->rhs);
	    break;
	case TAGcsp_lt_sentence:
	    to_csp_lt_sentence(sentence)->lhs = walk_sentence(to_csp_lt_sentence(sentence)->lhs);
	    to_csp_lt_sentence(sentence)->rhs = walk_sentence(to_csp_lt_sentence(sentence)->rhs);
	    break;
	case TAGcsp_equiv_sentence:
	    to_csp_equiv_sentence(sentence)->lhs = walk_sentence(to_csp_equiv_sentence(sentence)->lhs);
	    to_csp_equiv_sentence(sentence)->rhs = walk_sentence(to_csp_equiv_sentence(sentence)->rhs);

/*
 * Replace "(<cond> ? <pos> : <neg>) = <variable>" with
 * "<variable> = (<cond> ? <pos> : <neg>)" and
 * "(cond (<cond>) (...) = <variable>" with
 * "<variable> = (cond (<cond>) (...)".
 */
	    if ((is_arith_if_sentence(to_csp_equiv_sentence(sentence)->lhs) ||
		 is_arith_switch_sentence(to_csp_equiv_sentence(sentence)->lhs)) &&
		is_variable_or_constant_sentence(to_csp_equiv_sentence(sentence)->rhs)) {
		csp_sentence temp = to_csp_equiv_sentence(sentence)->lhs;
		to_csp_equiv_sentence(sentence)->lhs = to_csp_equiv_sentence(sentence)->rhs;
		to_csp_equiv_sentence(sentence)->rhs = temp;
	    }
/*
 * Now replace "<variable> = (<cond> ? <pos> : <neg>)" with
 * "(<cond> => (<variable> = <pos>)) && (!<cond> => (<variable> = <neg>))".
 */
	    if (is_variable_or_constant_sentence(to_csp_equiv_sentence(sentence)->lhs) &&
		is_arith_if_sentence(to_csp_equiv_sentence(sentence)->rhs)) {
		csp_sentence result = to_csp_sentence(new_csp_and_sentence(to_csp_sentence(new_csp_impl_sentence(get_arith_if_sentence_condition(to_csp_equiv_sentence(sentence)->rhs),
														 to_csp_sentence(new_csp_equiv_sentence(rdup_csp_sentence(to_csp_equiv_sentence(sentence)->lhs), get_arith_if_sentence_positive_expression(to_csp_equiv_sentence(sentence)->rhs))))),
									   to_csp_sentence(new_csp_impl_sentence(to_csp_sentence(new_csp_not_sentence(get_arith_if_sentence_condition(to_csp_equiv_sentence(sentence)->rhs))),
														 to_csp_sentence(new_csp_equiv_sentence(rdup_csp_sentence(to_csp_equiv_sentence(sentence)->lhs), get_arith_if_sentence_negative_expression(to_csp_equiv_sentence(sentence)->rhs)))))));
		rfre_csp_sentence(sentence);
		sentence = walk_sentence(result);
		break;
	    }
	    break;
	case TAGcsp_atomic_sentence:
	    if (is_arith_switch_sentence(sentence)) {
		csp_term result = arith_switch_to_ifs(to_csp_atomic_sentence(sentence)->a);
		rfre_csp_term(to_csp_atomic_sentence(sentence)->a);
		to_csp_atomic_sentence(sentence)->a = result;
	    }
	    to_csp_atomic_sentence(sentence)->a = walk_term(to_csp_atomic_sentence(sentence)->a);
	    break;
    }
    return sentence;
}

node normalize_mv_model(node sys)
{
    unsigned int ix;

    for (ix = 0; ix < to_csp(sys->constraints)->sentences->sz; ix++) {
	to_csp(sys->constraints)->sentences->arr[ix] = walk_sentence(to_csp(sys->constraints)->sentences->arr[ix]);
    }

    return sys;
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
