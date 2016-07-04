#include "config.h"
#include "search.h"
#include "oc.h"

#include <assert.h>
#include <math.h>

int eval_obs_int_expr(obs_expr e)
{
    assert(obs_exprNIL != e);

    switch (e->tag) {
        case TAGobs_expr_variable:
            assert(0);
            break;

/* Literals: */
        case TAGobs_expr_int:
            return to_obs_expr_int(e)->v;
        case TAGobs_expr_bool:
        case TAGobs_expr_enum:
/* Noop. */
            break;

/* Binary expressions: */
        case TAGobs_expr_and:
        case TAGobs_expr_or:
        case TAGobs_expr_imply:
        case TAGobs_expr_lt:
            return eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->l)) <
                   eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->r));
        case TAGobs_expr_gt:
            return eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->l)) >
                   eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->r));
        case TAGobs_expr_le:
            return eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->l)) <=
                   eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->r));
        case TAGobs_expr_ge:
            return eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->l)) >=
                   eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->r));
        case TAGobs_expr_ne:
            return eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->l)) !=
                   eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->r));
        case TAGobs_expr_eq:
            return eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->l)) ==
                   eval_obs_int_expr(to_obs_expr(to_obs_expr_binop(e)->r));

/* Unary expressions: */
        case TAGobs_expr_not:
            break;

/* Others: */
        case TAGobs_expr_if_else:
            break;
        case TAGobs_expr_cond:
            break;
        case TAGobs_expr_concatenation:
            break;
    };

    assert(0);
    abort();
}
