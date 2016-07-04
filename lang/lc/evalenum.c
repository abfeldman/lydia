#include "config.h"

#include <math.h>
#include <assert.h>

#include "lcm.h"
#include "lc_types.h"
#include "evalenum.h"
#include "evalbool.h"

int evaluate_enum_term(csp_term term, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, lydia_symbol *result)
{
    int c;
    lydia_symbol l, r, x;
    lydia_symbol name;
    unsigned pos;

    switch (term->tag) {
	case TAGcsp_function_term:
	    name = to_csp_function_term(term)->name;
	    if (add_lydia_symbol("delay") == name) {
		assert(0); /* TODO: Unsupported message. */
	    }
	    if (add_lydia_symbol("on") == name) {
		assert(0); /* TODO: Unsupported message. */
	    }
	    if (add_lydia_symbol("arith_if") == name) {
		assert(to_csp_function_term(term)->args->sz == 3);
		if (!evaluate_bool_term(to_csp_function_term(term)->args->arr[0], domains, variables, constants, known_variables, &c) ||
		    !evaluate_enum_term(to_csp_function_term(term)->args->arr[1], domains, variables, constants, known_variables, &l) ||
		    !evaluate_enum_term(to_csp_function_term(term)->args->arr[2], domains, variables, constants, known_variables, &r)) {
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
			if (!evaluate_enum_term(to_csp_function_term(term)->args->arr[ix + 1], domains, variables, constants, known_variables, &x)) {
			    return 0;
			}
			*result = x;
			return 1;
		    }
		}
		if (csp_termNIL != to_csp_function_term(term)->args->arr[iz - 1]) {
		    if (!evaluate_enum_term(to_csp_function_term(term)->args->arr[iz - 1], domains, variables, constants, known_variables, &x)) {
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
	    assert(constants->arr[to_csp_constant_term(term)->c]->tag == TAGenum_constant);
	    *result = to_enum_constant(constants->arr[to_csp_constant_term(term)->c])->value;
	    break;
	case TAGcsp_variable_term:
	    assert(variables->arr[to_csp_variable_term(term)->v]->tag == TAGenum_variable); /* TODO: Change this to an error. */
	    if (!search_variable_assignment_list(known_variables,
						 to_csp_variable_term(term)->v,
						 &pos)) {
		return 0;
	    }
	    assert(known_variables->arr[pos]->tag == TAGenum_variable_assignment);

	    *result = domains->arr[to_enum_variable(variables->arr[to_csp_variable_term(term)->v])->values_set]->entries->arr[to_enum_variable_assignment(known_variables->arr[pos])->value];
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
