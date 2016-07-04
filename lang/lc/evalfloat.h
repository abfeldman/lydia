#ifndef __EVALFLOAT_H__
#define __EVALFLOAT_H__

#include "lcm.h"

int evaluate_float_term(csp_term term, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, double *result);

#endif

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
