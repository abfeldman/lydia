#include <math.h>
#include <assert.h>

#include "hierarchy.h"
#include "variable.h"
#include "fold.h"
#include "mv.h"

static int is_bool_constant_sentence(csp_sentence sentence, lydia_bool value, constant_list constants)
{
    if (sentence->tag == TAGcsp_atomic_sentence &&
	to_csp_atomic_sentence(sentence)->a->tag == TAGcsp_constant_term &&
	constants->arr[to_csp_constant_term(to_csp_atomic_sentence(sentence)->a)->c]->tag == TAGbool_constant &&
	to_bool_constant(constants->arr[to_csp_constant_term(to_csp_atomic_sentence(sentence)->a)->c])->value == value) {
	return 1;
    }

    return 0;
}

static int is_bool_constant_term(csp_term term, lydia_bool value, constant_list constants)
{
    if (term->tag == TAGcsp_constant_term &&
	constants->arr[to_csp_constant_term(term)->c]->tag == TAGbool_constant &&
	to_bool_constant(constants->arr[to_csp_constant_term(term)->c])->value == value) {
	return 1;
    }

    return 0;
}

static csp_sentence new_bool_constant_sentence(lydia_bool value, constant_list constants)
{
    register unsigned int ix;

    for (ix = 0; ix < constants->sz; ix++) {
	if (constants->arr[ix]->tag == TAGbool_constant &&
	    to_bool_constant(constants->arr[ix])->value == value) {
	    break;
	}
    }
    if (ix == constants->sz) {
	append_constant_list(constants, to_constant(new_bool_constant(lydia_symbolNIL, value)));
    }

    return to_csp_sentence(new_csp_atomic_sentence(to_csp_term(new_csp_constant_term(ix))));
}

static csp_term new_bool_constant_term(lydia_bool value, constant_list constants)
{
    register unsigned int ix;

    for (ix = 0; ix < constants->sz; ix++) {
	if (constants->arr[ix]->tag == TAGbool_constant &&
	    to_bool_constant(constants->arr[ix])->value == value) {
	    break;
	}
    }
    if (ix == constants->sz) {
	append_constant_list(constants, to_constant(new_bool_constant(lydia_symbolNIL, value)));
    }

    return to_csp_term(new_csp_constant_term(ix));
}

static csp_term make_not_term(csp_term n)
{
    return to_csp_term(new_csp_function_term(add_lydia_symbol("not"), append_csp_term_list(new_csp_term_list(), n)));
}

static csp_term make_or_term(csp_term lhs, csp_term rhs)
{
    return to_csp_term(new_csp_function_term(add_lydia_symbol("or"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), lhs), rhs)));
}

static csp_term make_and_term(csp_term lhs, csp_term rhs)
{
    return to_csp_term(new_csp_function_term(add_lydia_symbol("and"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), lhs), rhs)));
}

static csp_term walk_term(csp_term term, constant_list constants)
{
    register unsigned int ix;
    lydia_symbol name;
    csp_term result;

    switch (term->tag) {
	case TAGcsp_function_term:
	    name = to_csp_function_term(term)->name;
	    for (ix = 0; ix < to_csp_function_term(term)->args->sz; ix++) {
		to_csp_function_term(term)->args->arr[ix] = walk_term(to_csp_function_term(term)->args->arr[ix], constants);
	    }
	    if (add_lydia_symbol("not") == name) {
		assert(to_csp_function_term(term)->args->sz == 1);
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_FALSE, constants)) {
		    rfre_csp_term(term);
		    return new_bool_constant_term(LYDIA_TRUE, constants);
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_TRUE, constants)) {
		    rfre_csp_term(term);
		    return new_bool_constant_term(LYDIA_FALSE, constants);
		}
	    }
	    if (add_lydia_symbol("mul") == name || add_lydia_symbol("and") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_FALSE, constants) ||
		    is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_FALSE, constants)) {
		    rfre_csp_term(term);
		    return new_bool_constant_term(LYDIA_FALSE, constants);
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    result = to_csp_function_term(term)->args->arr[1];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = to_csp_function_term(term)->args->arr[0];
		    fre_csp_term(term);
		    return result;
		}
	    }
	    if (add_lydia_symbol("add") == name || add_lydia_symbol("or") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_TRUE, constants) ||
		    is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_TRUE, constants)) {
		    rfre_csp_term(term);
		    return new_bool_constant_term(LYDIA_TRUE, constants);
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    result = to_csp_function_term(term)->args->arr[1];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = to_csp_function_term(term)->args->arr[0];
		    fre_csp_term(term);
		    return result;
		}
	    }
	    if (add_lydia_symbol("equiv") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    result = to_csp_function_term(term)->args->arr[1];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = to_csp_function_term(term)->args->arr[0];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    result = make_not_term(to_csp_function_term(term)->args->arr[1]);
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = make_not_term(to_csp_function_term(term)->args->arr[0]);
		    fre_csp_term(term);
		    return result;
		}
	    }
	    if (add_lydia_symbol("ne") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    result = to_csp_function_term(term)->args->arr[1];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = to_csp_function_term(term)->args->arr[0];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    result = make_not_term(to_csp_function_term(term)->args->arr[1]);
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = make_not_term(to_csp_function_term(term)->args->arr[0]);
		    fre_csp_term(term);
		    return result;
		}
	    }
	    if (add_lydia_symbol("arith_switch") == name) {
		assert(to_csp_function_term(term)->args->sz >= 2);
/* TODO: Implement. */
	    }
	    if (add_lydia_symbol("arith_if") == name) {
		assert(to_csp_function_term(term)->args->sz == 3);
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    rfre_csp_term(to_csp_function_term(term)->args->arr[2]);
		    result = to_csp_function_term(term)->args->arr[1];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[0], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[0]);
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = to_csp_function_term(term)->args->arr[2];
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[2], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[2]);
		    result = make_or_term(make_not_term(to_csp_function_term(term)->args->arr[0]), to_csp_function_term(term)->args->arr[1]);
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_TRUE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = make_or_term(to_csp_function_term(term)->args->arr[0], to_csp_function_term(term)->args->arr[2]);
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[2], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[2]);
		    result = make_and_term(make_not_term(to_csp_function_term(term)->args->arr[0]), to_csp_function_term(term)->args->arr[1]);
		    fre_csp_term(term);
		    return result;
		}
		if (is_bool_constant_term(to_csp_function_term(term)->args->arr[1], LYDIA_FALSE, constants)) {
		    rfre_csp_term(to_csp_function_term(term)->args->arr[1]);
		    result = make_and_term(make_not_term(to_csp_function_term(term)->args->arr[0]), to_csp_function_term(term)->args->arr[2]);
		    fre_csp_term(term);
		    return result;
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

static csp_sentence walk_sentence(csp_sentence sentence, constant_list constants)
{
    csp_sentence result;

    switch (sentence->tag) {
	case TAGcsp_not_sentence:
	    to_csp_not_sentence(sentence)->n = walk_sentence(to_csp_not_sentence(sentence)->n, constants);
	    if (is_bool_constant_sentence(to_csp_not_sentence(sentence)->n, LYDIA_FALSE, constants)) {
		rfre_csp_sentence(sentence);
		return new_bool_constant_sentence(LYDIA_TRUE, constants);
	    }
	    if (is_bool_constant_sentence(to_csp_not_sentence(sentence)->n, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(sentence);
		return new_bool_constant_sentence(LYDIA_FALSE, constants);
	    }
	    break;
	case TAGcsp_and_sentence:
	    to_csp_and_sentence(sentence)->lhs = walk_sentence(to_csp_and_sentence(sentence)->lhs, constants);
	    to_csp_and_sentence(sentence)->rhs = walk_sentence(to_csp_and_sentence(sentence)->rhs, constants);
	    if (is_bool_constant_sentence(to_csp_or_sentence(sentence)->lhs, LYDIA_FALSE, constants) ||
		is_bool_constant_sentence(to_csp_or_sentence(sentence)->rhs, LYDIA_FALSE, constants)) {
		rfre_csp_sentence(sentence);
		return new_bool_constant_sentence(LYDIA_FALSE, constants);
	    }
	    if (is_bool_constant_sentence(to_csp_and_sentence(sentence)->lhs, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(to_csp_and_sentence(sentence)->lhs);
		result = to_csp_and_sentence(sentence)->rhs;
		fre_csp_sentence(sentence);
		return result;
	    }
	    if (is_bool_constant_sentence(to_csp_and_sentence(sentence)->rhs, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(to_csp_and_sentence(sentence)->rhs);
		result = to_csp_and_sentence(sentence)->lhs;
		fre_csp_sentence(sentence);
		return result;
	    }
	    break;
	case TAGcsp_or_sentence:
	    to_csp_or_sentence(sentence)->lhs = walk_sentence(to_csp_or_sentence(sentence)->lhs, constants);
	    to_csp_or_sentence(sentence)->rhs = walk_sentence(to_csp_or_sentence(sentence)->rhs, constants);
	    if (is_bool_constant_sentence(to_csp_or_sentence(sentence)->lhs, LYDIA_TRUE, constants) ||
		is_bool_constant_sentence(to_csp_or_sentence(sentence)->rhs, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(sentence);
		return new_bool_constant_sentence(LYDIA_TRUE, constants);
	    }
	    if (is_bool_constant_sentence(to_csp_or_sentence(sentence)->lhs, LYDIA_FALSE, constants)) {
		rfre_csp_sentence(to_csp_or_sentence(sentence)->lhs);
		result = to_csp_or_sentence(sentence)->rhs;
		fre_csp_sentence(sentence);
		return result;
	    }
	    if (is_bool_constant_sentence(to_csp_or_sentence(sentence)->rhs, LYDIA_FALSE, constants)) {
		rfre_csp_sentence(to_csp_or_sentence(sentence)->rhs);
		result = to_csp_or_sentence(sentence)->lhs;
		fre_csp_sentence(sentence);
		return result;
	    }
	    break;
	case TAGcsp_impl_sentence:
	    to_csp_impl_sentence(sentence)->lhs = walk_sentence(to_csp_impl_sentence(sentence)->lhs, constants);
	    to_csp_impl_sentence(sentence)->rhs = walk_sentence(to_csp_impl_sentence(sentence)->rhs, constants);
	    if (is_bool_constant_sentence(to_csp_impl_sentence(sentence)->lhs, LYDIA_FALSE, constants) ||
		is_bool_constant_sentence(to_csp_impl_sentence(sentence)->rhs, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(sentence);
		return new_bool_constant_sentence(LYDIA_TRUE, constants);
	    }
	    if (is_bool_constant_sentence(to_csp_impl_sentence(sentence)->lhs, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(to_csp_impl_sentence(sentence)->lhs);
		result = to_csp_impl_sentence(sentence)->rhs;
		fre_csp_sentence(sentence);
		return result;
	    }
	    if (is_bool_constant_sentence(to_csp_impl_sentence(sentence)->rhs, LYDIA_FALSE, constants)) {
		rfre_csp_sentence(to_csp_impl_sentence(sentence)->rhs);
		result = to_csp_sentence(new_csp_not_sentence(to_csp_impl_sentence(sentence)->lhs));
		fre_csp_sentence(sentence);
		return result;
	    }
	    break;
	case TAGcsp_lt_sentence:
	    to_csp_lt_sentence(sentence)->lhs = walk_sentence(to_csp_lt_sentence(sentence)->lhs, constants);
	    to_csp_lt_sentence(sentence)->rhs = walk_sentence(to_csp_lt_sentence(sentence)->rhs, constants);
	    break;
	case TAGcsp_equiv_sentence:
	    to_csp_equiv_sentence(sentence)->lhs = walk_sentence(to_csp_equiv_sentence(sentence)->lhs, constants);
	    to_csp_equiv_sentence(sentence)->rhs = walk_sentence(to_csp_equiv_sentence(sentence)->rhs, constants);
	    if (is_bool_constant_sentence(to_csp_equiv_sentence(sentence)->lhs, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(to_csp_equiv_sentence(sentence)->lhs);
		result = to_csp_equiv_sentence(sentence)->rhs;
		fre_csp_sentence(sentence);
		return result;
	    }
	    if (is_bool_constant_sentence(to_csp_equiv_sentence(sentence)->rhs, LYDIA_TRUE, constants)) {
		rfre_csp_sentence(to_csp_equiv_sentence(sentence)->rhs);
		result = to_csp_equiv_sentence(sentence)->lhs;
		fre_csp_sentence(sentence);
		return result;
	    }
	    if (is_bool_constant_sentence(to_csp_equiv_sentence(sentence)->lhs, LYDIA_FALSE, constants)) {
		rfre_csp_sentence(to_csp_equiv_sentence(sentence)->lhs);
		result = to_csp_sentence(new_csp_not_sentence(to_csp_equiv_sentence(sentence)->rhs));
		fre_csp_sentence(sentence);
		return result;
	    }
	    if (is_bool_constant_sentence(to_csp_equiv_sentence(sentence)->rhs, LYDIA_FALSE, constants)) {
		rfre_csp_sentence(to_csp_equiv_sentence(sentence)->rhs);
		result = to_csp_sentence(new_csp_not_sentence(to_csp_equiv_sentence(sentence)->lhs));
		fre_csp_sentence(sentence);
		return result;
	    }
	    break;
	case TAGcsp_atomic_sentence:
	    to_csp_atomic_sentence(sentence)->a = walk_term(to_csp_atomic_sentence(sentence)->a, constants);
	    break;
    }
    return sentence;
}

void fold_bool_constants(node sys)
{
    unsigned int ix;

    if (to_csp(sys->constraints)->sentences->sz > 0) {
	for (ix = 0; ix < to_csp(sys->constraints)->sentences->sz; ix++) {
	    to_csp(sys->constraints)->sentences->arr[ix] = walk_sentence(to_csp(sys->constraints)->sentences->arr[ix], sys->constraints->constants);
	}
    }
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
