#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "rewrite.h"
#include "search.h"
#include "strdup.h"
#include "array.h"
#include "attr.h"
#include "expr.h"
#include "iter.h"

#define is_constant_or_variable(ex) ((ex)->tag == TAGexpr_variable || (ex)->tag == TAGexpr_int || (ex)->tag == TAGexpr_float || (ex)->tag == TAGexpr_bool || (ex)->tag == TAGexpr_enum)

lydia_symbol_list expref_systems = lydia_symbol_listNIL;

/* Function Prototypes: */
static void flatten_system_predicates(predicate p, predicate_list l, const_attribute_entry_list attribute_table, index_entry_list index_table, const_user_type_entry_list user_type_table);

static extent_list bind_extents(extent_list extents,
                                const_index_entry_list index_table,
                                const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    if (extents != extent_listNIL) {
        for (ix = 0; ix < extents->sz; ix++) {
            if (TAGexpr_int != extents->arr[ix]->from->tag) {
                int from = eval_int_expr(extents->arr[ix]->from, index_table, user_type_table);
                rfre_expr(extents->arr[ix]->from);
                extents->arr[ix]->from = to_expr(new_expr_int(to_type(new_int_type()), from));
            }
            if (TAGexpr_int != extents->arr[ix]->to->tag) {
                int to = eval_int_expr(extents->arr[ix]->to, index_table, user_type_table);
                rfre_expr(extents->arr[ix]->to);
                extents->arr[ix]->to = to_expr(new_expr_int(to_type(new_int_type()), to));
            }
        }
    }

    return extents;
}

/**
 * Walks down an expression and evaluates all indices. This function is
 * used in the expansion of "forall" and "exists" quantifiers.
 *
 * @param e the expression to traverse;
 * @param index_table in this table we store the current indices of the
 *                    "forall" and "exists" index variables when unrolling
 *                    them;
 * @user_type_table a list of all user defined types.
 * @returns an expression in which all indices are integer expressions.
 */
static expr bind_indices(expr e, const_index_entry_list index_table, const_user_type_entry_list user_type_table)
{
    unsigned int ix;
    expr_variable var = expr_variableNIL;

    switch (e->tag) {
        case TAGexpr_variable:
            var = to_expr_variable(e);
            var->name->ranges = bind_extents(var->name->ranges,
                                             index_table,
                                             user_type_table);
            if (variable_qualifier_listNIL != var->name->qualifiers) {
                for (ix = 0; ix < var->name->qualifiers->sz; ix++) {
                    var->name->qualifiers->arr[ix]->ranges = bind_extents(var->name->qualifiers->arr[ix]->ranges,
                                                                          index_table,
                                                                          user_type_table);
                }
            }
            break;

/* Literals: */
        case TAGexpr_bool:
        case TAGexpr_int:
        case TAGexpr_float:
        case TAGexpr_enum:
/* Noop. */
            break;

/* Binary expressions: */
        case TAGexpr_add:
        case TAGexpr_mult:
        case TAGexpr_and:
        case TAGexpr_or:
        case TAGexpr_imply:
        case TAGexpr_lt:
        case TAGexpr_gt:
        case TAGexpr_le:
        case TAGexpr_ge:
        case TAGexpr_eq:
        case TAGexpr_ne:
        case TAGexpr_sub:
        case TAGexpr_div:
        case TAGexpr_mod:
            to_expr_binop(e)->l = bind_indices(to_expr(to_expr_binop(e)->l), index_table, user_type_table);
            to_expr_binop(e)->r = bind_indices(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
            break;

/* Unary expressions: */
        case TAGexpr_not:
        case TAGexpr_negate:
            to_expr_unop(e)->x = bind_indices(to_expr(to_expr_unop(e)->x), index_table, user_type_table);
            break;

/* Others: */
        case TAGexpr_if_else:
            to_expr_if_else(e)->cond = bind_indices(to_expr(to_expr_if_else(e)->cond), index_table, user_type_table);
            to_expr_if_else(e)->thenval = bind_indices(to_expr(to_expr_if_else(e)->thenval), index_table, user_type_table);
            to_expr_if_else(e)->elseval = bind_indices(to_expr(to_expr_if_else(e)->elseval), index_table, user_type_table);
            break;
        case TAGexpr_cond:
            to_expr_cond(e)->lhs = bind_indices(to_expr(to_expr_cond(e)->lhs), index_table, user_type_table);
            for (ix = 0; ix < to_expr_cond(e)->choices->sz; ix++) {
                to_expr_cond(e)->choices->arr[ix]->cond = bind_indices(to_expr(to_expr_cond(e)->choices->arr[ix]->cond), index_table, user_type_table);
                to_expr_cond(e)->choices->arr[ix]->val = bind_indices(to_expr(to_expr_cond(e)->choices->arr[ix]->val), index_table, user_type_table);
            }
            if (exprNIL != to_expr_cond(e)->deflt) {
                to_expr_cond(e)->deflt = bind_indices(to_expr(to_expr_cond(e)->deflt), index_table, user_type_table);
            }
            break;
        case TAGexpr_apply:
            for (ix = 0; ix < to_expr_apply(e)->parms->sz; ix++) {
                to_expr_apply(e)->parms->arr[ix] = bind_indices(to_expr(to_expr_apply(e)->parms->arr[ix]), index_table, user_type_table);
            }
            bind_extents(to_expr_apply(e)->extents, index_table, user_type_table);
            break;
        case TAGexpr_cast:
            to_expr_cast(e)->v = bind_indices(to_expr(to_expr_cast(e)->v), index_table, user_type_table);
            break;
        case TAGexpr_concatenation:
            for (ix = 0; ix < to_expr_concatenation(e)->l->sz; ix++) {
                to_expr_concatenation(e)->l->arr[ix] = bind_indices(to_expr(to_expr_concatenation(e)->l->arr[ix]), index_table, user_type_table);
            }
            break;
    };

    return e;
}

static expr_list expand_variable(const_expr_variable var,
                                 const int fg_array,
                                 const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    struct_user_type_entry structure;

    unsigned int pos;

    extent_list extent;

    expr_variable new_var;
    expr_list new_exprs;

    expr_list result;

    result = new_expr_list();
    if (fg_array &&
        var->name->ranges != extent_listNIL &&
        var->name->ranges->sz > 0) {
        extent = init_extent(var->name->ranges, index_entry_listNIL, user_type_table);
        do {
            new_var = new_expr_variable(rdup_type(var->type), new_variable_identifier(rdup_orig_symbol(var->name->name), rdup_extent_list(extent), rdup_variable_qualifier_list(var->name->qualifiers)));
            new_exprs = expand_variable(new_var, 0, user_type_table);
            concat_expr_list(result, new_exprs);
            rfre_expr_variable(new_var);
        } while (extent_listNIL != (extent = advance_extent(extent, var->name->ranges, index_entry_listNIL, user_type_table)));
    } else if (var->type->tag == TAGuser_type &&
        search_user_type_entry_list_with_tag(user_type_table,
                                             to_user_type(var->type)->name->sym,
                                             TAGstruct_user_type_entry,
                                             &pos)) {
        structure = to_struct_user_type_entry(user_type_table->arr[pos]);
        for (ix = 0; ix < structure->entries->sz; ix++) {
            new_var = new_expr_variable(rdup_type(structure->types->arr[ix]), new_variable_identifier(rdup_orig_symbol(structure->entries->arr[ix]), rdup_extent_list(structure->ranges->arr[ix]), append_variable_qualifier_list(var->name->qualifiers == variable_qualifier_listNIL ? new_variable_qualifier_list() : rdup_variable_qualifier_list(var->name->qualifiers), new_variable_qualifier(rdup_orig_symbol(var->name->name), rdup_extent_list(var->name->ranges)))));
            new_exprs = expand_variable(new_var, 1, user_type_table);
            concat_expr_list(result, new_exprs);
            rfre_expr_variable(new_var);
        }
    } else {
        append_expr_list(result, rdup_expr(to_expr(var)));
    }
    return result;
}

static expr_list expand_concatenation(const_expr_concatenation concatenation,
                                      const_formal_list formals,
                                      const_local_list locals,
                                      const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    expr_list result = new_expr_list();

    for (ix = 0; ix < concatenation->l->sz; ix++) {
        if (concatenation->l->arr[ix]->tag == TAGexpr_concatenation) {
            concat_expr_list(result, expand_concatenation(to_expr_concatenation(concatenation->l->arr[ix]), formals, locals, user_type_table));
            continue;
        }
        if (concatenation->l->arr[ix]->tag == TAGexpr_variable) {
            concat_expr_list(result, expand_variable(to_expr_variable(concatenation->l->arr[ix]), 1, user_type_table));
            continue;
        }
        append_expr_list(result, rdup_expr(concatenation->l->arr[ix]));
    }

    return result;
}

static expr_list expand_expr(const_expr expr,
                             const_formal_list formals,
                             const_local_list locals,
                             const_user_type_entry_list user_type_table)
{
    expr_list result = expr_listNIL;
    switch (expr->tag) {
        case TAGexpr_variable:
            return expand_variable(to_expr_variable(expr), 1, user_type_table);
        case TAGexpr_concatenation:
            return expand_concatenation(to_expr_concatenation(expr), formals, locals, user_type_table);
        default:
            result = append_expr_list(new_expr_list(), rdup_expr(expr));
            break;
    }
    return result;
}

static expr rewrite_equation(const_expr lhs_expr,
                             const_expr rhs_expr,
                             const_formal_list formals,
                             const_local_list locals,
                             const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    expr result = exprNIL;
    expr_list lexp = expand_expr(lhs_expr, formals, locals, user_type_table);
    expr_list rexp = expand_expr(rhs_expr, formals, locals, user_type_table);

    assert(lexp->sz == rexp->sz);
    assert(lexp->sz > 0);

    result = to_expr(new_expr_eq(to_type(new_bool_type()), rdup_expr(lexp->arr[0]), rdup_expr(rexp->arr[0])));

    for (ix = 1; ix < lexp->sz; ix++) {
        result = to_expr(new_expr_and(to_type(new_bool_type()),
                                      result,
                                      to_expr(new_expr_eq(to_type(new_bool_type()),
                                                          rdup_expr(lexp->arr[ix]),
                                                          rdup_expr(rexp->arr[ix])))));
    }
    rfre_expr_list(lexp);
    rfre_expr_list(rexp);
    return result;
}

static formal_list expand_formal(formal parameter,
                                 const int fg_array,
                                 const_user_type_entry_list user_type_table)
{
    unsigned int pos;

    formal_list result = new_formal_list();
    if (fg_array && parameter->name->ranges != extent_listNIL && parameter->name->ranges->sz > 0) {
        extent_list extent = init_extent(parameter->name->ranges, index_entry_listNIL, user_type_table);
        do {
            formal q = new_formal(new_variable_identifier(rdup_orig_symbol(parameter->name->name), rdup_extent_list(extent), rdup_variable_qualifier_list(parameter->name->qualifiers)),
                                  rdup_type(parameter->type),
                                  parameter->offset,
                                  rdup_int_list(parameter->ref_count));
            formal_list formals = expand_formal(q, 0, user_type_table);
            concat_formal_list(result, formals);
        } while (extent_listNIL != (extent = advance_extent(extent, parameter->name->ranges, index_entry_listNIL, user_type_table)));
        return result;
    }
    if (parameter->type->tag == TAGuser_type &&
        search_user_type_entry_list_with_tag(user_type_table, to_user_type(parameter->type)->name->sym, TAGstruct_user_type_entry, &pos)) {
        struct_user_type_entry structure = to_struct_user_type_entry(user_type_table->arr[pos]);
        register unsigned int ix;
        for (ix = 0; ix < structure->entries->sz; ix++) {
            formal q = new_formal(new_variable_identifier(rdup_orig_symbol(structure->entries->arr[ix]), rdup_extent_list(structure->ranges->arr[ix]),
                                                          append_variable_qualifier_list(parameter->name->qualifiers == variable_qualifier_listNIL ? new_variable_qualifier_list() : rdup_variable_qualifier_list(parameter->name->qualifiers),
                                                                                         new_variable_qualifier(rdup_orig_symbol(parameter->name->name),
                                                                                                                rdup_extent_list(parameter->name->ranges)))),
                                  rdup_type(structure->types->arr[ix]),
                                  parameter->offset,
                                  rdup_int_list(parameter->ref_count));
            formal_list formals = expand_formal(q, 1, user_type_table);
            concat_formal_list(result, formals);
        }
        return result;
    }

    append_formal_list(result, rdup_formal(parameter));
    return result;
}

static local_list expand_local(local parameter,
                               const int fg_array,
                               const_user_type_entry_list user_type_table)
{
    register unsigned int ix;
    unsigned int pos;

    struct_user_type_entry structure;
    extent_list extent;
    local_list result;
    local_list locals;
    local q;

    result = new_local_list();
    if (fg_array &&
        parameter->name->ranges != extent_listNIL &&
        parameter->name->ranges->sz > 0) {
        extent = init_extent(parameter->name->ranges, index_entry_listNIL, user_type_table);
        do {
            q = new_local(new_variable_identifier(rdup_orig_symbol(parameter->name->name), rdup_extent_list(extent), rdup_variable_qualifier_list(parameter->name->qualifiers)),
                          rdup_type(parameter->type),
                          int_listNIL);
            locals = expand_local(q, 0, user_type_table);
            concat_local_list(result, locals);
            rfre_local(q);
        } while (extent_listNIL != (extent = advance_extent(extent, parameter->name->ranges, index_entry_listNIL, user_type_table)));
    } else if (parameter->type->tag == TAGuser_type &&
        search_user_type_entry_list_with_tag(user_type_table,
                                             to_user_type(parameter->type)->name->sym,
                                             TAGstruct_user_type_entry,
                                             &pos)) {
        structure = to_struct_user_type_entry(user_type_table->arr[pos]);
        for (ix = 0; ix < structure->entries->sz; ix++) {
            q = new_local(new_variable_identifier(rdup_orig_symbol(structure->entries->arr[ix]), rdup_extent_list(structure->ranges->arr[ix]),
                                                  append_variable_qualifier_list(parameter->name->qualifiers == variable_qualifier_listNIL ? new_variable_qualifier_list() : rdup_variable_qualifier_list(parameter->name->qualifiers),
                                                                                 new_variable_qualifier(rdup_orig_symbol(parameter->name->name),
                                                                                                        rdup_extent_list(parameter->name->ranges)))),
                                  rdup_type(structure->types->arr[ix]),
                                  rdup_int_list(parameter->ref_count));
            locals = expand_local(q, 1, user_type_table);
            concat_local_list(result, locals);
            rfre_local(q);
        }
    } else {
        append_local_list(result, rdup_local(parameter));
    }

    return result;
}

static attribute_list expand_attribute(const_attribute attr,
                                       const_user_type_entry_list user_type_table)
{
    attribute_list result = new_attribute_list();

    expr_list vars = expand_variable(attr->var, 1, user_type_table);

    register unsigned int ix;
    for (ix = 0; ix < vars->sz; ix++) {
        attribute new_attr;
        assert(TAGexpr_variable == to_expr_variable(vars->arr[ix])->tag);
        new_attr = new_attribute(rdup_type(attr->type),
                                 to_expr_variable(vars->arr[ix]),
                                 rdup_orig_symbol(attr->alias),
                                 rdup_expr(attr->value));
        append_attribute_list(result, new_attr);
    }
    fre_expr_list(vars);

    return result;
}

static void flatten_forall_predicate(predicate_list l,
                                     forall_predicate f,
                                     const_attribute_entry_list attribute_table,
                                     index_entry_list index_table,
                                     const_user_type_entry_list user_type_table)
{
    unsigned int pos;
    int from = eval_int_expr(f->ranges->from, index_table, user_type_table);
    int to = eval_int_expr(f->ranges->to, index_table, user_type_table);
    register int ix;
    register unsigned int iy;

    compound_predicate c = f->body;
    for (ix = from; ix <= to; ix++) {
        if (search_index_entry_list(index_table, f->id->sym, &pos)) {
            index_table->arr[pos]->value = ix;
        } else {
            append_index_entry_list(index_table, new_index_entry(f->id->sym, ix));
        }
        for (iy = 0; iy < c->predicates->sz; iy++) {
            flatten_system_predicates(c->predicates->arr[iy],
                                      l,
                                      attribute_table,
                                      index_table,
                                      user_type_table);
        }
    }
}

static void flatten_exists_predicate(predicate_list l,
                                     forall_predicate f,
                                     const_attribute_entry_list attribute_table,
                                     index_entry_list index_table,
                                     const_user_type_entry_list user_type_table)
{
    unsigned int pos;
    int from = eval_int_expr(f->ranges->from, index_table, user_type_table);
    int to = eval_int_expr(f->ranges->to, index_table, user_type_table);
    register int ix;
    register unsigned int iy;

    compound_predicate c = f->body;
    predicate_list n = new_predicate_list();
    for (ix = from; ix <= to; ix++) {
        if (search_index_entry_list(index_table, f->id->sym, &pos)) {
            index_table->arr[pos]->value = ix;
        } else {
            append_index_entry_list(index_table,
                                    new_index_entry(f->id->sym, ix));
        }
        for (iy = 0; iy < c->predicates->sz; iy++) {
            flatten_system_predicates(c->predicates->arr[iy],
                                      n,
                                      attribute_table,
                                      index_table,
                                      user_type_table);
        }
    }
    if (n->sz > 0) {
        simple_predicate s;
        assert(TAGsimple_predicate == n->arr[0]->tag);
        s = new_simple_predicate(originNIL, to_simple_predicate(n->arr[0])->x);
        for (iy = 1; iy < n->sz; iy++) {
            s->x = to_expr(new_expr_or(to_type(new_bool_type()), s->x, to_simple_predicate(n->arr[iy])->x));
        }
        append_predicate_list(l, to_predicate(s));
    }
    fre_predicate_list(n);
}

/**
 * Recursively replaces all the predicates with "simple" ones. A simple
 * predicate is a predicate containing an expression only. Most important,
 * "forall" and "exist" quantifiers are expanded, "compound predicates are
 * nested, "if" and "switch" predicates are expanded.
 *
 * @param p the current predicate;
 * @param l a list of simple predicate where the result goes. Must be
 *          an existing list;
 * @param attribute_table a table with all variable attributes;
 * @param index_table in this table we store the current indices of the
 *                    "forall" and "exists" index variables when unrolling
 *                    them.
 */
static void flatten_system_predicates(predicate p,
                                      predicate_list l,
                                      const_attribute_entry_list attribute_table,
                                      index_entry_list index_table,
                                      const_user_type_entry_list user_type_table)
{
    unsigned int ix, iy;

    predicate c;

    switch (p->tag) {
        case TAGsimple_predicate:
            c = rdup_predicate(p);
            bind_indices(to_simple_predicate(c)->x,
                         index_table,
                         user_type_table);
            append_predicate_list(l, c);
            break;
        case TAGforall_predicate:
            flatten_forall_predicate(l,
                                     to_forall_predicate(p),
                                     attribute_table,
                                     index_table,
                                     user_type_table);
            break;
        case TAGexists_predicate:
            flatten_exists_predicate(l,
                                     to_forall_predicate(p),
                                     attribute_table,
                                     index_table,
                                     user_type_table);
            break;
        case TAGcompound_predicate:
            for (ix = 0; ix < to_compound_predicate(p)->predicates->sz; ix++) {
                flatten_system_predicates(to_compound_predicate(p)->predicates->arr[ix],
                                          l,
                                          attribute_table,
                                          index_table,
                                          user_type_table);
            }
            break;
        case TAGif_predicate:
            if (to_if_predicate(p)->thenval != compound_predicateNIL) {
                predicate_list n = new_predicate_list();
                flatten_system_predicates(to_predicate(to_if_predicate(p)->thenval),
                                          n,
                                          attribute_table,
                                          index_table,
                                          user_type_table);
                for (ix = 0; ix < n->sz; ix++) {
                    expr cond = rdup_expr(to_if_predicate(p)->cond);
                    predicate q;
                    bind_indices(cond, index_table, user_type_table);
                    q = to_predicate(new_simple_predicate(rdup_origin(p->org), to_expr(new_expr_or(to_type(new_bool_type()), to_expr(new_expr_not(to_type(new_bool_type()), cond)), rdup_expr(to_simple_predicate(n->arr[ix])->x)))));
                    append_predicate_list(l, q);
                }
                rfre_predicate_list(n);
            }
            if (to_if_predicate(p)->elseval != compound_predicateNIL) {
                predicate_list n = new_predicate_list();
                flatten_system_predicates(to_predicate(to_if_predicate(p)->elseval), n, attribute_table, index_table, user_type_table);
                for (ix = 0; ix < n->sz; ix++) {
                    expr cond = rdup_expr(to_if_predicate(p)->cond);
                    predicate q;
                    bind_indices(cond, index_table, user_type_table);
                    q = to_predicate(new_simple_predicate(rdup_origin(p->org), to_expr(new_expr_or(to_type(new_bool_type()), cond, rdup_expr(to_simple_predicate(n->arr[ix])->x)))));
                    append_predicate_list(l, q);
                }
                rfre_predicate_list(n);
            }
            break;
        case TAGswitch_predicate:
            for (ix = 0; ix < to_switch_predicate(p)->choices->sz; ix++) {
                expr cond = to_expr(new_expr_ne(to_type(new_bool_type()), rdup_expr(to_switch_predicate(p)->lhs), rdup_expr(to_switch_predicate(p)->choices->arr[ix]->rhs)));
                predicate_list n;
                for (iy = 0; iy < ix; iy++) {
                    cond = to_expr(new_expr_or(to_type(new_bool_type()), cond, to_expr(new_expr_eq(to_type(new_bool_type()), rdup_expr(to_switch_predicate(p)->lhs), rdup_expr(to_switch_predicate(p)->choices->arr[iy]->rhs)))));
                }

                n = new_predicate_list();
                flatten_system_predicates(to_predicate(to_switch_predicate(p)->choices->arr[ix]->predicate), n, attribute_table, index_table, user_type_table);
                for (iy = 0; iy < n->sz; iy++) {
                    expr consequent = rdup_expr(to_simple_predicate(n->arr[iy])->x);
                    predicate q = to_predicate(new_simple_predicate(rdup_origin(p->org), to_expr(new_expr_or(to_type(new_bool_type()), rdup_expr(cond), consequent))));
                    append_predicate_list(l, q);
                }
                rfre_predicate_list(n);

                rfre_expr(cond);
            }
            if (compound_predicateNIL != to_switch_predicate(p)->deflt) {
                predicate_list n;
                expr cond = exprNIL;
                if (0 != to_switch_predicate(p)->choices->sz) {
                    cond = to_expr(new_expr_eq(to_type(new_bool_type()), rdup_expr(to_switch_predicate(p)->lhs), rdup_expr(to_switch_predicate(p)->choices->arr[0]->rhs)));
                    for (ix = 1; ix < to_switch_predicate(p)->choices->sz; ix++) {
                        cond = to_expr(new_expr_or(to_type(new_bool_type()), to_expr(new_expr_eq(to_type(new_bool_type()), rdup_expr(to_switch_predicate(p)->lhs), rdup_expr(to_switch_predicate(p)->choices->arr[ix]->rhs))), cond));
                    }
                }

                n = new_predicate_list();
                flatten_system_predicates(to_predicate(to_switch_predicate(p)->deflt), n, attribute_table, index_table, user_type_table);
                for (ix = 0; ix < n->sz; ix++) {
                    expr consequent = rdup_expr(to_simple_predicate(n->arr[ix])->x);
                    predicate q = predicateNIL;
                    if (exprNIL != cond) {
                        q = to_predicate(new_simple_predicate(rdup_origin(p->org), to_expr(new_expr_or(to_type(new_bool_type()), rdup_expr(cond), consequent))));
                    } else {
                        q = to_predicate(new_simple_predicate(rdup_origin(p->org), consequent));
                    }
                    append_predicate_list(l, q);
                }
                rfre_predicate_list(n);

                rfre_expr(cond);
            }
            break;
        case TAGvariable_declaration:
            if (TAGuser_type == to_variable_declaration(p)->type->tag && is_attribute(attribute_table, to_user_type(to_variable_declaration(p)->type)->name->sym)) {
                break;
            }
            for (ix = 0; ix < to_variable_declaration(p)->instances->sz; ix++) {
                if (exprNIL != to_variable_declaration(p)->instances->arr[ix]->val) {
                    if (to_variable_declaration(p)->instances->arr[ix]->val->type != typeNIL && TAGuser_type == to_variable_declaration(p)->instances->arr[ix]->val->type->tag && is_attribute(attribute_table, to_user_type(to_variable_declaration(p)->instances->arr[ix]->val->type)->name->sym)) {
                        continue;
                    }
                    append_predicate_list(l, to_predicate(new_simple_predicate(rdup_origin(to_variable_declaration(p)->org), to_expr(new_expr_eq(to_type(new_bool_type()), to_expr(new_expr_variable(rdup_type(to_variable_declaration(p)->type), new_variable_identifier(rdup_orig_symbol(to_variable_declaration(p)->instances->arr[ix]->name), extent_listNIL/* TODO: What is the extent list. */, variable_qualifier_listNIL))), rdup_expr(to_variable_declaration(p)->instances->arr[ix]->val))))));
                }
            }
            break;
        case TAGsystem_declaration:
            for (ix = 0; ix < to_system_declaration(p)->instances->sz; ix++) {
                if (expr_listNIL != to_system_declaration(p)->instances->arr[ix]->arguments) {
                    simple_predicate s = new_simple_predicate(rdup_origin(to_variable_declaration(p)->org),
                                                              to_expr(new_expr_apply(to_type(new_user_type(rdup_orig_symbol(to_system_declaration(p)->name))),
                                                                                     rdup_orig_symbol(to_system_declaration(p)->instances->arr[ix]->name),
                                                                                     rdup_extent_list(to_system_declaration(p)->instances->arr[ix]->ranges),
                                                                                     rdup_expr_list(to_system_declaration(p)->instances->arr[ix]->arguments))));
                    append_predicate_list(l, to_predicate(s));
                }
            }
            break;
        case TAGattribute_declaration:
/* Noop. */
            break;
    }
}

void flatten_predicates(model m,
                        const_attribute_entry_list attribute_table,
                        index_entry_list index_table,
                        const_user_type_entry_list user_type_table)
{
    register unsigned int ix;
    for (ix = 0; ix < m->defs->sz; ix++) {
        predicate_list predicates;
        if (m->defs->arr[ix]->tag != TAGsystem_definition) {
            continue;
        }
        predicates = new_predicate_list();
        flatten_system_predicates(to_predicate(to_system_definition(m->defs->arr[ix])->predicates),
                                  predicates,
                                  attribute_table,
                                  index_table,
                                  user_type_table);
        rfre_predicate_list(to_system_definition(m->defs->arr[ix])->predicates->predicates);
        to_system_definition(m->defs->arr[ix])->predicates->predicates = predicates;
    }
}

static expr arith_switch_to_ifs(expr_cond ex)
{
    register unsigned int ix;

    expr *tail = NULL, result = exprNIL;
    expr cond = ex->lhs;

    assert(ex->choices != choice_listNIL);
    assert(ex->choices->sz > 0);

    for (ix = 0; ix < ex->choices->sz; ix++) {
        expr_if_else next = new_expr_if_else(rdup_type(ex->choices->arr[ix]->val->type), exprNIL, exprNIL, exprNIL);
        next->cond = to_expr(new_expr_eq(to_type(new_bool_type()), rdup_expr(cond), rdup_expr(ex->choices->arr[ix]->cond)));
        next->thenval = rdup_expr(ex->choices->arr[ix]->val);
        if (ix == 0) {
            result = to_expr(next);
        } else {
            *tail = to_expr(next);
        }
        tail = &next->elseval;
    }
    if (exprNIL != ex->deflt) {
        *tail = rdup_expr(ex->deflt);
    }

    return result;
}

static expr_variable make_satvar(expr e)
{
    expr_variable result = expr_variableNIL;
    char *buf = strdup(to_expr_apply(e)->name->sym->name);
    register size_t ix = strlen(buf);
    buf = realloc(buf, ix + 2);
    if (NULL == buf) {
        /* To Do: Proper out-of-memory handling. */
        assert(0);
        abort();
    }
    memmove(buf + 1, buf, ix + 1);
    buf[0] = '$';

    result = new_expr_variable(to_type(new_bool_type()),
                               new_variable_identifier(new_orig_symbol(add_lydia_symbol(buf), originNIL),
                                                       extent_listNIL,
                                                       variable_qualifier_listNIL));
    free(buf);

    return result;
}

static expr inline_expr(system_definition sys,
                        expr e,
                        const_formal_list formals,
                        const_local_list locals,
                        const_expr_list func_args,
                        const_user_type_entry_list user_type_table,
                        int context)
{
    unsigned int pos, ix;

    switch (e->tag) {
        case TAGexpr_variable:
            if (expr_listNIL != func_args &&
                search_formal_list(formals, to_expr_variable(e)->name, &pos)) {
/* We are inside a function body and we want to replace the formal expression with the argument. */
                rfre_expr(e);
                return inline_expr(sys,
                                   rdup_expr(func_args->arr[formals->arr[pos]->offset]),
                                   formals,
                                   locals,
                                   expr_listNIL,
                                   user_type_table,
                                   TAGexpr_variable);
            }
/* Now we are anywhere in the AST and we want to inline a constant. */
            if (search_user_type_entry_list_with_tag(user_type_table, to_expr_variable(e)->name->name->sym, TAGconstant_user_type_entry, &pos)) {
                rfre_expr(e);
                return inline_expr(sys,
                                   rdup_expr(to_function_user_type_entry(user_type_table->arr[pos])->value),
                                   formals,
                                   locals,
                                   func_args,
                                   user_type_table,
                                   TAGexpr_variable);
            }
            if (variable_qualifier_listNIL != to_expr_variable(e)->name->qualifiers &&
                to_expr_variable(e)->name->qualifiers->sz > 0 &&
                search_user_type_entry_list_with_tag(user_type_table, to_expr_variable(e)->name->qualifiers->arr[0]->name->sym, TAGenum_user_type_entry, &pos)) {
/* This is an enumeration constant. */
                expr result = to_expr(new_expr_enum(rdup_type(e->type),
                                                    to_expr_variable(e)->name->qualifiers->arr[0]->name->sym,
                                                    to_expr_variable(e)->name->name->sym));
                rfre_expr(e);
                return result;
            }
            break;

/* Literals: */
        case TAGexpr_bool:
        case TAGexpr_int:
        case TAGexpr_float:
        case TAGexpr_enum:
/* Noop. */
            break;

/* Binary expressions: */
        case TAGexpr_add:
        case TAGexpr_mult:
        case TAGexpr_and:
        case TAGexpr_or:
        case TAGexpr_imply:
        case TAGexpr_lt:
        case TAGexpr_gt:
        case TAGexpr_le:
        case TAGexpr_ge:
        case TAGexpr_ne:
        case TAGexpr_sub:
        case TAGexpr_div:
        case TAGexpr_mod:
            to_expr_binop(e)->l = inline_expr(sys,
                                              to_expr(to_expr_binop(e)->l),
                                              formals,
                                              locals,
                                              func_args,
                                              user_type_table,
                                              TAGexpr_mod);
            to_expr_binop(e)->r = inline_expr(sys,
                                              to_expr(to_expr_binop(e)->r),
                                              formals,
                                              locals,
                                              func_args,
                                              user_type_table,
                                              TAGexpr_mod);
            break;

/* Unary expressions: */
        case TAGexpr_not:
        case TAGexpr_negate:
            to_expr_unop(e)->x = inline_expr(sys,
                                             to_expr(to_expr_unop(e)->x),
                                             formals,
                                             locals,
                                             func_args,
                                             user_type_table,
                                             TAGexpr_negate);
            break;

/* Others: */
        case TAGexpr_if_else:
            to_expr_if_else(e)->cond = inline_expr(sys,
                                                   to_expr(to_expr_if_else(e)->cond),
                                                   formals,
                                                   locals,
                                                   func_args,
                                                   user_type_table,
                                                   TAGexpr_if_else);
            to_expr_if_else(e)->thenval = inline_expr(sys,
                                                      to_expr(to_expr_if_else(e)->thenval),
                                                      formals,
                                                      locals,
                                                      func_args,
                                                      user_type_table,
                                                      TAGexpr_if_else);
            to_expr_if_else(e)->elseval = inline_expr(sys,
                                                      to_expr(to_expr_if_else(e)->elseval),
                                                      formals,
                                                      locals,
                                                      func_args,
                                                      user_type_table,
                                                      TAGexpr_if_else);
            break;
        case TAGexpr_cond:
            to_expr_cond(e)->lhs = inline_expr(sys,
                                               to_expr(to_expr_cond(e)->lhs),
                                               formals,
                                               locals,
                                               func_args,
                                               user_type_table,
                                               TAGexpr_cond);
            for (ix = 0; ix < to_expr_cond(e)->choices->sz; ix++) {
                to_expr_cond(e)->choices->arr[ix]->cond = inline_expr(sys,
                                                                      to_expr(to_expr_cond(e)->choices->arr[ix]->cond),
                                                                      formals,
                                                                      locals,
                                                                      func_args,
                                                                      user_type_table,
                                                                      TAGexpr_cond);
                to_expr_cond(e)->choices->arr[ix]->val = inline_expr(sys,
                                                                     to_expr(to_expr_cond(e)->choices->arr[ix]->val),
                                                                     formals,
                                                                     locals,
                                                                     func_args,
                                                                     user_type_table,
                                                                     TAGexpr_cond);
            }
            if (exprNIL != to_expr_cond(e)->deflt) {
                to_expr_cond(e)->deflt = inline_expr(sys,
                                                     to_expr(to_expr_cond(e)->deflt),
                                                     formals,
                                                     locals,
                                                     func_args,
                                                     user_type_table,
                                                     TAGexpr_cond);
            }
            break;
        case TAGexpr_apply:
            for (ix = 0; ix < to_expr_apply(e)->parms->sz; ix++) {
                to_expr_apply(e)->parms->arr[ix] = inline_expr(sys,
                                                               to_expr(to_expr_apply(e)->parms->arr[ix]),
                                                               formals,
                                                               locals,
                                                               func_args,
                                                               user_type_table,
                                                               TAGexpr_apply);
            }
/*
 * If an argument is an expression different from a variable, replace it with
 * an anonymous variable and a new simple predicate. We need for the
 * hierarchical model.
 */
            assert(typeNIL != e->type);
/*
 * Replace all structures and arrays with the list of all elements in these
 * structures or arrays.
 */
            for (ix = to_expr_apply(e)->parms->sz - 1; ix < to_expr_apply(e)->parms->sz; ix--) {
                expr_list exp = expand_expr(to_expr_apply(e)->parms->arr[ix], formals, locals, user_type_table);
                delete_expr_list(to_expr_apply(e)->parms, ix);
                insertlist_expr_list(to_expr_apply(e)->parms, ix, exp);
            }
/* Replace function calls with the body of the function. */
            if (TAGuser_type == e->type->tag &&
                search_user_type_entry_list_with_tag(user_type_table,
                                                     to_expr_apply(e)->name->sym,
                                                     TAGfunction_user_type_entry,
                                                     &pos)) {
                function_user_type_entry func = to_function_user_type_entry(user_type_table->arr[pos]);
                expr result = inline_expr(sys,
                                          rdup_expr(func->value),
                                          func->formals,
                                          locals,
                                          to_expr_apply(e)->parms,
                                          user_type_table,
                                          TAGexpr_apply);
                rfre_expr(e);
                return result;
            }
            if (TAGuser_type == e->type->tag &&
                search_user_type_entry_list_with_tag(user_type_table,
                                                     to_user_type(to_expr_apply(e)->type)->name->sym,
                                                     TAGsystem_user_type_entry,
                                                     &pos)) {
                for (ix = 0; ix < to_expr_apply(e)->parms->sz; ix++) {
                    expr rhs;
                    lydia_symbol name;
                    expr_variable lhs;
                    simple_predicate eq;
                    char *buf;
/*
 * The parameters of a system reference should all be variable expressions.
 * If they are not, generate a variabe and append an extra equation.
 */
                    if (TAGexpr_variable == to_expr_apply(e)->parms->arr[ix]->tag) {
                        continue;
                    }
                    rhs = to_expr_apply(e)->parms->arr[ix];
                    buf = malloc(strlen(to_expr_apply(e)->name->sym->name) + 128);
                    sprintf(buf, "$%s#%d", to_expr_apply(e)->name->sym->name, ix + 1);
                    name = add_lydia_symbol(buf);
                    lhs = new_expr_variable(rdup_type(rhs->type),
                                            new_variable_identifier(new_orig_symbol(name, originNIL),
                                                                    extent_listNIL,
                                                                    variable_qualifier_listNIL));
                    free(buf);
                    eq = new_simple_predicate(originNIL, to_expr(new_expr_eq(to_type(new_bool_type()), to_expr(lhs), rhs)));

                    append_predicate_list(sys->predicates->predicates, to_predicate(eq));
                    to_expr_apply(e)->parms->arr[ix] = rdup_expr(to_expr(lhs));
                }
                if (context != -1) {
                    expr_variable result = expr_variableNIL;
                    expr_variable parm = expr_variableNIL;

                    if (!member_lydia_symbol_list(expref_systems, to_user_type(to_expr_apply(e)->type)->name->sym)) {
                        append_lydia_symbol_list(expref_systems, rdup_lydia_symbol(to_user_type(to_expr_apply(e)->type)->name->sym));
                    }
                    result = make_satvar(e);
                    parm = rdup_expr_variable(result);

                    append_expr_list(to_expr_apply(e)->parms, to_expr(parm));
                    append_predicate_list(sys->predicates->predicates,
                                          to_predicate(new_simple_predicate(originNIL, e)));
                    return to_expr(result);
                }
            }
            break;
        case TAGexpr_cast:
            to_expr_cast(e)->v = inline_expr(sys,
                                             to_expr(to_expr_cast(e)->v),
                                             formals,
                                             locals,
                                             func_args,
                                             user_type_table,
                                             TAGexpr_cast);
            break;
        case TAGexpr_concatenation:
            for (ix = 0; ix < to_expr_concatenation(e)->l->sz; ix++) {
                to_expr_concatenation(e)->l->arr[ix] = inline_expr(sys,
                                                                   to_expr(to_expr_concatenation(e)->l->arr[ix]),
                                                                   formals,
                                                                   locals,
                                                                   func_args,
                                                                   user_type_table,
                                                                   TAGexpr_concatenation);
            }
            break;
        case TAGexpr_eq:
            to_expr_binop(e)->l = inline_expr(sys,
                                              to_expr(to_expr_binop(e)->l),
                                              formals,
                                              locals,
                                              func_args,
                                              user_type_table,
                                              TAGexpr_eq);
            to_expr_binop(e)->r = inline_expr(sys,
                                              to_expr(to_expr_binop(e)->r),
                                              formals,
                                              locals,
                                              func_args,
                                              user_type_table,
                                              TAGexpr_eq);
            if (to_expr_binop(e)->l->tag == TAGexpr_concatenation ||
                to_expr_binop(e)->l->tag == TAGexpr_if_else ||
                to_expr_binop(e)->l->tag == TAGexpr_cond) {
/* Move the concatenation or the arithmetic if to the right. */
                expr xchg = to_expr_binop(e)->l;
                to_expr_binop(e)->l = to_expr_binop(e)->r;
                to_expr_binop(e)->r = xchg;
            }
            if (is_constant_or_variable(to_expr_binop(e)->l) &&
                to_expr_binop(e)->r->tag == TAGexpr_cond) {
                expr result = arith_switch_to_ifs(to_expr_cond(to_expr_binop(e)->r));
                rfre_expr(to_expr_binop(e)->r);
                to_expr_binop(e)->r = result;
                return inline_expr(sys,
                                   e,
                                   formals,
                                   locals,
                                   func_args,
                                   user_type_table,
                                   TAGexpr_eq);
            }
/*
 * Now replace "<variable> = (<cond> ? <pos> : <neg>)" with
 * "(<cond> => (<variable> = <pos>)) && (!<cond> => (<variable> = <neg>))".
 */
            if (is_constant_or_variable(to_expr_binop(e)->l) &&
                to_expr_binop(e)->r->tag == TAGexpr_if_else) {
                expr cond = to_expr_if_else(to_expr_binop(e)->r)->cond;
                expr pos = to_expr_if_else(to_expr_binop(e)->r)->thenval;
                expr neg = to_expr_if_else(to_expr_binop(e)->r)->elseval;
                expr variable = to_expr_binop(e)->l;
                expr result = to_expr(new_expr_if_else(to_type(new_bool_type()),
                                                       cond,
                                                       to_expr(new_expr_eq(to_type(new_bool_type()), variable, pos)),
                                                       to_expr(new_expr_eq(to_type(new_bool_type()), rdup_expr(variable), neg))));
                fre_expr(e);
                return inline_expr(sys,
                                   result,
                                   formals,
                                   locals,
                                   func_args,
                                   user_type_table,
                                   TAGexpr_eq);
            }

            if ((to_expr_binop(e)->l->tag == TAGexpr_variable ||
                 to_expr_binop(e)->l->tag == TAGexpr_concatenation) &&
                (to_expr_binop(e)->r->tag == TAGexpr_variable ||
                 to_expr_binop(e)->r->tag == TAGexpr_concatenation)) {
                expr result = rewrite_equation(to_expr_binop(e)->l,
                                               to_expr_binop(e)->r,
                                               formals,
                                               locals,
                                               user_type_table);
                rfre_expr(e);
                return result;
            }
            break;
    };

    return e;
}

void append_system_satvar(system_definition sys)
{
    register unsigned int ix;

    predicate_list predicates = sys->predicates->predicates;
    predicate result;
    expr rhs = exprNIL;
    expr lhs = to_expr(new_expr_variable(to_type(new_bool_type()), new_variable_identifier(new_orig_symbol(add_lydia_symbol("$this"), originNIL), extent_listNIL, variable_qualifier_listNIL)));
    expr eq;

    if (0 == predicates->sz) {
        return;
    }

    for (ix = 0; ix < predicates->sz; ix++) {
        assert(predicates->arr[ix]->tag == TAGsimple_predicate);
        if (exprNIL == rhs) {
            rhs = rdup_expr(to_simple_predicate(predicates->arr[ix])->x);
            continue;
        }
        rhs = to_expr(new_expr_and(to_type(new_bool_type()),
                                   rhs,
                                   rdup_expr(to_simple_predicate(predicates->arr[ix])->x)));
    }

    eq = to_expr(new_expr_eq(to_type(new_bool_type()), lhs, rhs));
    result = to_predicate(new_simple_predicate(originNIL, eq));

    rfre_predicate_list(sys->predicates->predicates);
    sys->predicates->predicates = append_predicate_list(new_predicate_list(), result);
}

void rewrite_systems(model m, user_type_entry_list user_type_table)
{
    register unsigned int ix, iy;

    expref_systems = new_lydia_symbol_list();
    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGsystem_definition) {
            system_definition sys = to_system_definition(m->defs->arr[ix]);
            formal_list formals = sys->formals;
            local_list locals = sys->locals;
            attribute_list attributes = sys->attributes;
            for (iy = 0; iy < sys->predicates->predicates->sz; iy++) {
                if (sys->predicates->predicates->arr[iy]->tag == TAGsimple_predicate) {
                    simple_predicate predicate = to_simple_predicate(sys->predicates->predicates->arr[iy]);
                    predicate->x = inline_expr(sys,
                                               predicate->x,
                                               sys->formals,
                                               sys->locals,
                                               expr_listNIL,
                                               user_type_table,
                                               -1);
                }
            }
            for (iy = 0; iy < sys->attributes->sz; iy++) {
/*
 * To Do: We can't have a system instantiation in an attribute. Check
 * for this and produce an error.
 */
                sys->attributes->arr[iy]->value = inline_expr(sys,
                                                              sys->attributes->arr[iy]->value,
                                                              sys->formals,
                                                              sys->locals,
                                                              expr_listNIL,
                                                              user_type_table,
                                                              -1);
            }

/**
 * Expand all the array and structure attributes to a list of attributes.
 * Note that should happen before we expand the arrays and structures
 * themselves.
 */
            for (iy = attributes->sz - 1; iy < attributes->sz; iy--) {
                attribute_list exp = expand_attribute(attributes->arr[iy], user_type_table);
                delete_attribute_list(attributes, iy);
                insertlist_attribute_list(attributes, iy, exp);
            }

/**
 * Expand all the structures and arrays in the formals list, replacing
 * them with a list of variables.
 */
            for (iy = formals->sz - 1; iy < formals->sz; iy--) {
                formal_list exp;
                exp = expand_formal(formals->arr[iy], 1, user_type_table);
                delete_formal_list(formals, iy);
                insertlist_formal_list(formals, iy, exp);
            }
/**
 * Expand all the structures and arrays in the locals list, replacing
 * them with a list of variables.
 */
            for (iy = locals->sz - 1; iy < locals->sz; iy--) {
                local_list exp;
                exp = expand_local(locals->arr[iy], 1, user_type_table);
                delete_local_list(locals, iy);
                insertlist_local_list(locals, iy, exp);
            }
        }
    }
    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGconstant_definition || m->defs->arr[ix]->tag == TAGfunction_definition) {
            m->defs = delete_definition_list(m->defs, ix);
            ix -= 1;
        }
    }
    for (ix = 0; ix < m->defs->sz; ix++) {
        system_definition sys;
        if (m->defs->arr[ix]->tag != TAGsystem_definition) {
            continue;
        }
        sys = to_system_definition(m->defs->arr[ix]);
        if (member_lydia_symbol_list(expref_systems, m->defs->arr[ix]->name->sym)) {
            system_definition sys = to_system_definition(m->defs->arr[ix]);
            formal this = new_formal(new_variable_identifier(new_orig_symbol(add_lydia_symbol("$this"), originNIL), extent_listNIL, variable_qualifier_listNIL),
                                     to_type(new_bool_type()),
                                     sys->formals->sz,
                                     append_int_list(new_int_list(), 1));
            append_formal_list(sys->formals, this);
            append_system_satvar(sys);
        }

        assert(sys->predicates->predicates != predicate_listNIL);
        for (iy = 0; iy < sys->predicates->predicates->sz; iy++) {
/* All predicates should be inlined to simple predicates at this point. */
            simple_predicate predicate = to_simple_predicate(sys->predicates->predicates->arr[iy]);
            expr_apply inst;

            assert(sys->predicates->predicates->arr[iy]->tag == TAGsimple_predicate);
            if (TAGexpr_apply == predicate->x->tag) {
                inst = to_expr_apply(predicate->x);
                if (TAGuser_type != inst->type->tag ||
                    !member_lydia_symbol_list(expref_systems, to_user_type(inst->type)->name->sym)) {
                    continue;
                }
                if (0 == inst->parms->sz || 
                    inst->parms->arr[inst->parms->sz - 1]->tag != TAGexpr_variable ||
                    '$' != to_expr_variable(inst->parms->arr[inst->parms->sz - 1])->name->name->sym->name[0] ||
                    0 != strcmp(inst->name->sym->name, to_expr_variable(inst->parms->arr[inst->parms->sz - 1])->name->name->sym->name + 1)) {
                    append_expr_list(inst->parms, to_expr(make_satvar(to_expr(inst))));
                }
            }
        }
    }
    rfre_lydia_symbol_list(expref_systems);
}
