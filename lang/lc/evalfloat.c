#include "config.h"
#include "evalfloat.h"
#include "evalbool.h"
#include "lc_types.h"
#include "defs.h"

#include <math.h>
#include <assert.h>

int evaluate_float_term(csp_term term, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, double *result)
{
    int c;
    double l, r, x;
    lydia_symbol name;
    unsigned pos;

    switch (term->tag) {
	case TAGcsp_function_term:
/* TODO: Implement this in a nicer fashion with a table of functions. */
	    name = to_csp_function_term(term)->name;
	    if (add_lydia_symbol("delay") == name) {
		assert(0); /* TODO: Unsupported message. */
	    }
	    if (add_lydia_symbol("on") == name) {
		assert(0); /* TODO: Unsupported message. */
	    }
	    if (add_lydia_symbol("abs") == name) {
		assert(to_csp_function_term(term)->args->sz == 1);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &x)) {
		    return 0;
		}
		*result = fabs(x);
		break;
	    }
	    if (add_lydia_symbol("neg") == name) {
		assert(to_csp_function_term(term)->args->sz == 1);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &x)) {
		    return 0;
		}
		*result = -x;
		break;
	    }
	    if (add_lydia_symbol("sin") == name) {
		assert(to_csp_function_term(term)->args->sz == 1);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &x)) {
		    return 0;
		}
		*result = sin(x);
		break;
	    }
	    if (add_lydia_symbol("cos") == name) {
		assert(to_csp_function_term(term)->args->sz == 1);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &x)) {
		    return 0;
		}
		*result = cos(x);
		break;
	    }
	    if (add_lydia_symbol("tan") == name) {
		assert(to_csp_function_term(term)->args->sz == 1);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &x)) {
		    return 0;
		}
		*result = tan(x);
		break;
	    }
	    if (add_lydia_symbol("min") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &l) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &r)) {
		    return 0;
		}
		*result = min(l, r);
		break;
	    }
	    if (add_lydia_symbol("max") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &l) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &r)) {
		    return 0;
		}
		*result = max(l, r);
		break;
	    }
	    if (add_lydia_symbol("add") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &l) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &r)) {
		    return 0;
		}
		*result = l + r;
		break;
	    }
	    if (add_lydia_symbol("sub") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &l) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &r)) {
		    return 0;
		}
		*result = l - r;
		break;
	    }
	    if (add_lydia_symbol("mul") == name) {
		assert(to_csp_function_term(term)->args->sz == 2);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &l) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &r)) {
		    return 0;
		}
		*result = l * r;
		break;
	    }
	    if (add_lydia_symbol("div") == name) {
/* TODO: Check for division by zero. */
		assert(to_csp_function_term(term)->args->sz == 2);
		if (!evaluate_float_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &l) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &r)) {
		    return 0;
		}
		*result = l / r;
		break;
	    }
	    if (add_lydia_symbol("arith_if") == name) {
		assert(to_csp_function_term(term)->args->sz == 3);
		if (!evaluate_bool_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &c) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &l) ||
		    !evaluate_float_term(to_csp_function_term(term)->args->arr[2], domains, variables, constants, known_variables, &r)) {
		    return 0;
		}
		*result = (c ? l : r);
		break;
	    }
	    if (add_lydia_symbol("arith_switch") == name) {
		csp_term cond;
		unsigned int ix, iz;
		assert(to_csp_function_term(term)->args->sz >= 4);
		assert(to_csp_function_term(term)->args->sz % 2 == 0);
		cond = to_csp_function_term(term)->args->arr[0];
		iz = to_csp_function_term(term)->args->sz;
		for (ix = 1; ix < iz - 1; ix += 2) {
		    if (!terms_equiv(cond, to_csp_function_term(term)->args->arr[ix], domains, variables, constants, known_variables, &c)) {
			return 0;
		    }
		    if (c) {
			if (!evaluate_float_term(to_csp_function_term(term)->args->arr[ix + 1], domains, variables, constants, known_variables, &x)) {
			    return 0;
			}
			*result = x;
			return 1;
		    }
		}
		if (csp_termNIL != to_csp_function_term(term)->args->arr[iz - 1]) {
		    if (!evaluate_float_term(to_csp_function_term(term)->args->arr[iz - 1], domains, variables, constants, known_variables, &x)) {
			return 0;
		    }
		    *result = x;
		    break;
		}
		return 0;
	    }
	    assert(0); /* TODO: Change this to an error. */
	    break;
	case TAGcsp_constant_term:
	    assert(constants->arr[to_csp_constant_term(term)->c]->tag == TAGfloat_constant);
	    *result = to_float_constant(constants->arr[to_csp_constant_term(term)->c])->value;
	    break;
	case TAGcsp_variable_term:
	    assert(variables->arr[to_csp_variable_term(term)->v]->tag == TAGfloat_variable); /* TODO: Change this to an error. */
	    if (!search_variable_assignment_list(known_variables,
						 to_csp_variable_term(term)->v,
						 &pos)) {
		return 0;
	    }
	    assert(known_variables->arr[pos]->tag == TAGfloat_variable_assignment);
	    *result = to_float_variable_assignment(known_variables->arr[pos])->value;
	    break;
	default:
	    assert(0); /* TODO: Change this to an error. */
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
