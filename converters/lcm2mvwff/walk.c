#include <math.h>
#include <assert.h>

#include "hierarchy.h"
#include "variable.h"
#include "walk.h"
#include "mv.h"

values_set_list domains = values_set_listNIL;

static mv_wff_expr create_variable_constant_inequality(variable_list variables,
                                                       constant_list constants,
                                                       csp_variable_term v,
                                                       csp_constant_term c)
{
    enum_variable var;
    enum_constant con;

    values_set domain;

    register unsigned int ix, iy;

    mv_wff_expr result = mv_wff_exprNIL;

/* To Do: Convert the assertions below to errors. */
    assert((variables->arr[v->v]->tag == TAGenum_variable && constants->arr[c->c]->tag == TAGenum_constant) ||
           (variables->arr[v->v]->tag == TAGbool_variable && constants->arr[c->c]->tag == TAGbool_constant));
    if (variables->arr[v->v]->tag == TAGbool_variable) {
        if (to_bool_constant(constants->arr[c->c])->value) {
            return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_FALSE));
        } else {
            return to_mv_wff_expr(new_mv_wff_e_var(v->v, 0));
        }
    }
    var = to_enum_variable(variables->arr[v->v]);
    con = to_enum_constant(constants->arr[c->c]);
    assert(var->values_set == con->values_set);

    domain = domains->arr[var->values_set];

/* To Do: This search here should be looking nicer. */
    for (iy = 0; iy < domain->entries->sz; iy++) {
        if (domain->entries->arr[iy] == con->value) {
            break;
        }
    }
    assert(iy < domain->entries->sz);

    for (ix = 0; ix < iy; ix++) {
        if (result == mv_wff_exprNIL) {
            result = to_mv_wff_expr(new_mv_wff_e_var(v->v, (int)ix));
            continue;
        }
        result = to_mv_wff_expr(new_mv_wff_e_or(result, to_mv_wff_expr(new_mv_wff_e_var(v->v, (int)ix))));
    }

    if (result == mv_wff_exprNIL) {
        return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
    }

    return result;
}

static mv_wff_expr eval_constant_equality(constant_list constants, csp_constant_term c1, csp_constant_term c2)
{
    enum_constant const1, const2;
/* To Do: Convert the three assertions below to errors. */
    assert(constants->arr[c1->c]->tag == TAGenum_constant);
    assert(constants->arr[c2->c]->tag == TAGenum_constant);
    const1 = to_enum_constant(constants->arr[c1->c]);
    const2 = to_enum_constant(constants->arr[c2->c]);
    assert(const1->values_set == const2->values_set);

    if (const1->value == const2->value) {
        return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
    }
    return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_FALSE));
}

static mv_wff_expr eval_constant_inequality(constant_list constants, csp_constant_term c1, csp_constant_term c2)
{
    enum_constant const1, const2;

/* To Do: Convert the three assertions below to errors. */
    assert(constants->arr[c1->c]->tag == TAGenum_constant);
    assert(constants->arr[c2->c]->tag == TAGenum_constant);
    const1 = to_enum_constant(constants->arr[c1->c]);
    const2 = to_enum_constant(constants->arr[c2->c]);
    assert(const1->values_set == const2->values_set);

    if (const1->value < const2->value) {
        return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
    }
    return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_FALSE));
}

static mv_wff_expr create_variable_equality(variable_list variables, csp_variable_term v1, csp_variable_term v2)
{
    mv_wff_expr result = mv_wff_exprNIL;

    register unsigned int ix;

/* To Do: Convert the three assertions below to errors. */
    assert(variables->arr[v1->v]->tag == TAGenum_variable ||
           variables->arr[v1->v]->tag == TAGbool_variable);
    assert(variables->arr[v2->v]->tag == TAGenum_variable ||
           variables->arr[v2->v]->tag == TAGbool_variable);
    assert(variables->arr[v1->v]->tag == variables->arr[v2->v]->tag);

    if (variables->arr[v1->v]->tag == TAGenum_variable) {
        enum_variable var1 = to_enum_variable(variables->arr[v1->v]);
        enum_variable var2 = to_enum_variable(variables->arr[v2->v]);
        values_set domain;
        assert(var1->values_set == var2->values_set);

        domain = domains->arr[var1->values_set];

        for (ix = 0; ix < domain->entries->sz; ix++) {
            mv_wff_expr q = to_mv_wff_expr(new_mv_wff_e_and(to_mv_wff_expr(new_mv_wff_e_var(v1->v, (int)ix)),
                                                            to_mv_wff_expr(new_mv_wff_e_var(v2->v, (int)ix))));
            if (ix == 0) {
                result = q;
                continue;
            }
            result = to_mv_wff_expr(new_mv_wff_e_or(result, q));
        }
    } else {
        result = to_mv_wff_expr(new_mv_wff_e_equiv(to_mv_wff_expr(new_mv_wff_e_var(v1->v, 1)),
                                                   to_mv_wff_expr(new_mv_wff_e_var(v2->v, 1))));
    }

    return result;
}

static mv_wff_expr create_variable_inequality(variable_list variables, csp_variable_term v1, csp_variable_term v2)
{
    mv_wff_expr result = mv_wff_exprNIL;

    register unsigned int ix, iy;

/* To Do: Convert the three assertions below to errors. */
    assert(variables->arr[v1->v]->tag == TAGenum_variable ||
           variables->arr[v1->v]->tag == TAGbool_variable);
    assert(variables->arr[v2->v]->tag == TAGenum_variable ||
           variables->arr[v2->v]->tag == TAGbool_variable);
    assert(variables->arr[v1->v]->tag == variables->arr[v2->v]->tag);

    if (variables->arr[v1->v]->tag == TAGenum_variable) {
        enum_variable var1 = to_enum_variable(variables->arr[v1->v]);
        enum_variable var2 = to_enum_variable(variables->arr[v2->v]);
        values_set domain;

        assert(var1->values_set == var2->values_set);

        domain = domains->arr[var1->values_set];

        if (domain->entries->sz == 1) {
            return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_FALSE));
        }

        for (ix = 0; ix < domain->entries->sz; ix++) {
            mv_wff_expr x, y = mv_wff_exprNIL;
            for (iy = ix + 1; iy < domain->entries->sz; iy++) {
                if (y == mv_wff_exprNIL) {
                    y = to_mv_wff_expr(new_mv_wff_e_var(v2->v, (int)iy));
                    continue;
                }
                y = to_mv_wff_expr(new_mv_wff_e_or(y, to_mv_wff_expr(new_mv_wff_e_var(v2->v, (int)iy))));
            }
            if (y == mv_wff_exprNIL) {
                continue;
            } 
            x = to_mv_wff_expr(new_mv_wff_e_and(to_mv_wff_expr(new_mv_wff_e_var(v1->v, (int)ix)), y));
            if (result == mv_wff_exprNIL) {
                result = x;
                continue;
            }
            result = to_mv_wff_expr(new_mv_wff_e_or(result, x));
        }
    } else {
        result = to_mv_wff_expr(new_mv_wff_e_and(to_mv_wff_expr(new_mv_wff_e_var(v1->v, 0)),
                                                 to_mv_wff_expr(new_mv_wff_e_var(v2->v, 1))));
    }

    return result;
}

static mv_wff_e_var create_variable_constant_equality(variable_list variables,
                                                      constant_list constants,
                                                      csp_variable_term v,
                                                      csp_constant_term c)
{
    enum_variable var;
    enum_constant con;

    values_set domain;

    unsigned int j;

/* To Do: Some hashing here is appropriate. */
/* To Do: Convert the assertions below to errors. */
    assert((variables->arr[v->v]->tag == TAGenum_variable && constants->arr[c->c]->tag == TAGenum_constant) ||
           (variables->arr[v->v]->tag == TAGbool_variable && constants->arr[c->c]->tag == TAGbool_constant));
    if (variables->arr[v->v]->tag == TAGbool_variable) {
        if (to_bool_constant(constants->arr[c->c])->value) {
            return new_mv_wff_e_var(v->v, 1);
        } else {
            return new_mv_wff_e_var(v->v, 0);
        }
    }
    var = to_enum_variable(variables->arr[v->v]);
    con = to_enum_constant(constants->arr[c->c]);
    assert(var->values_set == con->values_set);

    domain = domains->arr[var->values_set];

    for (j = 0; j < domain->entries->sz; j++) {
        if (domain->entries->arr[j] == con->value) {
            break;
        }
    }
    assert(j < domain->entries->sz);

    return new_mv_wff_e_var(v->v, (int)j);
}

static mv_wff_expr walk_term(csp_term term,
                             variable_list variables,
                             constant_list constants)
{
    mv_wff_expr x, y, z;
    mv_wff_expr n, l, r;
    lydia_symbol name;

    switch (term->tag) {
        case TAGcsp_function_term:
            name = to_csp_function_term(term)->name;
            if (add_lydia_symbol("lt") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_constant_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_variable_term)) {
                    return to_mv_wff_expr(create_variable_constant_inequality(variables,
                                                                              constants,
                                                                              to_csp_variable_term(to_csp_function_term(term)->args->arr[1]),
                                                                              to_csp_constant_term(to_csp_function_term(term)->args->arr[0])));
                }
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_variable_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_constant_term)) {
                    return to_mv_wff_expr(create_variable_constant_inequality(variables,
                                                                              constants,
                                                                              to_csp_variable_term(to_csp_function_term(term)->args->arr[0]),
                                                                              to_csp_constant_term(to_csp_function_term(term)->args->arr[1])));
                }
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_variable_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_variable_term)) {
                    return create_variable_inequality(variables,
                                                      to_csp_variable_term(to_csp_function_term(term)->args->arr[0]),
                                                      to_csp_variable_term(to_csp_function_term(term)->args->arr[1]));
                }
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_constant_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_constant_term)) {
                    return eval_constant_inequality(constants,
                                                    to_csp_constant_term(to_csp_function_term(term)->args->arr[0]),
                                                    to_csp_constant_term(to_csp_function_term(term)->args->arr[1]));
                }
/* To Do: It is not clear if this point is unreachable. */
                assert(0);
                abort();
            }
            if (add_lydia_symbol("equiv") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_constant_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_variable_term)) {
                    return to_mv_wff_expr(create_variable_constant_equality(variables,
                                                                            constants,
                                                                            to_csp_variable_term(to_csp_function_term(term)->args->arr[1]),
                                                                            to_csp_constant_term(to_csp_function_term(term)->args->arr[0])));
                }
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_variable_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_constant_term)) {
                    return to_mv_wff_expr(create_variable_constant_equality(variables,
                                                                            constants,
                                                                            to_csp_variable_term(to_csp_function_term(term)->args->arr[0]),
                                                                            to_csp_constant_term(to_csp_function_term(term)->args->arr[1])));
                }
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_variable_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_variable_term)) {
                    return create_variable_equality(variables,
                                                    to_csp_variable_term(to_csp_function_term(term)->args->arr[0]),
                                                    to_csp_variable_term(to_csp_function_term(term)->args->arr[1]));
                }
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_constant_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_constant_term)) {
                    return eval_constant_equality(constants,
                                                  to_csp_constant_term(to_csp_function_term(term)->args->arr[0]),
                                                  to_csp_constant_term(to_csp_function_term(term)->args->arr[1]));
                }
                if (mv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return mv_wff_exprNIL;
                }
                if (mv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_mv_wff_expr(l);
                    return mv_wff_exprNIL;
                }
                return to_mv_wff_expr(new_mv_wff_e_equiv(l, r));
            }
            if (add_lydia_symbol("ne") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_constant_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_variable_term)) {
                    csp_term c = to_csp_function_term(term)->args->arr[0];
                    to_csp_function_term(term)->args->arr[0] = to_csp_function_term(term)->args->arr[1];
                    to_csp_function_term(term)->args->arr[1] = c;
                }
                if ((to_csp_function_term(term)->args->arr[0]->tag == TAGcsp_variable_term) &&
                    (to_csp_function_term(term)->args->arr[1]->tag == TAGcsp_constant_term)) {
/* To Do: Implement this. */
                    assert(0);
                } else {
                    if (mv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                        return mv_wff_exprNIL;
                    }
                    if (mv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                        rfre_mv_wff_expr(l);
                        return mv_wff_exprNIL;
                    }
                    return to_mv_wff_expr(new_mv_wff_e_not(to_mv_wff_expr(new_mv_wff_e_equiv(l, r))));
                }
            }
            if (add_lydia_symbol("not") == name) {
                assert(to_csp_function_term(term)->args->sz == 1);
                if (mv_wff_exprNIL == (n = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return mv_wff_exprNIL;
                }
                return to_mv_wff_expr(new_mv_wff_e_not(n));
            }
            if (add_lydia_symbol("mul") == name || add_lydia_symbol("and") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if (mv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return mv_wff_exprNIL;
                }
                if (mv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_mv_wff_expr(l);
                    return mv_wff_exprNIL;
                }
                return to_mv_wff_expr(new_mv_wff_e_and(l, r));
            }
            if (add_lydia_symbol("add") == name || add_lydia_symbol("or") == name) {
                assert(to_csp_function_term(term)->args->sz == 2);
                if (mv_wff_exprNIL == (l = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return mv_wff_exprNIL;
                }
                if (mv_wff_exprNIL == (r = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_mv_wff_expr(l);
                    return mv_wff_exprNIL;
                }
                return to_mv_wff_expr(new_mv_wff_e_or(l, r));
            }
            if (add_lydia_symbol("arith_switch") == name) {
                int has_default;
                mv_wff_expr lhs, result, def_result;
                unsigned int ix, iz;

                assert(to_csp_function_term(term)->args->sz >= 2);
                has_default = (to_csp_function_term(term)->args->sz % 2 == 0);
                lhs = to_mv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[0], variables, constants));
                if (lhs == mv_wff_exprNIL) {
                    return mv_wff_exprNIL;
                }
                result = mv_wff_exprNIL;
                def_result = mv_wff_exprNIL;
                iz = to_csp_function_term(term)->args->sz;
                for (ix = 1; ix < iz - 1; ix += 2) {
                    mv_wff_expr condition, def_condition;

                    mv_wff_expr rhs, value;
                    if (ix > 1) {
                        lhs = rdup_mv_wff_expr(lhs);
                    }
                    rhs = to_mv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[ix], variables, constants));
                    if (rhs == mv_wff_exprNIL) {
                        rfre_mv_wff_expr(lhs);
                        rfre_mv_wff_expr(result);
                        rfre_mv_wff_expr(def_result);
                        return mv_wff_exprNIL;
                    }
                    value = to_mv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[ix + 1], variables, constants));
                    if (value == mv_wff_exprNIL) {
                        rfre_mv_wff_expr(rhs);
                        rfre_mv_wff_expr(lhs);
                        rfre_mv_wff_expr(result);
                        rfre_mv_wff_expr(def_result);
                        return mv_wff_exprNIL;
                    }
/**
 * Check for a constant condition and fold it if this is the case. We have
 * to deal only with the multi-valued case because the Booleans will be
 * folded by the appropriate procedure.
 */
                    if (lhs->tag == TAGcsp_constant_term &&
                        rhs->tag == TAGcsp_constant_term) {
/* To Do: Implement this. */
                        assert(0);
                    }
                    condition = to_mv_wff_expr(new_mv_wff_e_impl(to_mv_wff_expr(new_mv_wff_e_equiv(lhs, rhs)), value));
                    def_condition = to_mv_wff_expr(new_mv_wff_e_not(to_mv_wff_expr(new_mv_wff_e_equiv(rdup_mv_wff_expr(lhs), rdup_mv_wff_expr(rhs)))));
                    if (mv_wff_exprNIL == result) {
                        result = condition;
                        def_result = def_condition;
                        continue;
                    }
                    result = to_mv_wff_expr(new_mv_wff_e_and(result, condition));
                    def_result = to_mv_wff_expr(new_mv_wff_e_and(def_result, def_condition));
                }

                if (has_default) {
                    mv_wff_expr def_value = to_mv_wff_expr(walk_term(to_csp_function_term(term)->args->arr[iz - 1], variables, constants));
                    if (mv_wff_exprNIL == def_value) {
                        rfre_mv_wff_expr(result);
                        rfre_mv_wff_expr(def_result);
                        return mv_wff_exprNIL;
                    }
                    if (mv_wff_exprNIL == result && 
                        mv_wff_exprNIL == def_result) {
                        rfre_mv_wff_expr(result);
                        rfre_mv_wff_expr(def_result);
                        return def_value;
                    }
                    result = to_mv_wff_expr(new_mv_wff_e_and(result, to_mv_wff_expr(new_mv_wff_e_impl(def_result, def_value))));
                }

                return result;
            }
            if (add_lydia_symbol("arith_if") == name) {
                assert(to_csp_function_term(term)->args->sz == 3);
                if (mv_wff_exprNIL == (x = walk_term(to_csp_function_term(term)->args->arr[0], variables, constants))) {
                    return mv_wff_exprNIL;
                }
                if (mv_wff_exprNIL == (y = walk_term(to_csp_function_term(term)->args->arr[1], variables, constants))) {
                    rfre_mv_wff_expr(x);
                    return mv_wff_exprNIL;
                }
                if (mv_wff_exprNIL == (z = walk_term(to_csp_function_term(term)->args->arr[2], variables, constants))) {
                    rfre_mv_wff_expr(x);
                    rfre_mv_wff_expr(y);
                    return mv_wff_exprNIL;
                }
                return to_mv_wff_expr(new_mv_wff_e_and(to_mv_wff_expr(new_mv_wff_e_or(to_mv_wff_expr(new_mv_wff_e_not(x)), y)),
                                                       to_mv_wff_expr(new_mv_wff_e_or(rdup_mv_wff_expr(x), z))));
            }
            break;
        case TAGcsp_constant_term:
            assert(constants->arr[to_csp_constant_term(term)->c]->tag == TAGbool_constant);
            return to_mv_wff_expr(new_mv_wff_e_const(to_bool_constant(constants->arr[to_csp_constant_term(term)->c])->value));
        case TAGcsp_variable_term:
            if (TAGbool_variable == variables->arr[to_csp_variable_term(term)->v]->tag) {
                return to_mv_wff_expr(new_mv_wff_e_var(to_csp_variable_term(term)->v, 1));
            } else if (TAGenum_variable == variables->arr[to_csp_variable_term(term)->v]->tag) {
                unsigned int domain = to_enum_variable(variables->arr[to_csp_variable_term(term)->v])->values_set;
                if (add_lydia_symbol("bool") == domains->arr[domain]->name) {
                    return to_mv_wff_expr(new_mv_wff_e_var(to_csp_variable_term(term)->v, 1));
                }
                assert(0);
                abort();
            } else {
                assert(0);
                abort();
            }
    }
    return mv_wff_exprNIL;
}

static mv_wff_expr walk_sentence(csp_sentence sentence,
                                 variable_list variables,
                                 constant_list constants)
{
    mv_wff_expr n, l, r;

    switch (sentence->tag) {
        case TAGcsp_not_sentence:
            if (mv_wff_exprNIL == (n = to_mv_wff_expr(walk_sentence(to_csp_not_sentence(sentence)->n, variables, constants)))) {
                return mv_wff_exprNIL;
            }
            return to_mv_wff_expr(new_mv_wff_e_not(n));
        case TAGcsp_and_sentence:
            if (mv_wff_exprNIL == (l = to_mv_wff_expr(walk_sentence(to_csp_and_sentence(sentence)->lhs, variables, constants)))) {
                return mv_wff_exprNIL;
            }
            if (mv_wff_exprNIL == (r = to_mv_wff_expr(walk_sentence(to_csp_and_sentence(sentence)->rhs, variables, constants)))) {
                rfre_mv_wff_expr(l);
                return mv_wff_exprNIL;
            }
            return to_mv_wff_expr(new_mv_wff_e_and(l, r));
        case TAGcsp_or_sentence:
            if (mv_wff_exprNIL == (l = to_mv_wff_expr(walk_sentence(to_csp_or_sentence(sentence)->lhs, variables, constants)))) {
                return mv_wff_exprNIL;
            }
            if (mv_wff_exprNIL == (r = to_mv_wff_expr(walk_sentence(to_csp_or_sentence(sentence)->rhs, variables, constants)))) {
                rfre_mv_wff_expr(l);
                return mv_wff_exprNIL;
            }
            return to_mv_wff_expr(new_mv_wff_e_or(l, r));
        case TAGcsp_impl_sentence:
            if (mv_wff_exprNIL == (l = to_mv_wff_expr(walk_sentence(to_csp_impl_sentence(sentence)->lhs, variables, constants)))) {
                return mv_wff_exprNIL;
            }
            if (mv_wff_exprNIL == (r = to_mv_wff_expr(walk_sentence(to_csp_impl_sentence(sentence)->rhs, variables, constants)))) {
                rfre_mv_wff_expr(l);
                return mv_wff_exprNIL;
            }
            return to_mv_wff_expr(new_mv_wff_e_impl(l, r));
        case TAGcsp_equiv_sentence:
            if (to_csp_equiv_sentence(sentence)->lhs->tag == TAGcsp_atomic_sentence &&
                to_csp_equiv_sentence(sentence)->rhs->tag == TAGcsp_atomic_sentence) {
                if (to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a->tag == TAGcsp_constant_term &&
                    to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a->tag == TAGcsp_constant_term) {
                    return eval_constant_equality(constants,
                                                  to_csp_constant_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a),
                                                  to_csp_constant_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a));
                }
                if (to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a->tag == TAGcsp_variable_term &&
                    to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a->tag == TAGcsp_variable_term) {
                    return create_variable_equality(variables,
                                                    to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a),
                                                    to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a));
                }
                if (to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a->tag == TAGcsp_variable_term &&
                    to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a->tag == TAGcsp_constant_term) {
                    return to_mv_wff_expr(create_variable_constant_equality(variables,
                                                                            constants,
                                                                            to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a),
                                                                            to_csp_constant_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a)));
                }
                if (to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a->tag == TAGcsp_constant_term &&
                    to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a->tag == TAGcsp_variable_term) {
                    return to_mv_wff_expr(create_variable_constant_equality(variables,
                                                                            constants,
                                                                            to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a),
                                                                            to_csp_constant_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a)));
                }
            }
            if (mv_wff_exprNIL == (l = to_mv_wff_expr(walk_sentence(to_csp_equiv_sentence(sentence)->lhs, variables, constants)))) {
                return mv_wff_exprNIL;
            }
            if (mv_wff_exprNIL == (r = to_mv_wff_expr(walk_sentence(to_csp_equiv_sentence(sentence)->rhs, variables, constants)))) {
                rfre_mv_wff_expr(l);
                return mv_wff_exprNIL;
            }
            return to_mv_wff_expr(new_mv_wff_e_equiv(l, r));
        case TAGcsp_lt_sentence:
            if (to_csp_lt_sentence(sentence)->lhs->tag == TAGcsp_atomic_sentence &&
                to_csp_lt_sentence(sentence)->rhs->tag == TAGcsp_atomic_sentence) {
                if (to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->lhs)->a->tag == TAGcsp_constant_term &&
                    to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->rhs)->a->tag == TAGcsp_constant_term) {
                    return eval_constant_inequality(constants,
                                                    to_csp_constant_term(to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->lhs)->a),
                                                    to_csp_constant_term(to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->rhs)->a));
                }
                if (to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->lhs)->a->tag == TAGcsp_variable_term &&
                    to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->rhs)->a->tag == TAGcsp_variable_term) {
                    return create_variable_inequality(variables,
                                                      to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a),
                                                      to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a));
                }
                if (to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->lhs)->a->tag == TAGcsp_variable_term &&
                    to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->rhs)->a->tag == TAGcsp_constant_term) {
                    return create_variable_constant_inequality(variables,
                                                               constants,
                                                               to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a),
                                                               to_csp_constant_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a));
                }
                if (to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->lhs)->a->tag == TAGcsp_constant_term &&
                    to_csp_atomic_sentence(to_csp_lt_sentence(sentence)->rhs)->a->tag == TAGcsp_variable_term) {
                    return create_variable_constant_inequality(variables,
                                                               constants,
                                                               to_csp_variable_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->rhs)->a),
                                                               to_csp_constant_term(to_csp_atomic_sentence(to_csp_equiv_sentence(sentence)->lhs)->a));
                }
            }
/* To Do: It is not clear if this point is unreachable. */
            assert(0);
            abort();
            break;
        case TAGcsp_atomic_sentence:
            return to_mv_wff_expr(walk_term(to_csp_atomic_sentence(sentence)->a, variables, constants));
    }
    return mv_wff_exprNIL;
}

node convert_mv_model(node sys)
{
    unsigned int i;

    node result = new_node(rdup_lydia_symbol(sys->type),
                           edge_listNIL,
                           to_kb(new_mv_wff(rdup_values_set_list(sys->constraints->domains),
                                            new_variable_list(),
                                            new_variable_list(),
                                            new_constant_list(),
                                            ENCODING_NONE,
                                            new_mv_wff_expr_list())));
    domains = result->constraints->domains;

    for (i = 0; i < sys->constraints->variables->sz; i++) {
        if (sys->constraints->variables->arr[i]->tag != TAGbool_variable &&
            sys->constraints->variables->arr[i]->tag != TAGenum_variable) {
            rfre_node(result);
            return nodeNIL;
        }
        result->constraints->variables = append_variable_list(result->constraints->variables, rdup_variable(sys->constraints->variables->arr[i]));
    }
    for (i = 0; i < sys->constraints->constants->sz; i++) {
        if (sys->constraints->constants->arr[i]->tag != TAGbool_constant &&
            sys->constraints->constants->arr[i]->tag != TAGenum_constant) {
            rfre_node(result);
            return nodeNIL;
        }
    }
    if (sys->edges != edge_listNIL) {
        result->edges = rdup_edge_list(sys->edges);
    }
    for (i = 0; i < to_csp(sys->constraints)->sentences->sz; i++) {
        mv_wff_expr expr = walk_sentence(to_csp(sys->constraints)->sentences->arr[i], sys->constraints->variables, sys->constraints->constants);
        if (mv_wff_exprNIL == expr) {
            rfre_node(result);
            return nodeNIL;
        }
        append_mv_wff_expr_list(to_mv_wff(result->constraints)->e, expr);
    }

    replace_bool_variables(result->constraints->variables,
                           result->constraints->domains);

    return result;
}
