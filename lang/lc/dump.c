#include "ast.h"
#include "hash.h"
#include "iter.h"
#include "attr.h"
#include "expr.h"
#include "types.h"
#include "error.h"
#include "array.h"
#include "strdup.h"
#include "config.h"
#include "search.h"
#include "rewrite.h"
#include "evalint.h"
#include "evalbool.h"
#include "evalenum.h"
#include "variable.h"
#include "hierarchy.h"
#include "evalfloat.h"

#include <assert.h>

typedef struct str_csp_dumper_context {
    csp_hierarchy result;
    node node;
    hash_table variable_hash;
    csp_sentence_list sentences;
    system_definition system;
    definition_list systems;
    const_user_type_entry_list user_type_table;
    const_attribute_entry_list attribute_table;
    int rc;
    origin org;
} csp_dumper_context;

/* Function prototypes. */
static csp_term walk_term(csp_dumper_context *ctx, expr ex);

static void add_variable_hash(hash_table *hash,
                              const_identifier var,
                              lydia_symbol node,
                              const unsigned int pos)
{
    char *var_name;
    char *new_var_name;

    var_name = get_variable_name(var);
    new_var_name = realloc(var_name, strlen(var_name) + strlen(node->name) + 2);
    if (NULL == new_var_name) {
        assert(0);
        abort();
    }
    strcat(new_var_name, ".");
    strcat(new_var_name, node->name);

    hash_add(hash,
             new_var_name,
             (unsigned int)strlen(new_var_name),
             (void *)&pos,
             sizeof(unsigned int),
             NULL);

    free(new_var_name);
}

static void delete_variable_hash(hash_table *hash,
                                 const_identifier var,
                                 lydia_symbol node)
{
    char *var_name;
    char *new_var_name;

    var_name = get_variable_name(var);
    new_var_name = realloc(var_name, strlen(var_name) + strlen(node->name) + 2);
    if (NULL == new_var_name) {
        assert(0);
        abort();
    }
    strcat(new_var_name, ".");
    strcat(new_var_name, node->name);
    hash_del(hash, new_var_name, (unsigned int)strlen(new_var_name));
    free(new_var_name);
}

static int find_variable_hash(hash_table *hash,
                              const_identifier var,
                              lydia_symbol node,
                              unsigned int *pos)
{
    void *location;
    char *var_name;
    char *new_var_name;

    var_name = get_variable_name(var);
    new_var_name = realloc(var_name, strlen(var_name) + strlen(node->name) + 2);
    if (NULL == new_var_name) {
        assert(0);
        abort();
    }
    strcat(new_var_name, ".");
    strcat(new_var_name, node->name);
    if (0 == hash_find(hash,
                       new_var_name,
                       (unsigned int)strlen(new_var_name),
                       &location)) {
        *pos = *(unsigned int *)location;

        free(new_var_name);

        return 1;
    }

    free(new_var_name);

    return 0;
}

static int find_variable_name_hash(hash_table *hash,
                                   lydia_symbol var,
                                   lydia_symbol node,
                                   unsigned int *pos)
{
    void *location;
    char *var_name;
    char *new_var_name;

    var_name = strdup(var->name);
    new_var_name = realloc(var_name, strlen(var_name) + strlen(node->name) + 2);
    if (NULL == new_var_name) {
        assert(0);
        abort();
    }
    strcat(new_var_name, ".");
    strcat(new_var_name, node->name);
    if (0 == hash_find(hash,
                       new_var_name,
                       (unsigned int)strlen(new_var_name),
                       &location)) {
        *pos = *(unsigned int *)location;

        free(new_var_name);

        return 1;
    }

    free(new_var_name);

    return 0;
}

static identifier make_identifier(variable_identifier name)
{
    register unsigned int ix, iy;

    identifier result = new_identifier(lydia_symbolNIL, int_listNIL, qualifier_listNIL);

    result->name = rdup_lydia_symbol(name->name->sym);
    if (extent_listNIL != name->ranges) {
        result->indices = new_int_list();
        for (ix = 0; ix < name->ranges->sz; ix++) {
            if (eval_int_expr(name->ranges->arr[ix]->from, index_entry_listNIL, user_type_entry_listNIL) !=
                eval_int_expr(name->ranges->arr[ix]->to, index_entry_listNIL, user_type_entry_listNIL)) {
                assert(0);
                abort();
            }
            append_int_list(result->indices,
                            eval_int_expr(name->ranges->arr[ix]->from,
                                          index_entry_listNIL,
                                          user_type_entry_listNIL));
        }
    }
    if (variable_qualifier_listNIL != name->qualifiers && name->qualifiers->sz > 0) {
        result->qualifiers = new_qualifier_list();
        for (ix = 0; ix < name->qualifiers->sz; ix++) {
            append_qualifier_list(result->qualifiers, new_qualifier(name->qualifiers->arr[ix]->name->sym, int_listNIL));
            if (extent_listNIL != name->qualifiers->arr[ix]->ranges) {
                result->qualifiers->arr[result->qualifiers->sz - 1]->indices = new_int_list();
                for (iy = 0; iy < name->qualifiers->arr[ix]->ranges->sz; iy++) {
                    if (eval_int_expr(name->qualifiers->arr[ix]->ranges->arr[iy]->from, index_entry_listNIL, user_type_entry_listNIL) !=
                        eval_int_expr(name->qualifiers->arr[ix]->ranges->arr[iy]->to, index_entry_listNIL, user_type_entry_listNIL)) {
                        assert(0);
                        abort();
                    }
                    append_int_list(result->qualifiers->arr[result->qualifiers->sz - 1]->indices,
                                    eval_int_expr(name->qualifiers->arr[ix]->ranges->arr[iy]->from,
                                                  index_entry_listNIL,
                                                  user_type_entry_listNIL));
                }
            }
        }
    }

    return result;
}

void propagate_variable_attributes(variable from, variable to)
{
    unsigned int pos, ix;

    for (ix = 0; ix < from->attributes->sz; ix++) {
        if (search_variable_attribute_list(to->attributes, from->attributes->arr[ix]->name, &pos)) {
            if (!isequal_variable_attribute(from->attributes->arr[ix], to->attributes->arr[pos])) {
                assert(0);
            }
            continue;
        }
        to->attributes = append_variable_attribute_list(to->attributes, rdup_variable_attribute(from->attributes->arr[ix]));
    }
}

void propagate_attributes_up(hierarchy hcsp, node current_node, node parent_node, edge parent_edge)
{
    variable parent_var, kid_var;

    unsigned int parent_pos, kid_pos, ix, iy;

    for (ix = 0; ix < current_node->edges->sz; ix++) {
        edge edge = current_node->edges->arr[ix];
        node kid = find_node(to_hierarchy(hcsp), edge->type);
        assert(kid != nodeNIL);
        propagate_attributes_up(hcsp, kid, current_node, edge);
    }
    if (nodeNIL != parent_node) {
        for (iy = 0; iy < parent_edge->bindings->sz; iy++) {
            if (!search_variable_list(parent_node->constraints->variables,
                                      parent_edge->bindings->arr[iy]->from,
                                      &parent_pos)) {
                assert(0);
                abort();
            }
            if (!search_variable_list(current_node->constraints->variables,
                                      parent_edge->bindings->arr[iy]->to,
                                      &kid_pos)) {
                assert(0);
                abort();
            }
            parent_var = parent_node->constraints->variables->arr[parent_pos];
            kid_var = current_node->constraints->variables->arr[kid_pos];
            propagate_variable_attributes(kid_var, parent_var);
        }
    }
}

static void propagate_attributes(hierarchy hcsp)
{
    node root = find_root_node(hcsp);
    if (nodeNIL == root) {
        return; /* No nodes. */
    }
    propagate_attributes_up(hcsp, root, nodeNIL, edgeNIL);
}

static void evaluate_bool_attribute(csp_dumper_context *ctx, const_variable var, int idx1, int idx2, csp_term term, lydia_bool_list values, lydia_symbol attr)
{
    unsigned int i;
    int bv;

    variable_assignment_list known_variables = new_variable_assignment_list();
    switch (var->tag) {
        case TAGbool_variable:
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_FALSE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_FALSE)));
            if (!evaluate_bool_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &bv)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "false");
                ctx->rc = 0;
                break;
            }
            values = append_lydia_bool_list(values, bv);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_TRUE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_TRUE)));
            if (!evaluate_bool_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &bv)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* TODO: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "true");
                ctx->rc = 0;
                break;
            }
            values = append_lydia_bool_list(values, bv);
            break;
        case TAGenum_variable:
            for (i = 0; i < ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->sz; i++) {
                lydia_symbol type = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->name;
                lydia_symbol sym = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->arr[i];
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx1, i)));
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx2, i)));
                if (!evaluate_bool_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &bv)) {
                    leh_error(ERR_EVALUATE_ATTRIBUTE_ENUM_ERROR,
                              LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                              ctx->org,
                              attr->name,
                              var->name->name->name,
                              var->name->name->name,
                              type->name,
                              sym->name);
                    ctx->rc = 0;
                }
                values = append_lydia_bool_list(values, bv);
                known_variables = delete_variable_assignment_list(known_variables, 0);
                known_variables = delete_variable_assignment_list(known_variables, 0);
            }
            break;
        case TAGint_variable:
        case TAGfloat_variable:
            assert(0);
            break;
    }
    rfre_variable_assignment_list(known_variables);
}

static void evaluate_int_attribute(csp_dumper_context *ctx, const_variable var, int idx1, int idx2, csp_term term, int_list values, lydia_symbol attr)
{
    unsigned int i;
    int iv;

    variable_assignment_list known_variables = new_variable_assignment_list();
    switch (var->tag) {
        case TAGbool_variable:
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_FALSE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_FALSE)));
            if (!evaluate_int_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &iv)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "false");
                ctx->rc = 0;
                break;
            }
            values = append_int_list(values, iv);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_TRUE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_TRUE)));
            if (!evaluate_int_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &iv)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "true");
                ctx->rc = 0;
                break;
            }
            values = append_int_list(values, iv);
            break;
        case TAGenum_variable:
            for (i = 0; i < ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->sz; i++) {
                lydia_symbol type = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->name;
                lydia_symbol sym = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->arr[i];
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx1, i)));
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx2, i)));
                if (!evaluate_int_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &iv)) {
                    leh_error(ERR_EVALUATE_ATTRIBUTE_ENUM_ERROR,
                              LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                              ctx->org,
                              attr->name,
                              var->name->name->name,
                              var->name->name->name,
                              type->name,
                              sym->name);
                    ctx->rc = 0;
                    break;
                }
                values = append_int_list(values, iv);
                known_variables = delete_variable_assignment_list(known_variables, 0);
                known_variables = delete_variable_assignment_list(known_variables, 0);
            }
            break;
        case TAGint_variable:
        case TAGfloat_variable:
            assert(0); /* TODO: To error. */
            break;
    }
    rfre_variable_assignment_list(known_variables);
}

static void evaluate_float_attribute(csp_dumper_context *ctx, const_variable var, int idx1, int idx2, csp_term term, double_list values, lydia_symbol attr)
{
    unsigned int i;
    double fv;

    variable_assignment_list known_variables = new_variable_assignment_list();
    switch (var->tag) {
        case TAGbool_variable:
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_FALSE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_FALSE)));
            if (!evaluate_float_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &fv)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "false");
                ctx->rc = 0;
                break;
            }
            values = append_double_list(values, fv);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_TRUE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_TRUE)));
            if (!evaluate_float_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &fv)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "true");
                ctx->rc = 0;
                break;
            }
            values = append_double_list(values, fv);
            break;
        case TAGenum_variable:
            for (i = 0; i < ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->sz; i++) {
                lydia_symbol type = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->name;
                lydia_symbol sym = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->arr[i];
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx1, i)));
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx2, i)));
                if (!evaluate_float_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &fv)) {
                    leh_error(ERR_EVALUATE_ATTRIBUTE_ENUM_ERROR,
                              LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                              ctx->org,
                              attr->name,
                              var->name->name->name,
                              var->name->name->name,
                              type->name,
                              sym->name);
                    ctx->rc = 0;
                    break;
                }
                values = append_double_list(values, fv);
                known_variables = delete_variable_assignment_list(known_variables, 0);
                known_variables = delete_variable_assignment_list(known_variables, 0);
            }
            break;
        case TAGint_variable:
        case TAGfloat_variable:
            assert(0); /* TODO: To error. */
            break;
    }
    rfre_variable_assignment_list(known_variables);
}

static void evaluate_enum_attribute(csp_dumper_context *ctx, const_variable var, int idx1, int idx2, csp_term term, lydia_symbol_list values, lydia_symbol attr)
{
    unsigned int i;
    lydia_symbol ev;

    variable_assignment_list known_variables = new_variable_assignment_list();
    switch (var->tag) {
        case TAGbool_variable:
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_FALSE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_FALSE)));
            if (!evaluate_enum_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &ev)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "false");
                ctx->rc = 0;
                break;
            }
            values = append_lydia_symbol_list(values, ev);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = delete_variable_assignment_list(known_variables, 0);
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx1, LYDIA_TRUE)));
            known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_bool_variable_assignment(idx2, LYDIA_TRUE)));
            if (!evaluate_enum_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &ev)) {
                leh_error(ERR_EVALUATE_ATTRIBUTE_ERROR,
                          LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                          ctx->org,
                          attr->name,
                          var->name->name->name,
                          var->name->name->name,
                          "true");
                ctx->rc = 0;
                break;
            }
            values = append_lydia_symbol_list(values, ev);
            break;
        case TAGenum_variable:
            for (i = 0; i < ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->sz; i++) {
                lydia_symbol type = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->name;
                lydia_symbol sym = ctx->node->constraints->domains->arr[to_enum_variable(var)->values_set]->entries->arr[i];
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx1, i)));
                known_variables = append_variable_assignment_list(known_variables, to_variable_assignment(new_enum_variable_assignment(idx2, i)));
                if (!evaluate_enum_term(term, ctx->node->constraints->domains, ctx->node->constraints->variables, ctx->node->constraints->constants, known_variables, &ev)) {
                    leh_error(ERR_EVALUATE_ATTRIBUTE_ENUM_ERROR,
                              LEH_LOCATION_GLOBAL, /* @todo: Show the system name. */
                              ctx->org,
                              attr->name,
                              var->name->name->name,
                              var->name->name->name,
                              type->name,
                              sym->name);
                    ctx->rc = 0;
                }
                values = append_lydia_symbol_list(values, ev);
                known_variables = delete_variable_assignment_list(known_variables, 0);
                known_variables = delete_variable_assignment_list(known_variables, 0);
            }
            break;
        case TAGint_variable:
        case TAGfloat_variable:
            assert(0); /* TODO: To error. */
            break;
    }
    rfre_variable_assignment_list(known_variables);
}

static void attribute_variables(csp_dumper_context *ctx,
                                const_system_definition def,
                                variable_list variables)
{
    variable_attribute attribute;
    bool_variable_attribute b;
    int_variable_attribute i;
    float_variable_attribute f;
    enum_variable_attribute e;
    unsigned int idx1, idx2;
    unsigned int ix, iy;
    unsigned int value_set;

    int old_constants = ctx->node->constraints->constants->sz;
    int old_rc = ctx->rc;
    int result = 1;

    csp_term term;

    lydia_symbol attr;

    for (ix = 0; ix < def->attributes->sz; ix++) {
        identifier id = make_identifier(def->attributes->arr[ix]->var->name);
        ctx->rc = 1;
        if (!find_variable_hash(&ctx->variable_hash,
                                id,
                                ctx->node->type, &idx1)) {
/* The variable is not used, skip it. */
            rfre_identifier(id);
            continue;
        }

        rfre_identifier(id);
        ctx->org = def->attributes->arr[ix]->var->name->name->org;
        attribute = variable_attributeNIL;
        assert(def->attributes->arr[ix]->value != exprNIL);
        assert(def->attributes->arr[ix]->value->type != typeNIL);

        term = walk_term(ctx, def->attributes->arr[ix]->value);

        attr = to_user_type(def->attributes->arr[ix]->type)->name->sym;

        if (orig_symbolNIL != def->attributes->arr[ix]->alias) {
            if (!find_variable_name_hash(&ctx->variable_hash,
                                         def->attributes->arr[ix]->alias->sym,
                                         ctx->node->type,
                                         &idx2)) {
                assert(0);
                abort();
            }
        } else {
            idx2 = idx1;
        }

        switch (def->attributes->arr[ix]->value->type->tag) {
            case TAGbool_type:
                b = new_bool_variable_attribute(attr, new_lydia_bool_list());
                evaluate_bool_attribute(ctx, variables->arr[idx1], idx1, idx2, term, b->values, attr);
                attribute = to_variable_attribute(b);
                break;
            case TAGint_type:
                i = new_int_variable_attribute(attr, new_int_list());
                evaluate_int_attribute(ctx, variables->arr[idx1], idx1, idx2, term, i->values, attr);
                attribute = to_variable_attribute(i);
                break;
            case TAGfloat_type:
                f = new_float_variable_attribute(attr, new_double_list());
                evaluate_float_attribute(ctx, variables->arr[idx1], idx1, idx2, term, f->values, attr);
                attribute = to_variable_attribute(f);
                break;
            case TAGuser_type:
                if (!search_user_type_entry_list(ctx->user_type_table, to_user_type(def->attributes->arr[ix]->value->type)->name->sym, &iy)) {
                    assert(0);
                    abort();
                }
                assert(ctx->user_type_table->arr[iy]->tag == TAGenum_user_type_entry);
                if (!search_values_set_list(ctx->node->constraints->domains, to_user_type(def->attributes->arr[ix]->value->type)->name->sym, &value_set)) {
                    assert(0);
                    abort();
                }
                e = new_enum_variable_attribute(attr, new_lydia_symbol_list(), value_set);
                evaluate_enum_attribute(ctx, variables->arr[idx1], idx1, idx2, term, e->values, attr);
                attribute = to_variable_attribute(e);
                break;
            default:
                assert(0);
        }

        rfre_csp_term(term);
        if (orig_symbolNIL != def->attributes->arr[ix]->alias) {
            delete_variable_hash(&ctx->variable_hash,
                                 variables->arr[idx2]->name,
                                 ctx->node->type);
            delete_variable_list(variables, idx2);
        }
        if (ctx->rc) {
            check_internal_attribute(attribute,
                                     def->attributes->arr[ix],
                                     def->name->sym,
                                     variables->arr[idx1]->name);
        }
        append_variable_attribute_list(variables->arr[idx1]->attributes,
                                       attribute);
        result = ctx->rc & result;
    }
    for (ix = old_constants; ix < ctx->node->constraints->constants->sz; ) {
        delete_constant_list(ctx->node->constraints->constants, old_constants);
    }
    ctx->rc = old_rc & result;
}

static unsigned int register_domain(csp_dumper_context *ctx,
                                    lydia_symbol name,
                                    values_set_list domains)
{
    unsigned int i, j = 0, k;

    for (i = 0; i < domains->sz; i++) {
        if (name == domains->arr[i]->name) {
            return i;
        }
    }
    for (i = 0; i < ctx->user_type_table->sz; i++) {
        if (TAGenum_user_type_entry == ctx->user_type_table->arr[i]->tag &&
            to_enum_user_type_entry(ctx->user_type_table->arr[i])->name->sym == name) {
            orig_symbol_list entries = to_enum_user_type_entry(ctx->user_type_table->arr[i])->entries;
            lydia_symbol_list values = new_lydia_symbol_list();
            for (k = 0; k < entries->sz; k++) {
                values = append_lydia_symbol_list(values, entries->arr[k]->sym);
            }
            append_values_set_list(domains, new_values_set(name, values));
            j = 1;
        }
    }
    assert(j);

    return domains->sz - 1;
}

static int add_constant(csp_dumper_context *ctx, constant_list constants, lydia_symbol name, expr ex)
{
    unsigned int pos;

    constant nconst = constantNIL;

    switch (ex->tag) {
        case TAGexpr_bool:
            nconst = to_constant(new_bool_constant(rdup_lydia_symbol(name), to_expr_bool(ex)->v));
            break;
        case TAGexpr_float:
            nconst = to_constant(new_float_constant(rdup_lydia_symbol(name), to_expr_float(ex)->v));
            break;
        case TAGexpr_int:
            nconst = to_constant(new_int_constant(rdup_lydia_symbol(name), to_expr_int(ex)->v));
            break;
        case TAGexpr_enum:
            nconst = to_constant(new_enum_constant(rdup_lydia_symbol(name),
                                                   to_expr_enum(ex)->entry,
                                                   register_domain(ctx, to_expr_enum(ex)->name, ctx->node->constraints->domains)));
            break;
        default:
            assert(0);
            break;
    }

    if (search_constant_list(constants, nconst, &pos)) {
        rfre_constant(nconst);
        return (int)pos;
    }

    append_constant_list(constants, nconst);

    return (int)(constants->sz - 1);
}

static int add_variable(csp_dumper_context *ctx,
                        variable_list variables,
                        hash_table *variable_hash,
                        values_set_list domains,
                        identifier var,
                        lydia_symbol node,
                        type t)
{
    unsigned int pos;
    variable nvar = variableNIL;
#if 0
    if (search_variable_list(variables, var, &pos)) {
        return (int)pos;
    }
#endif
    if (find_variable_hash(variable_hash, var, node, &pos)) {
        return (int)pos;
    }

    switch (t->tag) {
        case TAGbool_type:
            nvar = to_variable(new_bool_variable(rdup_identifier(var), new_variable_attribute_list(), -1));
            break;
        case TAGfloat_type:
            nvar = to_variable(new_float_variable(rdup_identifier(var), new_variable_attribute_list()));
            break;
        case TAGint_type:
            nvar = to_variable(new_int_variable(rdup_identifier(var), new_variable_attribute_list()));
            break;
        case TAGuser_type:
            if (!search_user_type_entry_list(ctx->user_type_table, to_user_type(t)->name->sym, &pos)) {
                assert(0);
                abort();
            }
            if (ctx->user_type_table->arr[pos]->tag == TAGenum_user_type_entry) {
                nvar = to_variable(new_enum_variable(rdup_identifier(var),
                                                     new_variable_attribute_list(),
                                                     register_domain(ctx, to_enum_user_type_entry(t)->name->sym, domains)));
            }
            if (ctx->user_type_table->arr[pos]->tag == TAGalias_user_type_entry) {
                return add_variable(ctx,
                                    variables,
                                    variable_hash,
                                    domains,
                                    var,
                                    node,
                                    to_alias_user_type_entry(ctx->user_type_table->arr[pos])->type);
            }
            if (ctx->user_type_table->arr[pos]->tag == TAGstruct_user_type_entry) {
/* Can't appear because it is removed in the rewritings. */
                assert(0);
                abort();
            }
            assert(variableNIL != nvar);
            break;
        default:
            assert(0); /* TODO: To error. */
            abort();
            break;
    }

    add_variable_hash(variable_hash, nvar->name, node, variables->sz);
    append_variable_list(variables, nvar);

    return (int)(variables->sz - 1);
}

static csp_term walk_term(csp_dumper_context *ctx, expr ex)
{
    unsigned int ix;
    csp_term x, l, r, y, n;
    csp_term condition = csp_termNIL;
    csp_term result = csp_termNIL;
    identifier var;
    int varid;

    switch (ex->tag) {
        case TAGexpr_cond:
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("arith_switch"), new_csp_term_list()));
            to_csp_function_term(result)->args = append_csp_term_list(to_csp_function_term(result)->args, walk_term(ctx, to_expr_cond(ex)->lhs));
            for (ix = 0; ix < to_expr_cond(ex)->choices->sz; ix++) {
                csp_term cond = walk_term(ctx, to_expr_cond(ex)->choices->arr[ix]->cond);
                csp_term val = walk_term(ctx, to_expr_cond(ex)->choices->arr[ix]->val);
                to_csp_function_term(result)->args = append_csp_term_list(to_csp_function_term(result)->args, cond);
                to_csp_function_term(result)->args = append_csp_term_list(to_csp_function_term(result)->args, val);
            }
            if (exprNIL != to_expr_cond(ex)->deflt) {
                to_csp_function_term(result)->args = append_csp_term_list(to_csp_function_term(result)->args, walk_term(ctx, to_expr_cond(ex)->deflt));
            } else {
                to_csp_function_term(result)->args = append_csp_term_list(to_csp_function_term(result)->args, csp_termNIL);
            }
            break;
        case TAGexpr_not:
            if (csp_termNIL != (x = walk_term(ctx, to_expr_negate(ex)->x))) {
                result = to_csp_term(new_csp_function_term(add_lydia_symbol("not"),
                                                           append_csp_term_list(new_csp_term_list(), x)));
            }
            break;
        case TAGexpr_and:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_and(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_and(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("and"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_or:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_or(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_or(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("or"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_if_else:
            if (csp_termNIL == (condition = walk_term(ctx, to_expr_if_else(ex)->cond))) {
                break;
            }
            if (csp_termNIL == (y = walk_term(ctx, to_expr_if_else(ex)->thenval))) {
                rfre_csp_term(condition);
                break;
            }
            if (csp_termNIL == (n = walk_term(ctx, to_expr_if_else(ex)->elseval))) {
                rfre_csp_term(condition);
                rfre_csp_term(y);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("arith_if"),
                                                       append_csp_term_list(append_csp_term_list(append_csp_term_list(new_csp_term_list(), condition), y), n)));
            break;
        case TAGexpr_eq:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_and(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_and(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("equiv"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_ne:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_ne(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_ne(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("ne"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_lt:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_lt(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_lt(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("lt"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_ge:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_gt(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_gt(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("not"), append_csp_term_list(new_csp_term_list(), to_csp_term(new_csp_function_term(add_lydia_symbol("lt"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r))))));
            break;
        case TAGexpr_le:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_le(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_le(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("or"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), to_csp_term(new_csp_function_term(add_lydia_symbol("lt"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)))), to_csp_term(new_csp_function_term(add_lydia_symbol("equiv"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), rdup_csp_term(l)), rdup_csp_term(r)))))));
            break;
        case TAGexpr_gt:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_ge(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_ge(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("and"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), to_csp_term(new_csp_function_term(add_lydia_symbol("not"), append_csp_term_list(new_csp_term_list(), to_csp_term(new_csp_function_term(add_lydia_symbol("lt"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r))))))), to_csp_term(new_csp_function_term(add_lydia_symbol("not"), append_csp_term_list(new_csp_term_list(), to_csp_term(new_csp_function_term(add_lydia_symbol("equiv"), append_csp_term_list(append_csp_term_list(new_csp_term_list(), rdup_csp_term(l)), rdup_csp_term(r))))))))));
            break;
        case TAGexpr_mult:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_mult(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_mult(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("mul"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_div:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_div(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_div(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("div"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_add:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_add(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_add(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("add"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_sub:
            if (csp_termNIL == (l = walk_term(ctx, to_expr_sub(ex)->l))) {
                break;
            }
            if (csp_termNIL == (r = walk_term(ctx, to_expr_sub(ex)->r))) {
                rfre_csp_term(l);
                break;
            }
            result = to_csp_term(new_csp_function_term(add_lydia_symbol("sub"),
                                                       append_csp_term_list(append_csp_term_list(new_csp_term_list(), l), r)));
            break;
        case TAGexpr_negate:
            if (csp_termNIL != (x = walk_term(ctx, to_expr_negate(ex)->x))) {
                result = to_csp_term(new_csp_function_term(add_lydia_symbol("neg"),
                                                           append_csp_term_list(new_csp_term_list(), x)));
            }
            break;
        case TAGexpr_float:
        case TAGexpr_bool:
        case TAGexpr_int:
        case TAGexpr_enum:
            result = to_csp_term(new_csp_constant_term(add_constant(ctx,
                                                                    ctx->node->constraints->constants,
                                                                    lydia_symbolNIL /* TODO: Add support for named constants. */,
                                                                    ex)));
            break;
        case TAGexpr_variable:
            var = make_identifier(to_expr_variable(ex)->name);
            varid = add_variable(ctx,
                                 ctx->node->constraints->variables,
                                 &ctx->variable_hash,
                                 ctx->node->constraints->domains,
                                 var,
                                 ctx->node->type,
                                 to_expr_variable(ex)->type);
            result = to_csp_term(new_csp_variable_term(varid));
            rfre_identifier(var);
            break;
        case TAGexpr_apply:
/* Should be inlined. */
            assert(0);
        default:
            assert(0); /* TODO: To error. */
            break;
    }

    return result;
}

static csp_sentence walk_expression(expr ex, csp_dumper_context *ctx)
{
    csp_sentence l, r, x;
    csp_sentence result = csp_sentenceNIL;

    assert(ex != exprNIL);

    switch (ex->tag) {
        case TAGexpr_ne:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_ne(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_ne(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_not_sentence(to_csp_sentence(new_csp_equiv_sentence(l, r))));
            break;
        case TAGexpr_eq:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_eq(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_eq(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_equiv_sentence(l, r));
            break;
        case TAGexpr_lt:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_lt(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_lt(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_lt_sentence(l, r));
            break;
        case TAGexpr_le:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_gt(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_gt(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_or_sentence(to_csp_sentence(new_csp_lt_sentence(l, r)),
                                                         to_csp_sentence(new_csp_equiv_sentence(rdup_csp_sentence(l), rdup_csp_sentence(r)))));
            break;
        case TAGexpr_gt:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_le(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_le(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_and_sentence(to_csp_sentence(new_csp_not_sentence(to_csp_sentence(new_csp_lt_sentence(l, r)))),
                                                          to_csp_sentence(new_csp_not_sentence(to_csp_sentence(new_csp_equiv_sentence(rdup_csp_sentence(l), rdup_csp_sentence(r)))))));
            break;
        case TAGexpr_ge:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_ge(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_ge(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_not_sentence(to_csp_sentence(new_csp_lt_sentence(l, r))));
            break;
        case TAGexpr_imply:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_imply(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_imply(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_impl_sentence(l, r));
            break;
        case TAGexpr_and:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_and(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_and(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_and_sentence(l, r));
            break;
        case TAGexpr_or:
            if (csp_sentenceNIL == (l = walk_expression(to_expr_or(ex)->l, ctx))) {
                break;
            }
            if (csp_sentenceNIL == (r = walk_expression(to_expr_or(ex)->r, ctx))) {
                rfre_csp_sentence(l);
                break;
            }
            result = to_csp_sentence(new_csp_or_sentence(l, r));
            break;
        case TAGexpr_not:
            if (csp_sentenceNIL != (x = walk_expression(to_expr_not(ex)->x, ctx))) {
                result = to_csp_sentence(new_csp_not_sentence(x));
            }
            break;
        case TAGexpr_negate:
        case TAGexpr_sub:
        case TAGexpr_add:
        case TAGexpr_mult:
        case TAGexpr_div:
        case TAGexpr_variable:
        case TAGexpr_float:
        case TAGexpr_bool:
        case TAGexpr_int:
        case TAGexpr_enum:
        case TAGexpr_if_else:
        case TAGexpr_cond:
            result = to_csp_sentence(new_csp_atomic_sentence(walk_term(ctx, ex)));
            break;
        default:
            break;
    }

    return result;
}

/* Appends an edge to the resulting H-CSP. */
static void append_edge(csp_dumper_context *ctx,
                        node src_node,
                        node dst_node,
                        lydia_symbol type,
                        lydia_symbol name,
                        expr_list src,
                        formal_list dst,
                        variable_list src_vars,
                        variable_list dst_vars,
                        int_list indices)
{
    register unsigned int ix;

    identifier src_id, dst_id;

    mapping symbol_mapping;

    edge edge = new_edge(type,
                         name,
                         rdup_int_list(indices),
                         new_mapping_list());

    for (ix = 0; ix < src->sz; ix++) {
        assert(src->arr[ix]->tag == TAGexpr_variable);
        src_id = make_identifier(to_expr_variable(src->arr[ix])->name);
        dst_id = make_identifier(dst->arr[ix]->name);

        add_variable(ctx,
                     src_vars,
                     &ctx->variable_hash,
                     src_node->constraints->domains,
                     src_id,
                     src_node->type,
                     to_expr_variable(src->arr[ix])->type);
        add_variable(ctx,
                     dst_vars,
                     &ctx->variable_hash,
                     dst_node->constraints->domains,
                     dst_id,
                     dst_node->type,
                     to_expr_variable(src->arr[ix])->type);

        symbol_mapping = new_mapping(src_id, dst_id);
        edge->bindings = append_mapping_list(edge->bindings, symbol_mapping);
    }

    ctx->node->edges = append_edge_list(src_node->edges, edge);
}

static void register_hier_edge(expr_apply pe, csp_dumper_context *ctx)
{
    unsigned int pos;
    lydia_symbol name, type;
    system_definition dest;
    formal_list to;
    expr_list from;
    node dst_node;
    variable_list src_vars, dst_vars;
    int_list indices;

    name = pe->name->sym;

    assert(NULL != ctx->systems);

    assert(TAGuser_type == pe->type->tag);
    type = to_user_type(pe->type)->name->sym;
    if (!search_system_definition_list(ctx->systems, type, &pos)) {
        assert(0);
        abort();
    }
    assert(ctx->systems->arr[pos]->tag == TAGsystem_definition);
    dest = to_system_definition(ctx->systems->arr[pos]);

    if (!search_reference_list(ctx->system->references, name, &pos)) {
        assert(0);
        abort();
    }

    to = dest->formals;
    from = pe->parms;
    assert(from->sz == to->sz);

    dst_node = find_node(to_hierarchy(ctx->result), type);
    assert(nodeNIL != dst_node);

    src_vars = ctx->node->constraints->variables;
    dst_vars = dst_node->constraints->variables;

    if (extent_listNIL == pe->extents) {
        append_edge(ctx,
                    ctx->node,
                    dst_node,
                    type,
                    name,
                    from,
                    to,
                    src_vars,
                    dst_vars,
                    int_listNIL);
        return;
    }
/* We have an array of systems. */
    indices = init_index(pe->extents,
                         index_entry_listNIL,
                         ctx->user_type_table);
    do {
        append_edge(ctx,
                    ctx->node,
                    dst_node,
                    type,
                    name,
                    from,
                    to,
                    src_vars,
                    dst_vars,
                    indices);
    } while (int_listNIL != (indices = advance_index(indices,
                                                     pe->extents,
                                                     index_entry_listNIL,
                                                     ctx->user_type_table)));
}

static simple_predicate dump_simple_predicate(simple_predicate prop, csp_dumper_context *ctx)
{
    csp_sentence s;

    switch (prop->x->tag) {
        case TAGexpr_not:
        case TAGexpr_and:
        case TAGexpr_mult:
        case TAGexpr_or:
        case TAGexpr_add:
        case TAGexpr_eq:
        case TAGexpr_ne:
        case TAGexpr_lt:
        case TAGexpr_gt:
        case TAGexpr_le:
        case TAGexpr_ge:
        case TAGexpr_imply:
        case TAGexpr_variable:
        case TAGexpr_int:
        case TAGexpr_bool:
        case TAGexpr_float:
        case TAGexpr_cond:
            s = walk_expression(prop->x, ctx);
            if (s == csp_sentenceNIL) {
                leh_error(INTERNAL_UNSUPPORTED_EXPR,
                          LEH_LOCATION_GLOBAL,
                          prop->org);
                ctx->rc = 0;
                break;
            }
            ctx->sentences = append_csp_sentence_list(ctx->sentences, s);
            break;
        case TAGexpr_negate:
        case TAGexpr_sub:
        case TAGexpr_if_else:
            s = to_csp_sentence(new_csp_atomic_sentence(walk_term(ctx, prop->x)));
            ctx->sentences = append_csp_sentence_list(ctx->sentences, s);
            break;
        case TAGexpr_apply:
            register_hier_edge(to_expr_apply(prop->x), ctx);
            break;
        default:
            leh_error(INTERNAL_UNSUPPORTED_EXPR,
                      LEH_LOCATION_GLOBAL,
                      prop->org);
            ctx->rc = 0;
            break;
    }

    return prop;
}

static system_definition dump_system(system_definition def,
                                     csp_dumper_context *ctx)
{
    register unsigned int ix;

    identifier var;

    node the_node = new_node(rdup_lydia_symbol(def->name->sym),
                             new_edge_list(),
                             to_kb(new_csp(new_values_set_list(),
                                           new_variable_list(),
                                           new_variable_list(),
                                           new_constant_list(),
                                           ENCODING_NONE,
                                           csp_sentence_listNIL)));

    ctx->node = the_node;
    ctx->sentences = new_csp_sentence_list();
    ctx->system = def;

    for (ix = 0; ix < def->formals->sz; ix++) {
        var = make_identifier(def->formals->arr[ix]->name);
        add_variable(ctx,
                     ctx->node->constraints->variables,
                     &ctx->variable_hash,
                     ctx->node->constraints->domains,
                     var,
                     ctx->node->type,
                     def->formals->arr[ix]->type);
        rfre_identifier(var);
    }
    for (ix = 0; ix < def->locals->sz; ix++) {
        var = make_identifier(def->locals->arr[ix]->name);
        add_variable(ctx,
                     ctx->node->constraints->variables,
                     &ctx->variable_hash,
                     ctx->node->constraints->domains,
                     var,
                     ctx->node->type,
                     def->locals->arr[ix]->type);
        rfre_identifier(var);
    }
    for (ix = 0; ix < def->predicates->predicates->sz; ix++) {
        assert(TAGsimple_predicate == def->predicates->predicates->arr[ix]->tag);
        dump_simple_predicate(to_simple_predicate(def->predicates->predicates->arr[ix]), ctx);
    }

    to_csp(ctx->node->constraints)->sentences = ctx->sentences;
    ctx->result->nodes = append_node_list(ctx->result->nodes, to_node(ctx->node));

    return def;
}

static void order_systems(const_definition_list defs, 
                          const_system_definition root,
                          csp_dumper_context *ctx)
{
    register unsigned int ix;
    unsigned int pos;

    int valid;
    definition dummy;
    if (search_system_definition_list(ctx->systems, root->name->sym, &pos)) {
        extract_definition_list(ctx->systems, pos, &dummy, &valid);
    }
    insert_definition_list(ctx->systems, 0, to_definition(root));

    for (ix = 0; ix < root->references->sz; ix++) {
        lydia_symbol name;
        assert(root->references->arr[ix]->type->tag == TAGuser_type);
        name = to_user_type(root->references->arr[ix]->type)->name->sym;
        if (name != root->name->sym &&
            search_system_definition_list(defs, name, &pos)) {
            order_systems(defs, to_system_definition(defs->arr[pos]), ctx);
        }
    }
}

csp_hierarchy dump_model(const_model model,
                         const_user_type_entry_list user_type_table,
                         const_attribute_entry_list attribute_table,
                         const int option_unused_systems,
                         int *status)
{
    unsigned int ix;

    csp_hierarchy result = new_csp_hierarchy(new_node_list());

    csp_dumper_context the_ctx;
    csp_dumper_context *ctx = &the_ctx;

    system_definition root;

    definition def;

    node current;

    ctx->result = result;
    ctx->systems = new_definition_list();
    ctx->user_type_table = user_type_table;
    ctx->attribute_table = attribute_table;
    ctx->rc = 1;

    hash_init(&ctx->variable_hash, 2, NULL, NULL);

    if (option_unused_systems) {
        for (ix = 0; ix < model->defs->sz; ix++) {
            def = model->defs->arr[ix];
            if (def->tag == TAGsystem_definition) {
                append_definition_list(ctx->systems, def);
            }
        }
    } else {
/*
 * Take the last system to be the root system. This may change in the
 * future with some more elaborate mechanism for choosing the main
 * system.
 */
        root = system_definitionNIL;
        for (ix = model->defs->sz - 1; ix < model->defs->sz; ix--) {
            if (model->defs->arr[ix]->tag == TAGsystem_definition) {
                root = to_system_definition(model->defs->arr[ix]);
                break;
            }
        }
        if (root != system_definitionNIL) {
            order_systems(model->defs, root, ctx);
        }
    }

    for (ix = 0; ix < ctx->systems->sz; ix++) {
        assert(ctx->systems->arr[ix]->tag == TAGsystem_definition);
        dump_system(to_system_definition(ctx->systems->arr[ix]), ctx);
        current = find_node(to_hierarchy(result), ctx->systems->arr[ix]->name->sym);

        attribute_variables(ctx,
                            to_system_definition(ctx->systems->arr[ix]),
                            current->constraints->variables);

        if (!cross_check_attributes(current->type,
                                    current->constraints->variables,
                                    to_system_definition(ctx->systems->arr[ix])->formals,
                                    to_system_definition(ctx->systems->arr[ix])->locals)) {
            ctx->rc = 0;
        }
    }
    propagate_attributes(to_hierarchy(ctx->result));

    *status = ctx->rc;

    fre_definition_list(ctx->systems);

    hash_destroy(&ctx->variable_hash);

    return result;
}
