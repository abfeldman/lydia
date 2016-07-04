#include "tv.h"
#include "oc.h"
#include "dump.h"
#include "error.h"
#include "config.h"
#include "variable.h"

#include "variable.h"

#include <assert.h>

typedef struct str_obs_typeinfer_context {
    values_set_list domains;
    variable_list variables;
    lydia_symbol_list undefined;
} obs_typeinfer_context;

static int member_lydia_symbol_list(const_lydia_symbol_list symbols,
                                    lydia_symbol symbol)
{
    register unsigned int ix;

    for (ix = 0; ix < symbols->sz; ix++) {
        if (symbols->arr[ix] == symbol) {
            return 1;
        }
    }

    return 0;
}

static int infer_types_expr(obs_typeinfer_context *ctx,
                            obs_expr expr,
                            lydia_symbol location)
{
    int result = 1;

    unsigned int pos;
    identifier var;

    switch (expr->tag) {
        case TAGobs_expr_variable:
            var = make_identifier(to_obs_expr_variable(expr)->name);
            if (search_variable_list(ctx->variables, var, &pos)) {
                rfre_identifier(var);
                if (TAGbool_variable == ctx->variables->arr[pos]->tag) {
                    to_obs_expr_variable(expr)->type = to_obs_type(new_obs_bool_type());
                    break;
                }
                if (TAGenum_variable == ctx->variables->arr[pos]->tag) {
                    values_set name = ctx->domains->arr[to_enum_variable(ctx->variables->arr[pos])->values_set];
                    to_obs_expr_variable(expr)->type = to_obs_type(new_obs_enum_type(name->name));
                    break;
                }
                assert(0);
                abort();
            }
            if (var->qualifiers != qualifier_listNIL &&
                var->qualifiers->sz == 1) {
/* To Do: Check all the indices. */
                lydia_symbol domain = var->qualifiers->arr[0]->name;
                if (search_values_set_list(ctx->domains, domain, &pos)) {
/* To Do: Check the entry. */
                    to_obs_expr_variable(expr)->type = to_obs_type(new_obs_enum_type(domain));
                    break;
                }
            }
            rfre_identifier(var);

            result = 0;

            if (!member_lydia_symbol_list(ctx->undefined, to_obs_expr_variable(expr)->name->name->sym)) {
                loeh_error(ERR_UNDEFINED_IDENTIFIER,
                           LOEH_LOCATION_OBSERVATION,
                           to_obs_expr_variable(expr)->name->name->org,
                           location->name,
                           to_obs_expr_variable(expr)->name->name->sym->name);
                append_lydia_symbol_list(ctx->undefined, to_obs_expr_variable(expr)->name->name->sym);
            }
            break;
        case TAGobs_expr_not:
            result &= infer_types_expr(ctx, to_obs_expr_not(expr)->x, location);
            break;
        case TAGobs_expr_and:
            result &= infer_types_expr(ctx, to_obs_expr_and(expr)->l, location);
            result &= infer_types_expr(ctx, to_obs_expr_and(expr)->r, location);
            break;
        case TAGobs_expr_or:
            result &= infer_types_expr(ctx, to_obs_expr_or(expr)->l, location);
            result &= infer_types_expr(ctx, to_obs_expr_or(expr)->r, location);
            break;
        case TAGobs_expr_imply:
            result &= infer_types_expr(ctx, to_obs_expr_imply(expr)->l, location);
            result &= infer_types_expr(ctx, to_obs_expr_imply(expr)->r, location);
            break;
        case TAGobs_expr_eq:
            result &= infer_types_expr(ctx, to_obs_expr_eq(expr)->l, location);
            result &= infer_types_expr(ctx, to_obs_expr_eq(expr)->r, location);
            break;
        default:
            assert(0);
            abort();
    }

    return result;
}

static int infer_types_instance(obs_typeinfer_context *ctx,
                                obs_instance instance)
{
    int result = 1;

    register unsigned int ix;

    for (ix = 0; ix < instance->constraints->sz; ix++) {
        ctx->undefined = new_lydia_symbol_list();
        result &= infer_types_expr(ctx,
                                   instance->constraints->arr[ix],
                                   instance->name->sym);
        rfre_lydia_symbol_list(ctx->undefined);
    }

    return result;
}

int infer_types_dump(obs_dump dump,
                     values_set_list domains,
                     variable_list variables)
{
    int result = 1;

    register unsigned int ix;

    obs_typeinfer_context ctx;

    ctx.domains = domains;
    ctx.variables = variables;

    for (ix = 0; ix < dump->observations->sz; ix++) {
        result &= infer_types_instance(&ctx, dump->observations->arr[ix]);
    }

    return result;
}
