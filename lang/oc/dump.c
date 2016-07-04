#include "tv.h"
#include "oc.h"
#include "expr.h"
#include "defs.h"
#include "config.h"
#include "variable.h"

#include <assert.h>

typedef struct str_obs_dump_context {
    values_set_list domains;
    node node;
} obs_dump_context;

identifier make_identifier(obs_variable_identifier name)
{
    register unsigned int ix;
    register unsigned int iy;

    identifier result = new_identifier(lydia_symbolNIL,
                                       int_listNIL,
                                       qualifier_listNIL);

    result->name = name->name->sym;
    if (obs_extent_listNIL != name->ranges) {
        result->indices = new_int_list();
        for (ix = 0; ix < name->ranges->sz; ix++) {
            if (eval_obs_int_expr(name->ranges->arr[ix]->from) !=
                eval_obs_int_expr(name->ranges->arr[ix]->to)) {
                assert(0);
                abort();
            }
            append_int_list(result->indices,
                            eval_obs_int_expr(name->ranges->arr[ix]->from));
        }

    }
    if (obs_variable_qualifier_listNIL != name->qualifiers &&
        name->qualifiers->sz > 0) {
        result->qualifiers = new_qualifier_list();
        for (ix = 0; ix < name->qualifiers->sz; ix++) {
            append_qualifier_list(result->qualifiers,
                                  new_qualifier(name->qualifiers->arr[ix]->name->sym,
                                                int_listNIL));
            if (obs_extent_listNIL != name->qualifiers->arr[ix]->ranges) {
                result->qualifiers->arr[result->qualifiers->sz - 1]->indices = new_int_list();
                for (iy = 0; iy < name->qualifiers->arr[ix]->ranges->sz; iy++) {
                    if (eval_obs_int_expr(name->qualifiers->arr[ix]->ranges->arr[iy]->from) !=
                        eval_obs_int_expr(name->qualifiers->arr[ix]->ranges->arr[iy]->to)) {
                        assert(0);
                        abort();
                    }
                    append_int_list(result->qualifiers->arr[result->qualifiers->sz - 1]->indices,
                                    eval_obs_int_expr(name->qualifiers->arr[ix]->ranges->arr[iy]->from));
                }
            }
        }
    }

    return result;
}

static unsigned int add_domain(obs_dump_context *ctx, lydia_symbol name)
{
    unsigned int pos;

    if (search_values_set_list(ctx->node->constraints->domains, name, &pos)) {
        return (int)pos;
    }
    if (!search_values_set_list(ctx->domains, name, &pos)) {
/* To Do: An internal error. */
        assert(0);
        abort();
    }
    append_values_set_list(ctx->node->constraints->domains,
                           new_values_set(name,
                                          rdup_lydia_symbol_list(ctx->domains->arr[pos]->entries)));

    return ctx->node->constraints->domains->sz - 1;
}

static int add_variable(obs_dump_context *ctx,
                        identifier var,
                        obs_type var_type)
{
    unsigned int pos;
    variable nvar = variableNIL;

    if (search_variable_list(ctx->node->constraints->variables, var, &pos)) {
        return (int)pos;
    }

    switch (var_type->tag) {
        case TAGobs_bool_type:
            nvar = to_variable(new_bool_variable(rdup_identifier(var),
                                                 new_variable_attribute_list(),
                                                 -1));
            break;
        case TAGobs_enum_type:
            nvar = to_variable(new_enum_variable(rdup_identifier(var),
                                                 new_variable_attribute_list(),
                                                 add_domain(ctx, to_obs_enum_type(var_type)->name)));
            break;
        default:
/* To Do: To error. */
            assert(0);
            abort();
            break;
    }

    append_variable_list(ctx->node->constraints->variables, nvar);

    return (int)(ctx->node->constraints->variables->sz - 1);
}

static int add_constant(obs_dump_context *ctx,
                        lydia_symbol name,
                        obs_expr expr)
{
    unsigned int pos;

    constant nconst = constantNIL;

    switch (expr->tag) {
        case TAGobs_expr_bool:
            nconst = to_constant(new_bool_constant(rdup_lydia_symbol(name),
                                                   to_obs_expr_bool(expr)->v));
            break;
        case TAGobs_expr_enum:
            nconst = to_constant(new_enum_constant(rdup_lydia_symbol(name),
                                                   to_obs_expr_enum(expr)->entry,
                                                   add_domain(ctx, to_obs_expr_enum(expr)->name)));
            break;
        default:
            assert(0);
            break;
    }

    if (search_constant_list(ctx->node->constraints->constants,
                             nconst,
                             &pos)) {
        rfre_constant(nconst);
        return (int)pos;
    }

    append_constant_list(ctx->node->constraints->constants, nconst);

    return (int)(ctx->node->constraints->constants->sz - 1);
}

static csp_sentence dump_expr(obs_dump_context *ctx, obs_expr expr)
{
    csp_sentence result = csp_sentenceNIL;
    csp_sentence l, r, x;
    identifier id;
    int idx;

    switch (expr->tag) {
        case TAGobs_expr_enum:
            idx = add_constant(ctx, lydia_symbolNIL, expr);
            result = to_csp_sentence(new_csp_atomic_sentence(to_csp_term(new_csp_constant_term(idx))));
            break;
        case TAGobs_expr_variable:
            id = make_identifier(to_obs_expr_variable(expr)->name);
            idx = add_variable(ctx, id, to_obs_expr_variable(expr)->type);
            result = to_csp_sentence(new_csp_atomic_sentence(to_csp_term(new_csp_variable_term(idx))));
            rfre_identifier(id);
            break;
        case TAGobs_expr_not:
            if (csp_sentenceNIL != (x = dump_expr(ctx, to_obs_expr_not(expr)->x))) {
                result = to_csp_sentence(new_csp_not_sentence(x));
            }
            break;
        case TAGobs_expr_and:
            if (csp_sentenceNIL == (l = dump_expr(ctx, to_obs_expr_and(expr)->l))) {
                break;
            }
            if (csp_sentenceNIL == (r = dump_expr(ctx, to_obs_expr_and(expr)->r))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_and_sentence(l, r));
            break;
        case TAGobs_expr_or:
            if (csp_sentenceNIL == (l = dump_expr(ctx, to_obs_expr_or(expr)->l))) {
                break;
            }
            if (csp_sentenceNIL == (r = dump_expr(ctx, to_obs_expr_or(expr)->r))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_or_sentence(l, r));
            break;
        case TAGobs_expr_imply:
            if (csp_sentenceNIL == (l = dump_expr(ctx, to_obs_expr_imply(expr)->l))) {
                break;
            }
            if (csp_sentenceNIL == (r = dump_expr(ctx, to_obs_expr_imply(expr)->r))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_impl_sentence(l, r));
            break;
        case TAGobs_expr_eq:
            if (csp_sentenceNIL == (l = dump_expr(ctx, to_obs_expr_eq(expr)->l))) {
                break;
            }
            if (csp_sentenceNIL == (r = dump_expr(ctx, to_obs_expr_eq(expr)->r))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_equiv_sentence(l, r));
            break;
        default:
            assert(0);
            abort();
    }

    return result;
}

static node dump_instance(obs_dump_context *ctx, obs_instance instance)
{
    register unsigned int ix;

    csp_sentence_list constraints = new_csp_sentence_list();
    node result = new_node(rdup_lydia_symbol(instance->name->sym),
                           new_edge_list(),
                           to_kb(new_csp(new_values_set_list(),
                                         new_variable_list(),
                                         new_variable_list(),
                                         new_constant_list(),
                                         ENCODING_NONE,
                                         constraints)));

    ctx->node = result;
    for (ix = 0; ix < instance->constraints->sz; ix++) {
        append_csp_sentence_list(constraints,
                                 dump_expr(ctx,
                                           instance->constraints->arr[ix]));
    }

    return result;
}

csp_hierarchy dump(obs_dump dump, values_set_list domains)
{
    register unsigned int ix;

    csp_hierarchy result = new_csp_hierarchy(new_node_list());

    obs_dump_context ctx;

    ctx.domains = domains;

    for (ix = 0; ix < dump->observations->sz; ix++) {
        append_node_list(result->nodes,
                         dump_instance(&ctx, dump->observations->arr[ix]));
    }

    return result;
}
