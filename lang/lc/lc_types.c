#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "lcm.h"
#include "lc_types.h"
#include "evalint.h"
#include "evalenum.h"
#include "evalbool.h"
#include "evalfloat.h"

int infer_term_type(csp_term term, variable_list variables, constant_list constants)
{
    lydia_symbol name;

    switch (term->tag) {
        case TAGcsp_function_term:
            name = to_csp_function_term(term)->name;
            if (add_lydia_symbol("lt") == name ||
                add_lydia_symbol("gt") == name ||
                add_lydia_symbol("le") == name ||
                add_lydia_symbol("ge") == name ||
                add_lydia_symbol("ne") == name ||
                add_lydia_symbol("equiv") == name) {
                return TYPEbool;
            }
            assert(to_csp_function_term(term)->args->sz > 0);
/*
 * We take the last argument as in the arithmetic if and switch functions,
 * we have the first argument a boolean condition which is not indicative
 * for the function return type.
 */
            return infer_term_type(to_csp_function_term(term)->args->arr[to_csp_function_term(term)->args->sz - 1], variables, constants);
        case TAGcsp_constant_term:
            switch (constants->arr[to_csp_constant_term(term)->c]->tag) {
                case TAGbool_constant:
                    return TYPEbool;
                case TAGint_constant:
                    return TYPEint;
                case TAGfloat_constant:
                    return TYPEfloat;
                case TAGenum_constant:
                    return TYPEenum;
            }
            assert(0);
        case TAGcsp_variable_term:
            switch (variables->arr[to_csp_variable_term(term)->v]->tag) {
                case TAGbool_variable:
                    return TYPEbool;
                case TAGint_variable:
                    return TYPEint;
                case TAGfloat_variable:
                    return TYPEfloat;
                case TAGenum_variable:
                    return TYPEenum;
            }
    }
    assert(0);
    abort();
}

int terms_lt(csp_term lhs, csp_term rhs, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, int *result)
{
    int l = infer_term_type(lhs, variables, constants);
    int r = infer_term_type(rhs, variables, constants);
    assert(l == r); /* TODO: To error. */
    switch (l) {
        case TYPEint:
            {
                int il, ir;
                if (!evaluate_int_term(lhs, domains, variables, constants, known_variables, &il) ||
                    !evaluate_int_term(rhs, domains, variables, constants, known_variables, &ir)) {
                    return 0;
                }
                *result = (il < ir);
                break;
            }
        case TYPEfloat:
            {
                double fl, fr;
                if (!evaluate_float_term(lhs, domains, variables, constants, known_variables, &fl) ||
                    !evaluate_float_term(rhs, domains, variables, constants, known_variables, &fr)) {
                    return 0;
                }
                *result = (fl < fr);
                break;
            }
        case TYPEbool:
        case TYPEenum:
        default:
            assert(0); /* TODO: To error. */
    }
    return 1;
}

int terms_gt(csp_term lhs, csp_term rhs, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, int *result)
{
    int l = infer_term_type(lhs, variables, constants);
    int r = infer_term_type(rhs, variables, constants);
    assert(l == r); /* TODO: To error. */
    switch (l) {
        case TYPEint:
            {
                int il, ir;
                if (!evaluate_int_term(lhs, domains, variables, constants, known_variables, &il) ||
                    !evaluate_int_term(rhs, domains, variables, constants, known_variables, &ir)) {
                    return 0;
                }
                *result = (il > ir);
                break;
            }
        case TYPEfloat:
            {
                double fl, fr;
                if (!evaluate_float_term(lhs, domains, variables, constants, known_variables, &fl) ||
                    !evaluate_float_term(rhs, domains, variables, constants, known_variables, &fr)) {
                    return 0;
                }
                *result = (fl > fr);
                break;
            }
        case TYPEbool:
        case TYPEenum:
        default:
            assert(0); /* TODO: To error. */
    }
    return 1;
}

int terms_le(csp_term lhs, csp_term rhs, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, int *result)
{
    int l = infer_term_type(lhs, variables, constants);
    int r = infer_term_type(rhs, variables, constants);
    assert(l == r); /* TODO: To error. */
    switch (l) {
        case TYPEint:
            {
                int il, ir;
                if (!evaluate_int_term(lhs, domains, variables, constants, known_variables, &il) ||
                    !evaluate_int_term(rhs, domains, variables, constants, known_variables, &ir)) {
                    return 0;
                }
                *result = (il <= ir);
                break;
            }
        case TYPEfloat:
            {
                double fl, fr;
                if (!evaluate_float_term(lhs, domains, variables, constants, known_variables, &fl) ||
                    !evaluate_float_term(rhs, domains, variables, constants, known_variables, &fr)) {
                    return 0;
                }
                *result = (fl <= fr);
                break;
            }
        case TYPEbool:
        case TYPEenum:
        default:
            assert(0); /* TODO: To error. */
    }
    return 1;
}

int terms_ge(csp_term lhs, csp_term rhs, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, int *result)
{
    int l = infer_term_type(lhs, variables, constants);
    int r = infer_term_type(rhs, variables, constants);
    assert(l == r); /* TODO: To error. */
    switch (l) {
        case TYPEint:
            {
                int il, ir;
                if (!evaluate_int_term(lhs, domains, variables, constants, known_variables, &il) ||
                    !evaluate_int_term(rhs, domains, variables, constants, known_variables, &ir)) {
                    return 0;
                }
                *result = (il >= ir);
                break;
            }
        case TYPEfloat:
            {
                double fl, fr;
                if (!evaluate_float_term(lhs, domains, variables, constants, known_variables, &fl) ||
                    !evaluate_float_term(rhs, domains, variables, constants, known_variables, &fr)) {
                    return 0;
                }
                *result = (fl >= fr);
                break;
            }
        case TYPEbool:
        case TYPEenum:
        default:
            assert(0); /* TODO: To error. */
    }
    return 1;
}

int terms_equiv(csp_term lhs, csp_term rhs, values_set_list domains, variable_list variables, constant_list constants, variable_assignment_list known_variables, int *result)
{
    int l = infer_term_type(lhs, variables, constants);
    int r = infer_term_type(rhs, variables, constants);
    assert(l == r); /* TODO: To error. */
    switch (l) {
        case TYPEbool:
            {
                int bl, br;
                if (!evaluate_bool_term(lhs, domains, variables, constants, known_variables, &bl) ||
                    !evaluate_bool_term(rhs, domains, variables, constants, known_variables, &br)) {
                    return 0;
                }
                *result = (bl == br);
                break;
            }
        case TYPEint:
            {
                int il, ir;
                if (!evaluate_int_term(lhs, domains, variables, constants, known_variables, &il) ||
                    !evaluate_int_term(rhs, domains, variables, constants, known_variables, &ir)) {
                    return 0;
                }
                *result = (il == ir);
                break;
            }
        case TYPEfloat:
            {
                double fl, fr;
                if (!evaluate_float_term(lhs, domains, variables, constants, known_variables, &fl) ||
                    !evaluate_float_term(rhs, domains, variables, constants, known_variables, &fr)) {
                    return 0;
                }
                *result = (fl == fr);
                break;
            }
        case TYPEenum:
            {
                lydia_symbol el, er;
                if (!evaluate_enum_term(lhs, domains, variables, constants, known_variables, &el) ||
                    !evaluate_enum_term(rhs, domains, variables, constants, known_variables, &er)) {
                    return 0;
                }
                *result = (el == er);
                break;
            }
        default:
            assert(0);
    }
    return 1;
}
