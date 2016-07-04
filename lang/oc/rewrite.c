#include "tv.h"
#include "oc.h"
#include "dump.h"
#include "error.h"
#include "config.h"
#include "variable.h"

#include "variable.h"

#include <assert.h>

typedef struct str_obs_rewrite_context {
    values_set_list domains;
    variable_list variables;
} obs_rewrite_context;

static obs_expr rewrite_expr(obs_rewrite_context *ctx, obs_expr expr)
{
    unsigned int pos;
    identifier var;

    switch (expr->tag) {
        case TAGobs_expr_variable:
            var = make_identifier(to_obs_expr_variable(expr)->name);
            if (var->qualifiers != qualifier_listNIL &&
                var->qualifiers->sz == 1) {
                lydia_symbol name = var->qualifiers->arr[0]->name;
                lydia_symbol entry = var->name;
                if (search_values_set_list(ctx->domains, name, &pos)) {
                    rfre_obs_expr_variable(to_obs_expr_variable(expr));
                    expr = to_obs_expr(new_obs_expr_enum(obs_origNIL,
                                                         obs_typeNIL,
                                                         name,
                                                         entry));
                }
            }
            rfre_identifier(var);
            break;
        case TAGobs_expr_not:
            to_obs_expr_not(expr)->x = rewrite_expr(ctx, to_obs_expr_not(expr)->x);
            break;
        case TAGobs_expr_and:
            to_obs_expr_and(expr)->l = rewrite_expr(ctx, to_obs_expr_and(expr)->l);
            to_obs_expr_and(expr)->r = rewrite_expr(ctx, to_obs_expr_and(expr)->r);
            break;
        case TAGobs_expr_or:
            to_obs_expr_or(expr)->l = rewrite_expr(ctx, to_obs_expr_or(expr)->l);
            to_obs_expr_or(expr)->r = rewrite_expr(ctx, to_obs_expr_or(expr)->r);
            break;
        case TAGobs_expr_imply:
            to_obs_expr_imply(expr)->l = rewrite_expr(ctx, to_obs_expr_imply(expr)->l);
            to_obs_expr_imply(expr)->r = rewrite_expr(ctx, to_obs_expr_imply(expr)->r);
            break;
        case TAGobs_expr_eq:
            to_obs_expr_eq(expr)->l = rewrite_expr(ctx, to_obs_expr_eq(expr)->l);
            to_obs_expr_eq(expr)->r = rewrite_expr(ctx, to_obs_expr_eq(expr)->r);
            break;
        default:
            assert(0);
            abort();
    }

    return expr;
}

static obs_instance rewrite_instance(obs_rewrite_context *ctx, obs_instance instance)
{
    register unsigned int ix;

    for (ix = 0; ix < instance->constraints->sz; ix++) {
        instance->constraints->arr[ix] = rewrite_expr(ctx, instance->constraints->arr[ix]);
    }

    return instance;
}

obs_dump rewrite_dump(obs_dump dump,
                      values_set_list domains,
                      variable_list variables)
{
    register unsigned int ix;

    obs_rewrite_context ctx;

    ctx.domains = domains;
    ctx.variables = variables;

    for (ix = 0; ix < dump->observations->sz; ix++) {
        dump->observations->arr[ix] = rewrite_instance(&ctx, dump->observations->arr[ix]);
    }

    return dump;
}
