#include "hierarchy.h"
#include "variable.h"
#include "lcm.h"
#include "tv.h"

#include <assert.h>
#include <math.h>

static tv_wff_expr walk_term(csp_term term, variable_list variables, constant_list constants)
{
    tv_wff_expr x, y, z;
    tv_wff_expr n, l, r;
    tv_wff_expr lhs, rhs, value, def_value, condition, def_condition, result, def_result;
    lydia_symbol name;

    unsigned int ix, iz;

    int has_default;

    switch (term->tag) {
        case TAGcsp_function_term:
            name = to_csp_function_term(term)->name;
            if (add_lydia_symbol("equiv") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if (tv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return tv_wff_exprNIL;
                }
                if (tv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_tv_wff_expr(l);
                    return tv_wff_exprNIL;
                }
                return to_tv_wff_expr(new_tv_wff_e_equiv(l, r));
            }
            if (add_lydia_symbol("ne") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if (tv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return tv_wff_exprNIL;
                }
                if (tv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_tv_wff_expr(l);
                    return tv_wff_exprNIL;
                }
                return to_tv_wff_expr(new_tv_wff_e_not(to_tv_wff_expr(new_tv_wff_e_equiv(l, r))));
            }
            if (add_lydia_symbol("not") == name) {
                assert(to_csp_function_term(term)->args->sz == 1);
                if (tv_wff_exprNIL == (n = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return tv_wff_exprNIL;
                }
                return to_tv_wff_expr(new_tv_wff_e_not(n));
            }
            if (add_lydia_symbol("lt") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if (tv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return tv_wff_exprNIL;
                }
                if (tv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_tv_wff_expr(l);
                    return tv_wff_exprNIL;
                }
                return to_tv_wff_expr(new_tv_wff_e_and(to_tv_wff_expr(new_tv_wff_e_not(l)), r));
            }
            if (add_lydia_symbol("mul") == name || add_lydia_symbol("and") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if (tv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return tv_wff_exprNIL;
                }
                if (tv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_tv_wff_expr(l);
                    return tv_wff_exprNIL;
                }
                return to_tv_wff_expr(new_tv_wff_e_and(l, r));
            }
            if (add_lydia_symbol("add") == name || add_lydia_symbol("or") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if (tv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return tv_wff_exprNIL;
                }
                if (tv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_tv_wff_expr(l);
                    return tv_wff_exprNIL;
                }
                return to_tv_wff_expr(new_tv_wff_e_or(l, r));
            }
            if (add_lydia_symbol("arith_switch") == name) {
                assert(to_csp_function_term(term)->args->sz >= 2);
                has_default = (to_csp_function_term(term)->args->sz % 2 == 0);
                lhs = to_tv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[0], variables, constants));
                if (lhs == tv_wff_exprNIL) {
                    return tv_wff_exprNIL;
                }
                result = tv_wff_exprNIL;
                def_result = tv_wff_exprNIL;
                iz = to_csp_function_term(term)->args->sz;
                for (ix = 1; ix < iz - 1; ix += 2) {
                    if (ix > 1) {
                        lhs = rdup_tv_wff_expr(lhs);
                    }
                    rhs = to_tv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[ix], variables, constants));
                    if (rhs == tv_wff_exprNIL) {
                        rfre_tv_wff_expr(lhs);
                        rfre_tv_wff_expr(result);
                        rfre_tv_wff_expr(def_result);
                        return tv_wff_exprNIL;
                    }
                    value = to_tv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[ix + 1], variables, constants));
                    if (value == tv_wff_exprNIL) {
                        rfre_tv_wff_expr(rhs);
                        rfre_tv_wff_expr(lhs);
                        rfre_tv_wff_expr(result);
                        rfre_tv_wff_expr(def_result);
                        return tv_wff_exprNIL;
                    }
                    condition = to_tv_wff_expr(new_tv_wff_e_impl(to_tv_wff_expr(new_tv_wff_e_equiv(lhs, rhs)), value));
                    def_condition = to_tv_wff_expr(new_tv_wff_e_not(to_tv_wff_expr(new_tv_wff_e_equiv(rdup_tv_wff_expr(lhs), rdup_tv_wff_expr(rhs)))));
                    if (tv_wff_exprNIL == result) {
                        result = condition;
                        def_result = def_condition;
                        continue;
                    }
                    result = to_tv_wff_expr(new_tv_wff_e_and(result, condition));
                    def_result = to_tv_wff_expr(new_tv_wff_e_and(def_result, def_condition));
                }

                if (has_default) {
                    def_value = to_tv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[iz - 1], variables, constants));
                    if (tv_wff_exprNIL == def_value) {
                        rfre_tv_wff_expr(result);
                        rfre_tv_wff_expr(def_result);
                        return tv_wff_exprNIL;
                    }
                    if (tv_wff_exprNIL == result && 
                        tv_wff_exprNIL == def_result) {
                        rfre_tv_wff_expr(result);
                        rfre_tv_wff_expr(def_result);
                        return def_value;
                    }
                    return to_tv_wff_expr(new_tv_wff_e_and(result, to_tv_wff_expr(new_tv_wff_e_impl(def_result, def_value))));
                }

                return result;
            }
            if (add_lydia_symbol("arith_if") == name) {
                assert(to_csp_function_term(term)->args->sz == 3);
                if (tv_wff_exprNIL == (x = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return tv_wff_exprNIL;
                }
                if (tv_wff_exprNIL == (y = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_tv_wff_expr(x);
                    return tv_wff_exprNIL;
                }
                if (tv_wff_exprNIL == (z = walk_term(to_csp_function_term(term)->args->arr[2], variables, constants))) {
                    rfre_tv_wff_expr(x);
                    rfre_tv_wff_expr(y);
                    return tv_wff_exprNIL;
                }
                return to_tv_wff_expr(new_tv_wff_e_and(to_tv_wff_expr(new_tv_wff_e_or(to_tv_wff_expr(new_tv_wff_e_not(x)), y)),
                                                       to_tv_wff_expr(new_tv_wff_e_or(rdup_tv_wff_expr(x), z))));
            }
            break;
        case TAGcsp_constant_term:
            assert(constants->arr[to_csp_constant_term(term)->c]->tag == TAGbool_constant);
            return to_tv_wff_expr(new_tv_wff_e_const(to_bool_constant(constants->arr[to_csp_constant_term(term)->c])->value));
        case TAGcsp_variable_term:
            assert(variables->arr[to_csp_variable_term(term)->v]->tag == TAGbool_variable);
            return to_tv_wff_expr(new_tv_wff_e_var(to_csp_variable_term(term)->v));
    }
    return tv_wff_exprNIL;
}

tv_wff_expr walk_tv_sentence(csp_sentence sentence, variable_list variables, constant_list constants)
{
    tv_wff_expr n, l, r;

    switch (sentence->tag) {
        case TAGcsp_not_sentence:
            if (tv_wff_exprNIL == (n = to_tv_wff_expr(walk_tv_sentence(to_csp_not_sentence(sentence)->n, variables, constants)))) {
                return tv_wff_exprNIL;
            }
            return to_tv_wff_expr(new_tv_wff_e_not(n));
        case TAGcsp_lt_sentence:
            if (tv_wff_exprNIL == (l = to_tv_wff_expr(walk_tv_sentence(to_csp_lt_sentence(sentence)->lhs, variables, constants)))) {
                return tv_wff_exprNIL;
            }
            if (tv_wff_exprNIL == (r = to_tv_wff_expr(walk_tv_sentence(to_csp_lt_sentence(sentence)->rhs, variables, constants)))) {
                rfre_tv_wff_expr(l);
                return tv_wff_exprNIL;
            }
            return to_tv_wff_expr(new_tv_wff_e_and(to_tv_wff_expr(new_tv_wff_e_not(l)), r));
        case TAGcsp_and_sentence:
            if (tv_wff_exprNIL == (l = to_tv_wff_expr(walk_tv_sentence(to_csp_and_sentence(sentence)->lhs, variables, constants)))) {
                return tv_wff_exprNIL;
            }
            if (tv_wff_exprNIL == (r = to_tv_wff_expr(walk_tv_sentence(to_csp_and_sentence(sentence)->rhs, variables, constants)))) {
                rfre_tv_wff_expr(l);
                return tv_wff_exprNIL;
            }
            return to_tv_wff_expr(new_tv_wff_e_and(l, r));
        case TAGcsp_or_sentence:
            if (tv_wff_exprNIL == (l = to_tv_wff_expr(walk_tv_sentence(to_csp_or_sentence(sentence)->lhs, variables, constants)))) {
                return tv_wff_exprNIL;
            }
            if (tv_wff_exprNIL == (r = to_tv_wff_expr(walk_tv_sentence(to_csp_or_sentence(sentence)->rhs, variables, constants)))) {
                rfre_tv_wff_expr(l);
                return tv_wff_exprNIL;
            }
            return to_tv_wff_expr(new_tv_wff_e_or(l, r));
        case TAGcsp_impl_sentence:
            if (tv_wff_exprNIL == (l = to_tv_wff_expr(walk_tv_sentence(to_csp_impl_sentence(sentence)->lhs, variables, constants)))) {
                return tv_wff_exprNIL;
            }
            if (tv_wff_exprNIL == (r = to_tv_wff_expr(walk_tv_sentence(to_csp_impl_sentence(sentence)->rhs, variables, constants)))) {
                rfre_tv_wff_expr(l);
                return tv_wff_exprNIL;
            }
            return to_tv_wff_expr(new_tv_wff_e_impl(l, r));
        case TAGcsp_equiv_sentence:
            if (tv_wff_exprNIL == (l = to_tv_wff_expr(walk_tv_sentence(to_csp_equiv_sentence(sentence)->lhs, variables, constants)))) {
                return tv_wff_exprNIL;
            }
            if (tv_wff_exprNIL == (r = to_tv_wff_expr(walk_tv_sentence(to_csp_equiv_sentence(sentence)->rhs, variables, constants)))) {
                rfre_tv_wff_expr(l);
                return tv_wff_exprNIL;
            }
            return to_tv_wff_expr(new_tv_wff_e_equiv(l, r));
        case TAGcsp_atomic_sentence:
            return to_tv_wff_expr(walk_term(to_csp_atomic_sentence(sentence)->a, variables, constants));
    }
    return tv_wff_exprNIL;
}
