#include "typeinfer.h"
#include "applyexpr.h"
#include "varexpr.h"
#include "search.h"
#include "config.h"
#include "array.h"
#include "error.h"
#include "defs.h"
#include "list.h"
#include "attr.h"
#include "expr.h"
#include "iter.h"
#include "lc.h"

#include <assert.h>

lydia_symbol_list undefined = lydia_symbol_listNIL;

#define ALLOW_BOOL    0x0001
#define ALLOW_INT     0x0002
#define ALLOW_FLOAT   0x0004
#define ALLOW_USER    0x0008

static const char *get_type_name(type t)
{
    switch (t->tag) {
        case TAGbool_type:
            return "bool";
        case TAGint_type:
            return "int";
        case TAGfloat_type:
            return "float";
        case TAGvariable_type:
            return "variable";
        case TAGuser_type:
            return to_user_type(t)->name->sym->name;
    }
    assert(0);
    abort();
}

static char *get_unary_operator_name(int tag)
{
    switch (tag) {
        case TAGexpr_negate:
            return "unary -";
        case TAGexpr_not:
            return "unary !";
    }
    assert(0);
    abort();
}

static char *get_binary_operator_name(int tag)
{
    switch (tag) {
        case TAGexpr_add:
            return "binary +";
        case TAGexpr_sub:
            return "binary -";
        case TAGexpr_mult:
            return "binary *";
        case TAGexpr_div:
            return "binary /";
        case TAGexpr_mod:
            return "binary mod";
        case TAGexpr_and:
            return "and";
        case TAGexpr_or:
            return "or";
        case TAGexpr_imply:
            return "binary =>";
        case TAGexpr_eq:
            return "binary =";
        case TAGexpr_ne:
            return "binary !=";
    }
    assert(0);
    abort();
}

static type resolve_type(type t, const_user_type_entry_list user_type_table)
{
    unsigned int pos;

    if (typeNIL == t) {
        return typeNIL;
    }
    if (TAGuser_type != t->tag) {
        return t;
    }
    if (!search_user_type_entry_list(user_type_table,
                                     to_user_type(t)->name->sym,
                                     &pos)) {
        return typeNIL;
    }

    if (user_type_table->arr[pos]->tag == TAGfunction_user_type_entry) {
        return to_function_user_type_entry(user_type_table->arr[pos])->type;
    } else if (user_type_table->arr[pos]->tag == TAGconstant_user_type_entry) {
        return to_constant_user_type_entry(user_type_table->arr[pos])->type;
    } else if (user_type_table->arr[pos]->tag == TAGsystem_user_type_entry) {
        return to_system_user_type_entry(user_type_table->arr[pos])->type;
    } else if (user_type_table->arr[pos]->tag == TAGalias_user_type_entry) {
        return to_alias_user_type_entry(user_type_table->arr[pos])->type;
    }
    return t;
}

int compatible_types(type l, type r, const_user_type_entry_list user_type_table)
{
    l = resolve_type(l, user_type_table);
    r = resolve_type(r, user_type_table);
    if (typeNIL == l || typeNIL == r) {
        return 1;  /* We have an unresolved type, supress a second error. */
    }

    if (l->tag == TAGvariable_type || r->tag == TAGvariable_type) {
        return 1; /* @todo: some extra checks */
    }

    if (l->tag == TAGuser_type && r->tag == TAGuser_type) {
        unsigned int lpos, rpos;

        if (!search_user_type_entry_list(user_type_table, to_user_type(l)->name->sym, &lpos) ||
            !search_user_type_entry_list(user_type_table, to_user_type(r)->name->sym, &rpos)) {
            assert(0);
            abort();
        }

        if (user_type_table->arr[lpos]->tag == user_type_table->arr[rpos]->tag) {
            if (user_type_table->arr[lpos]->tag == TAGenum_user_type_entry) {
                return to_enum_user_type_entry(user_type_table->arr[lpos])->name->sym == to_enum_user_type_entry(user_type_table->arr[rpos])->name->sym;
            }
            if (user_type_table->arr[lpos]->tag == TAGstruct_user_type_entry) {
                return to_struct_user_type_entry(user_type_table->arr[lpos])->name->sym == to_enum_user_type_entry(user_type_table->arr[rpos])->name->sym;
            }
        }
        return 0;
    }
/*
 * The conditional below handles the cases in which a system
 * instantiation has been used in a Boolean expression.
 */
    if ((l->tag == TAGbool_type && r->tag == TAGuser_type) ||
        (l->tag == TAGuser_type && r->tag == TAGbool_type)) {
        unsigned int pos;

        type u = l->tag == TAGuser_type ? l : r;

        if (!search_user_type_entry_list(user_type_table, to_user_type(u)->name->sym, &pos)) {
            assert(0);
            abort();
        }
        if (user_type_table->arr[pos]->tag == TAGsystem_user_type_entry) {
            return 1;
        }
        return 0;
    }
    return l->tag == r->tag;
}

static int check_binary_operator_types(expr_binop e, const_origin orig, int allowed, const_user_type_entry_list user_type_table)
{
    type t;

    if (e->l->type == typeNIL || e->r->type == typeNIL) {
        return 0;
    }

    if (!compatible_types(e->l->type, e->r->type, user_type_table)) {
/* To Do: Show in which system is that error. */
        leh_error(ERR_TYPE_BINOP,
                  LEH_LOCATION_GLOBAL,
                  orig,
                  get_binary_operator_name(e->tag));
        return 0;
    }
    t = resolve_type(e->l->type, user_type_table);
    if (typeNIL == t) {
        return 1;  /* We have an unresolved type, supress a second the error. */
    }
    
    if ((t->tag == TAGbool_type && (allowed & ALLOW_BOOL)) ||
        (t->tag == TAGint_type && (allowed & ALLOW_INT)) ||
        (t->tag == TAGfloat_type && (allowed & ALLOW_FLOAT)) ||
        (t->tag == TAGuser_type && (allowed & ALLOW_USER))) {
        return 1;
    }

/* To Do: Show in which system is that error. */
    leh_error(ERR_TYPE_BINOP,
              LEH_LOCATION_GLOBAL,
              orig,
              get_binary_operator_name(e->tag));

    return 0;
}

unsigned int expr_size(const_expr e,
                       const_index_entry_list indices,
                       const_user_type_entry_list user_type_table)
{
    if (TAGexpr_not == e->tag) {
        return expr_size(to_expr_not(e)->x,
                         indices,
                         user_type_table);
    } else if (TAGexpr_add == e->tag ||
               TAGexpr_sub == e->tag ||
               TAGexpr_mult == e->tag ||
               TAGexpr_div == e->tag ||
               TAGexpr_mod == e->tag ||
               TAGexpr_and == e->tag ||
               TAGexpr_or == e->tag ||
               TAGexpr_imply == e->tag ||
               TAGexpr_lt == e->tag ||
               TAGexpr_gt == e->tag ||
               TAGexpr_le == e->tag ||
               TAGexpr_ge == e->tag ||
               TAGexpr_eq == e->tag ||
               TAGexpr_ne == e->tag) {
        return expr_size(to_expr_binop(e)->l,
                         indices,
                         user_type_table);
    } else if (TAGexpr_if_else == e->tag) {
        return expr_size(to_expr_if_else(e)->thenval,
                         indices,
                         user_type_table);
    } else if (TAGexpr_cond == e->tag) {
        assert(to_expr_cond(e)->choices->sz > 0);
        return expr_size(to_expr_cond(e)->choices->arr[0]->val,
                         indices,
                         user_type_table);
    } else if (TAGexpr_float == e->tag ||
               TAGexpr_bool == e->tag ||
               TAGexpr_int == e->tag ||
               TAGexpr_enum == e->tag) {
        return 1;
    } else if (TAGexpr_concatenation == e->tag) {
        register unsigned int ix;
        unsigned int size = 0;
        for (ix = 0; ix < to_expr_concatenation(e)->l->sz; ix++) {
            size += expr_size(to_expr_concatenation(e)->l->arr[ix],
                              indices,
                              user_type_table);
        }
        return size;
    } else if (TAGexpr_variable == e->tag) {
        unsigned int size = variable_declaration_size(e->type,
                                                      indices,
                                                      user_type_table);
        if (to_expr_variable(e)->name->ranges != extent_listNIL &&
            to_expr_variable(e)->name->ranges->sz > 0) {
            size *= lc_array_size(to_expr_variable(e)->name->ranges,
                                  indices,
                                  user_type_table);
        }
        return size;
    } else if (TAGexpr_apply == e->tag) {
        unsigned int pos;
        if (!search_user_type_entry_list(user_type_table,
                                         to_expr_apply(e)->name->sym,
                                         &pos)) {
            assert(0);
            abort();
        }
        if (user_type_table->arr[pos]->tag == TAGfunction_user_type_entry) {
            return expr_size(to_function_user_type_entry(user_type_table->arr[pos])->value, indices, user_type_table);
        } 
        assert(0);
        abort();
    }

    assert(0);
    abort();
}

static int check_binary_operator_extents(const_expr_binop e,
                                         const_origin orig,
                                         const int fg_function,
                                         const_lydia_symbol location,
                                         const_quantifier_entry_list quantifier_table,
                                         const_user_type_entry_list user_type_table)
{
    index_entry_list indices = index_entry_listNIL;

    if (e->l->type == typeNIL || e->r->type == typeNIL) {
/* Suppress chained errors. */
        return 0;
    }

    if (quantifier_table->sz > 0) {
        indices = init_quantifier_table(quantifier_table, user_type_table);
    }
    do {
        if (expr_size(e->l, indices, user_type_table) !=
            expr_size(e->r, indices, user_type_table)) {
            leh_error(ERR_TYPE_BINOP_SIZES,
                      fg_function,
                      orig,
                      location->name,
                      get_binary_operator_name(e->tag));
            return 0;
        }
    } while (quantifier_table->sz > 0 &&
             index_entry_listNIL != (indices = advance_quantifier_table(indices, quantifier_table, user_type_table)));
        
    return 1;
}

static int check_unary_operator_type(expr_unop e, const_origin orig, int allowed, const_user_type_entry_list user_type_table)
{
    type t;

    if (to_expr_unop(e)->x->type == typeNIL) {
        return 0;
    }

    t = resolve_type(e->x->type, user_type_table);
    if (typeNIL == t) {
        return 1;  /* We have an unresolved type, supress a second the error. */
    }

    if ((t->tag == TAGbool_type && (allowed & ALLOW_BOOL)) ||
        (t->tag == TAGint_type && (allowed & ALLOW_INT)) ||
        (t->tag == TAGfloat_type && (allowed & ALLOW_FLOAT)) ||
        (t->tag == TAGuser_type && (allowed & ALLOW_USER))) {
        return 1;
    }

/* To Do: Show in what system is the type mismatch. */
    leh_error(ERR_TYPE_UNOP, LEH_LOCATION_GLOBAL, orig, get_unary_operator_name(e->tag));

    return 0;
}

int infer_types_expr(expr e,
                     const_user_type_entry_list user_type_table,
                     quantifier_entry_list quantifier_table,
                     const_origin orig,
                     formal_list formals,
                     local_list locals,
                     reference_list references,
                     int attr,
                     lydia_symbol where,
                     int function)
{
    unsigned int ix;

    int result = 1;

    type src, dst;

    switch (e->tag) {
        case TAGexpr_variable:
            result = infer_types_expr_variable(to_expr_variable(e),
                                               orig,
                                               user_type_table,
                                               quantifier_table,
                                               formals,
                                               locals,
                                               references,
                                               where,
                                               attr,
                                               function);
            break;
/* Binary expressions: */
        case TAGexpr_add:
        case TAGexpr_mult:
            result &= infer_types_expr(to_expr(to_expr_binop(e)->l), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= infer_types_expr(to_expr(to_expr_binop(e)->r), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= check_binary_operator_types(to_expr_binop(e), orig, ALLOW_BOOL | ALLOW_INT | ALLOW_FLOAT, user_type_table);
            if (result) {
                e->type = rdup_type(to_expr_binop(e)->l->type);
            }
            break;
        case TAGexpr_and:
        case TAGexpr_or:
        case TAGexpr_imply:
            result &= infer_types_expr(to_expr(to_expr_binop(e)->l), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= infer_types_expr(to_expr(to_expr_binop(e)->r), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= check_binary_operator_types(to_expr_binop(e), orig, ALLOW_BOOL, user_type_table);
            if (result) {
                e->type = rdup_type(to_expr_binop(e)->l->type);
            }
            break;
        case TAGexpr_lt:
        case TAGexpr_gt:
        case TAGexpr_le:
        case TAGexpr_ge:
        case TAGexpr_eq:
        case TAGexpr_ne:
            result &= infer_types_expr(to_expr(to_expr_binop(e)->l),
                                       user_type_table,
                                       quantifier_table,
                                       orig,
                                       formals,
                                       locals,
                                       references,
                                       attr,
                                       where,
                                       function);
            result &= infer_types_expr(to_expr(to_expr_binop(e)->r),
                                       user_type_table,
                                       quantifier_table,
                                       orig,
                                       formals,
                                       locals,
                                       references,
                                       attr,
                                       where,
                                       function);
/* Any type is allowed. */
            result &= check_binary_operator_types(to_expr_binop(e), orig, ALLOW_BOOL | ALLOW_INT | ALLOW_FLOAT | ALLOW_USER, user_type_table);
            result &= check_binary_operator_extents(to_expr_binop(e), orig, function, where, quantifier_table, user_type_table);
            if (result) {
                e->type = to_type(new_bool_type());
            }
            break;
        case TAGexpr_sub:
        case TAGexpr_div:
        case TAGexpr_mod:
            result &= infer_types_expr(to_expr(to_expr_binop(e)->l), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= infer_types_expr(to_expr(to_expr_binop(e)->r), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= check_binary_operator_types(to_expr_binop(e), orig, ALLOW_INT | ALLOW_FLOAT, user_type_table);
            if (result) {
                e->type = rdup_type(to_expr_binop(e)->l->type);
            }
            break;

/* Unary expressions: */
        case TAGexpr_not:
            result &= infer_types_expr(to_expr(to_expr_unop(e)->x), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= check_unary_operator_type(to_expr_unop(e), orig, ALLOW_BOOL, user_type_table);
            if (result) {
                e->type = rdup_type(to_expr_unop(e)->x->type);
            }
            break;
        case TAGexpr_negate:
            result &= infer_types_expr(to_expr(to_expr_unop(e)->x), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= check_unary_operator_type(to_expr_unop(e), orig, ALLOW_INT | ALLOW_FLOAT, user_type_table);
            if (result) {
                e->type = rdup_type(to_expr_unop(e)->x->type);
            }
            break;

/* Literals: */
        case TAGexpr_bool:
            e->type = to_type(new_bool_type());
            break;
        case TAGexpr_int:
            e->type = to_type(new_int_type());
            break;
        case TAGexpr_float:
            e->type = to_type(new_float_type());
            break;
        case TAGexpr_enum:
            assert(0);

/* Others: */
        case TAGexpr_if_else:
            result &= infer_types_expr(to_expr(to_expr_if_else(e)->cond), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= infer_types_expr(to_expr(to_expr_if_else(e)->thenval), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            result &= infer_types_expr(to_expr(to_expr_if_else(e)->elseval), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            if (!result) {
                break;
            }
            if (to_expr_if_else(e)->cond->type->tag != TAGbool_type ||
/* To Do: indices here shouldn't be always NULL. */
                expr_size(to_expr_if_else(e)->cond, index_entry_listNIL, user_type_table) != 1) {
                leh_error(ERR_TYPE_ARITH_IF_COND, function, orig, where->name);
                result = 0;
            }
            if (!compatible_types(to_expr_if_else(e)->thenval->type, to_expr_if_else(e)->elseval->type, user_type_table)) {
                leh_error(ERR_TYPE_ARITH_IF_VALS, function, orig, where->name);
                result = 0;
            }
            if (result) {
                e->type = rdup_type(to_expr_if_else(e)->thenval->type);
            }
            break;
        case TAGexpr_cond:
/* To Do: We do not signal error when the types in the RHS are different. Look at this. */
            result &= infer_types_expr(to_expr(to_expr_cond(e)->lhs), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            if (exprNIL != to_expr_cond(e)->deflt) {
                result &= infer_types_expr(to_expr(to_expr_cond(e)->deflt), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            } else if (!attr) {
                leh_error(ERR_MISSING_DEFAULT, function, orig, where->name);
                result = 0;
            }
            for (ix = 0; ix < to_expr_cond(e)->choices->sz; ix++) {
                result &= infer_types_expr(to_expr(to_expr_cond(e)->choices->arr[ix]->cond), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
                result &= infer_types_expr(to_expr(to_expr_cond(e)->choices->arr[ix]->val), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            }
            if (!result) {
                break;
            }
            for (ix = 0; ix < to_expr_cond(e)->choices->sz; ix++) {
                if (typeNIL != to_expr_cond(e)->choices->arr[ix]->cond->type &&
                    (!compatible_types(to_expr_cond(e)->choices->arr[ix]->cond->type, to_expr_cond(e)->lhs->type, user_type_table) ||
/* To Do: indices here shouldn't be always NULL. */
                     expr_size(to_expr_cond(e)->choices->arr[ix]->cond, index_entry_listNIL, user_type_table) != expr_size(to_expr_cond(e)->lhs, index_entry_listNIL, user_type_table))) {
                    leh_error(ERR_TYPE_COND, function, orig, where->name);
                    result = 0;
                }
                if (exprNIL != to_expr_cond(e)->choices->arr[0]->val && typeNIL != to_expr_cond(e)->choices->arr[0]->val->type && !compatible_types(to_expr_cond(e)->choices->arr[ix]->val->type, to_expr_cond(e)->choices->arr[0]->val->type, user_type_table)) {
                    leh_error(ERR_TYPE_COND, function, orig, where->name);
                    result = 0;
                }
            }
            if (result) {
                e->type = rdup_type(to_expr_cond(e)->choices->arr[0]->val->type);
            }
            break;
        case TAGexpr_apply:
            result = infer_types_expr_apply(to_expr_apply(e),
                                            orig,
                                            user_type_table,
                                            quantifier_table,
                                            formals,
                                            locals,
                                            references,
                                            where,
                                            attr,
                                            function);
            break;
        case TAGexpr_cast:
            result &= infer_types_expr(to_expr(to_expr_cast(e)->v), user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            if (!result) {
                break;
            }
            src = resolve_type(to_expr_cast(e)->v->type, user_type_table);
            dst = resolve_type(to_expr_cast(e)->dest, user_type_table);
            if (src == typeNIL) {
                break;
            }
            if (dst == typeNIL) {
                assert(to_expr_cast(e)->dest->tag == TAGuser_type);
                if (!member_lydia_symbol_list(undefined, to_user_type(to_expr_cast(e)->dest)->name->sym)) {
                    leh_error(ERR_UNDEFINED_IDENTIFIER,
                              function,
                              to_user_type(to_expr_cast(e)->dest)->name->org,
                              where->name,
                              to_user_type(to_expr_cast(e)->dest)->name->sym->name);
                    undefined = append_lydia_symbol_list(undefined, to_user_type(to_expr_cast(e)->dest)->name->sym);
                }
                result = 0;
                break;
            }
            if (TAGuser_type == src->tag || TAGuser_type == dst->tag) {
                leh_error(ERR_TYPE_CAST, function, orig, where->name);
                result = 0;
                break;
            }
            e->type = rdup_type(to_expr_cast(e)->dest);
            break;
        case TAGexpr_concatenation:
            if (0 == to_expr_concatenation(e)->l->sz) {
                leh_error(ERR_EMPTY_SEQUENCE, function, orig, where->name);
                result = 0;
                break;
            }
            for (ix = 0; ix < to_expr_concatenation(e)->l->sz; ix++) {
                result &= infer_types_expr(to_expr_concatenation(e)->l->arr[ix], user_type_table, quantifier_table, orig, formals, locals, references, attr, where, function);
            }
            if (!result) {
                break;
            }
            for (ix = 1; ix < to_expr_concatenation(e)->l->sz; ix++) {
                if (!compatible_types(to_expr_concatenation(e)->l->arr[ix]->type,
                                      to_expr_concatenation(e)->l->arr[ix - 1]->type,
                                      user_type_table)) {
                    leh_error(ERR_TYPE_SEQUENCE, function, orig, where->name);
                    result = 0;
                    break;
                }
            }
            e->type = rdup_type(to_expr_concatenation(e)->l->arr[0]->type);
            break;
    }
    return result;
}

static int infer_types_predicate(predicate p,
                                 const_user_type_entry_list user_type_table,
                                 const_attribute_entry_list attribute_table,
                                 quantifier_entry_list quantifier_table,
                                 formal_list formals,
                                 local_list locals,
                                 reference_list references,
                                 lydia_symbol where,
                                 int function)
{
    int result = 1;
    unsigned int iw, ix, iy, iz, pos;

    switch (p->tag) {
        case TAGsimple_predicate:
            result &= infer_types_expr(to_simple_predicate(p)->x,
                                       user_type_table,
                                       quantifier_table,
                                       p->org,
                                       formals,
                                       locals,
                                       references,
                                       0,
                                       where,
                                       function);
            if (to_simple_predicate(p)->x->type == typeNIL) {
                break;
            }
            if (to_simple_predicate(p)->x->type->tag == TAGuser_type &&
                search_user_type_entry_list(user_type_table, to_user_type(to_simple_predicate(p)->x->type)->name->sym, &pos)) {
                if (TAGsystem_user_type_entry == user_type_table->arr[pos]->tag) {
                    break;
                }
                if (TAGfunction_user_type_entry == user_type_table->arr[pos]->tag &&
                    TAGbool_type == to_function_user_type_entry(user_type_table->arr[pos])->type->tag) {
                    break;
                }
            }
            if (to_simple_predicate(p)->x->type->tag != TAGbool_type) {
                leh_error(ERR_TYPE_PREDICATE,
                          function,
                          p->org,
                          where->name);
                result = 0;
            }
            break;
        case TAGforall_predicate:
            result &= infer_types_expr(to_forall_predicate(p)->ranges->from, user_type_table, quantifier_table, p->org, formals, locals, references, 0, where, function);
            result &= infer_types_expr(to_forall_predicate(p)->ranges->to, user_type_table, quantifier_table, p->org, formals, locals, references, 0, where, function);
/*
 * To Do: Check if the forall identifier is not a local or formal
 * variable name.
 */
            iw = quantifier_table->sz;
            append_quantifier_entry_list(quantifier_table,
                                         new_quantifier_entry(to_forall_predicate(p)->id->sym,
                                                              rdup_expr(to_forall_predicate(p)->ranges->from),
                                                              rdup_expr(to_forall_predicate(p)->ranges->to)));
            result &= infer_types_predicate(to_predicate(to_forall_predicate(p)->body), user_type_table, attribute_table, quantifier_table, formals, locals, references, where, function);
            delete_quantifier_entry_list(quantifier_table, iw);
            break;
        case TAGexists_predicate:
            result &= infer_types_expr(to_exists_predicate(p)->ranges->from, user_type_table, quantifier_table, p->org, formals, locals, references, 0, where, function);
            result &= infer_types_expr(to_exists_predicate(p)->ranges->to, user_type_table, quantifier_table, p->org, formals, locals, references, 0, where, function);
/*
 * To Do: Check if the exists identifier is not a local or formal
 * variable name.
 */
            iw = quantifier_table->sz;
            append_quantifier_entry_list(quantifier_table,
                                         new_quantifier_entry(to_exists_predicate(p)->id->sym,
                                                              rdup_expr(to_exists_predicate(p)->ranges->from),
                                                              rdup_expr(to_exists_predicate(p)->ranges->to)));
            result &= infer_types_predicate(to_predicate(to_exists_predicate(p)->body), user_type_table, attribute_table, quantifier_table, formals, locals, references, where, function);
            delete_quantifier_entry_list(quantifier_table, iw);
            break;
        case TAGcompound_predicate:
            for (ix = 0; ix < to_compound_predicate(p)->predicates->sz; ix++) {
                result &= infer_types_predicate(to_compound_predicate(p)->predicates->arr[ix],
                                                user_type_table,
                                                attribute_table,
                                                quantifier_table,
                                                formals,
                                                locals,
                                                references,
                                                where,
                                                function);
            }
            break;
        case TAGif_predicate:
            result &= infer_types_expr(to_if_predicate(p)->cond,
                                       user_type_table,
                                       quantifier_table,
                                       p->org,
                                       formals,
                                       locals,
                                       references,
                                       0,
                                       where,
                                       function);
/*
 * If there was already an error in determining the type of the
 * condition, suppress the error.
 */
            if (typeNIL != to_if_predicate(p)->cond->type &&
                to_if_predicate(p)->cond->type->tag != TAGbool_type) {
                leh_error(ERR_TYPE_IF_COND,
                          function,
                          p->org,
                          where->name);
                result = 0;
            }
            if (to_if_predicate(p)->thenval != compound_predicateNIL) {
                result &= infer_types_predicate(to_predicate(to_if_predicate(p)->thenval), user_type_table, attribute_table, quantifier_table, formals, locals, references, where, function);
            }
            if (to_if_predicate(p)->elseval != compound_predicateNIL) {
                result &= infer_types_predicate(to_predicate(to_if_predicate(p)->elseval), user_type_table, attribute_table, quantifier_table, formals, locals, references, where, function);
            }
            break;
        case TAGswitch_predicate:
            result &= infer_types_expr(to_switch_predicate(p)->lhs, user_type_table, quantifier_table, p->org, formals, locals, references, 0, where, function);
            for (ix = 0; ix < to_switch_predicate(p)->choices->sz; ix++) {
                result &= infer_types_expr(to_switch_predicate(p)->choices->arr[ix]->rhs, user_type_table, quantifier_table, p->org, formals, locals, references, 0, where, function);
                result &= infer_types_predicate(to_predicate(to_switch_predicate(p)->choices->arr[ix]->predicate), user_type_table, attribute_table, quantifier_table, formals, locals, references, where, function);
            }
            if (to_switch_predicate(p)->deflt != compound_predicateNIL) {
                result &= infer_types_predicate(to_predicate(to_switch_predicate(p)->deflt), user_type_table, attribute_table, quantifier_table, formals, locals, references, where, function);
            }
            break;
        case TAGsystem_declaration:
            for (ix = 0; ix < to_system_declaration(p)->instances->sz; ix++) {
                system_instantiation inst = to_system_declaration(p)->instances->arr[ix];
                if (to_system_declaration(p)->name->sym == where) {
                    leh_error(ERR_SELF_REFERENCE,
                              function,
                              inst->name->org,
                              where->name,
                              inst->name->sym->name);
                    result = 0;
                }
                if (inst->arguments != expr_listNIL) {
                    for (iy = 0; iy < inst->arguments->sz; iy++) {
                        result &= infer_types_expr(inst->arguments->arr[iy],
                                                   user_type_table,
                                                   quantifier_table,
                                                   p->org,
                                                   formals,
                                                   locals,
                                                   references,
                                                   0,
                                                   where,
                                                   function);
                    }
                }
                if (inst->arguments == expr_listNIL) {
                    continue;
                }
                if (search_user_type_entry_list_with_tag(user_type_table, to_system_declaration(p)->name->sym, TAGsystem_user_type_entry, &pos)) {
                    system_user_type_entry sys = to_system_user_type_entry(user_type_table->arr[pos]);
                    if (!check_arguments(inst->name,
                                         inst->arguments,
                                         sys->formals,
                                         0,
                                         where,
                                         function,
                                         user_type_table,
                                         quantifier_table)) {
                        result = 0;
                    }
                }
            }
            break;
        case TAGvariable_declaration:
            for (ix = 0; ix < to_variable_declaration(p)->instances->sz; ix++) {
                if (exprNIL != to_variable_declaration(p)->instances->arr[ix]->val) {
                    result &= infer_types_expr(to_variable_declaration(p)->instances->arr[ix]->val, user_type_table, quantifier_table, p->org, formals, locals, references, 0, where, function);
                    if (!compatible_types(to_variable_declaration(p)->instances->arr[ix]->val->type, to_variable_declaration(p)->type, user_type_table)) {
                        leh_error(ERR_INCOMPATIBLE_INITIALIZER_TYPE,
                                  function,
                                  to_variable_declaration(p)->instances->arr[ix]->name->org,
                                  where->name,
                                  to_variable_declaration(p)->instances->arr[ix]->name->sym->name);
                        result = 0;
                    }
                }
            }
            break;
        case TAGattribute_declaration:
            for (ix = 0; ix < to_attribute_declaration(p)->instances->sz; ix++) {
                for (iz = 0; iz < to_attribute_declaration(p)->instances->arr[ix]->variables->sz; iz++) {
                    expr_variable var = to_attribute_declaration(p)->instances->arr[ix]->variables->arr[iz];
                    if (!infer_types_expr_variable(var,
                                                   var->name->name->org,
                                                   user_type_table,
                                                   quantifier_table,
                                                   formals,
                                                   locals,
                                                   references,
                                                   where,
                                                   0,
                                                   function)) {
                        result = 0;
                        continue;
                    }
                }
            }
            break;
    }
    return result;
}

int infer_types_formals(formal_list formals,
                        const_user_type_entry_list user_type_table)
{
    register unsigned int ix;

    int result = 1;

    for (ix = 1; ix < formals->sz; ix++) {
        if (typeNIL == formals->arr[ix]->type) {
            formals->arr[ix]->type = rdup_type(formals->arr[ix - 1]->type);
            if (int_listNIL != formals->arr[ix - 1]->ref_count) {
                formals->arr[ix]->ref_count = init_reference_count(formals->arr[ix]->name->ranges,
                                                                   formals->arr[ix]->type,
                                                                   user_type_table);
            }
        }
    }

    return result;
}

int infer_types_model(model m,
                      const_user_type_entry_list user_type_table,
                      const_attribute_entry_list attribute_table)
{
    int result = 1;

    unsigned int ix, iy, pos;
    quantifier_entry_list quantifier_table = new_quantifier_entry_list();
    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGconstant_definition) {
            result &= infer_types_expr(to_constant_definition(m->defs->arr[ix])->val, user_type_table, quantifier_table, to_constant_definition(m->defs->arr[ix])->name->org, formal_listNIL, local_listNIL, reference_listNIL, 0, to_constant_definition(m->defs->arr[ix])->name->sym, 0);
        }
    }
    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGfunction_definition) {
            function_definition func = to_function_definition(m->defs->arr[ix]);
            result &= infer_types_formals(func->formals, user_type_table);
        }
        if (m->defs->arr[ix]->tag == TAGsystem_definition) {
            system_definition sys = to_system_definition(m->defs->arr[ix]);
            result &= infer_types_formals(sys->formals, user_type_table);
        }
    }
    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGfunction_definition) {
            function_definition func;

            undefined = new_lydia_symbol_list();

            func = to_function_definition(m->defs->arr[ix]);
            result &= infer_types_expr(func->val, user_type_table, quantifier_table, func->name->org, func->formals, local_listNIL, reference_listNIL, 0, func->name->sym, 1);

            rfre_lydia_symbol_list(undefined);

            if (func->val->type != typeNIL && func->type != typeNIL && !compatible_types(func->val->type, func->type, user_type_table)) {
                leh_error(ERR_INCOMPATIBLE_RETURN_TYPE,
                          LEH_LOCATION_FUNCTION,
                          func->name->org,
                          func->name->sym->name);
                result = 0;
            }
            for (iy = 0; iy < func->formals->sz; iy++) {
                warn_unused(func->formals->arr[iy]->name,
                            func->formals->arr[iy]->type,
                            func->formals->arr[iy]->ref_count,
                            LEH_LOCATION_FUNCTION,
                            func->name->sym,
                            user_type_table);
            }
        }
    }
    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGsystem_definition) {
            system_definition sys = to_system_definition(m->defs->arr[ix]);
            undefined = new_lydia_symbol_list();
            result &= infer_types_predicate(to_predicate(sys->predicates),
                                            user_type_table,
                                            attribute_table,
                                            quantifier_table,
                                            sys->formals,
                                            sys->locals,
                                            sys->references,
                                            sys->name->sym,
                                            0);
            for (iy = 0; iy < sys->attributes->sz; iy++) {
                result &= infer_types_expr(to_expr(sys->attributes->arr[iy]->var),
                                           user_type_table,
                                           quantifier_table,
                                           sys->attributes->arr[iy]->var->name->name->org,
                                           sys->formals,
                                           sys->locals,
                                           sys->references,
                                           1,
                                           sys->name->sym,
                                           0);
                if (orig_symbolNIL != sys->attributes->arr[iy]->alias) {
/* Append the alias to the local formal or local list. */
                    unsigned int q;
                    if (search_formal_list(sys->formals, sys->attributes->arr[iy]->var->name, &q)) {
                        append_formal_list(sys->formals, rdup_formal(sys->formals->arr[q]));
                        rfre_extent_list(sys->formals->arr[sys->formals->sz - 1]->name->ranges);
                        sys->formals->arr[sys->formals->sz - 1]->name->name->sym = sys->attributes->arr[iy]->alias->sym;
                        sys->formals->arr[sys->formals->sz - 1]->name->ranges = extent_listNIL;
/* To Do: Check the ranges. */
                    }
                    if (search_local_list(sys->locals, sys->attributes->arr[iy]->var->name, &q)) {
                        append_local_list(sys->locals, rdup_local(sys->locals->arr[q]));
                        rfre_extent_list(sys->locals->arr[sys->locals->sz - 1]->name->ranges);
                        sys->locals->arr[sys->locals->sz - 1]->name->name->sym = sys->attributes->arr[iy]->alias->sym;
                        sys->locals->arr[sys->locals->sz - 1]->name->ranges = extent_listNIL;
/* To Do: Check the ranges. */
                    }
                }
                result &= infer_types_expr(sys->attributes->arr[iy]->value,
                                           user_type_table,
                                           quantifier_table,
                                           sys->attributes->arr[iy]->var->name->name->org,
                                           sys->formals,
                                           sys->locals,
                                           sys->references,
                                           1,
                                           sys->name->sym,
                                           0);
                if (orig_symbolNIL != sys->attributes->arr[iy]->alias) {
/* Remove the alias from the local fromal or local list. */
                    unsigned int q;
                    variable_identifier alias = new_variable_identifier(rdup_orig_symbol(sys->attributes->arr[iy]->alias), extent_listNIL, variable_qualifier_listNIL);
                    if (search_formal_list(sys->formals, alias, &q)) {
                        delete_formal_list(sys->formals, q);
                    }
                    if (search_local_list(sys->locals, alias, &q)) {
                        delete_local_list(sys->locals, q);
                    }
                    rfre_variable_identifier(alias);
                }
                assert(TAGuser_type == sys->attributes->arr[iy]->type->tag);
/* Biuilt-in attributes: */
                if (to_user_type(sys->attributes->arr[iy]->type)->name->sym == add_lydia_symbol("health") ||
                    to_user_type(sys->attributes->arr[iy]->type)->name->sym == add_lydia_symbol("observable")) {
                    type t = resolve_type(sys->attributes->arr[iy]->value->type, user_type_table);
                    if (t != typeNIL && t->tag != TAGbool_type) {
                        leh_error(ERR_TYPE_ATTR,
                                  LEH_LOCATION_SYSTEM,
                                  sys->attributes->arr[iy]->var->name->name->org,
                                  m->defs->arr[ix]->name->sym->name,
                                  "bool",
                                  get_type_name(sys->attributes->arr[iy]->value->type));
                        result = 0;
                    }
                } else if (to_user_type(sys->attributes->arr[iy]->type)->name->sym == add_lydia_symbol("probability")) {
                    type t = resolve_type(sys->attributes->arr[iy]->value->type, user_type_table);
                    if (t != typeNIL && t->tag != TAGfloat_type) {
                        leh_error(ERR_TYPE_ATTR,
                                  LEH_LOCATION_SYSTEM,
                                  sys->attributes->arr[iy]->var->name->name->org,
                                  m->defs->arr[ix]->name->sym->name,
                                  "float",
                                  get_type_name(sys->attributes->arr[iy]->value->type));
                        result = 0;
                    }
/* User attributes: */
                } else if (search_attribute_entry_list(attribute_table, to_user_type(sys->attributes->arr[iy]->type)->name->sym, &pos) &&
                    typeNIL != sys->attributes->arr[iy]->value->type &&
                    typeNIL != attribute_table->arr[pos]->type) {
                    if (!compatible_types(sys->attributes->arr[iy]->value->type,
                                          attribute_table->arr[pos]->type,
                                          user_type_table)) {
                        leh_error(ERR_TYPE_ATTR,
                                  LEH_LOCATION_SYSTEM,
                                  sys->attributes->arr[iy]->var->name->name->org,
                                  m->defs->arr[ix]->name->sym->name,
                                  get_type_name(attribute_table->arr[pos]->type),
                                  get_type_name(sys->attributes->arr[iy]->value->type));
                        result = 0;
                    }
                }
            }
            rfre_lydia_symbol_list(undefined);
            for (iy = 0; iy < sys->formals->sz; iy++) {
                warn_unused(sys->formals->arr[iy]->name,
                            sys->formals->arr[iy]->type,
                            sys->formals->arr[iy]->ref_count,
                            LEH_LOCATION_SYSTEM,
                            sys->name->sym,
                            user_type_table);
            }

            for (iy = 0; iy < sys->locals->sz; iy++) {
                warn_unused(sys->locals->arr[iy]->name,
                            sys->locals->arr[iy]->type,
                            sys->locals->arr[iy]->ref_count,
                            LEH_LOCATION_SYSTEM,
                            sys->name->sym,
                            user_type_table);
            }
            for (iy = 0; iy < sys->references->sz; iy++) {
                warn_unused_reference(sys->references->arr[iy]->name,
                                      sys->references->arr[iy]->ranges,
                                      sys->references->arr[iy]->ref_count,
                                      sys->name->sym,
                                      user_type_table);
            }
        }
    }
    rfre_quantifier_entry_list(quantifier_table);

    return result;
}

static int eval_int_declaration_size(expr e, const_user_type_entry_list user_type_table)
{
    unsigned int pos;

    assert(exprNIL != e);

    switch (e->tag) {
        case TAGexpr_variable:
            if (!search_user_type_entry_list_with_tag(user_type_table, to_expr_variable(e)->name->name->sym, TAGconstant_user_type_entry, &pos)) {
                leh_error(ERR_UNDEFINED_IDENTIFIER,
                          0, /* fg_function, */
                          to_expr_variable(e)->name->name->org,
                          "???", /* location->name, */
                          to_expr_variable(e)->name->name->sym->name);
                return -1;
            }
            return eval_int_declaration_size(to_constant_user_type_entry(user_type_table->arr[pos])->value, user_type_table);
/* Literals: */
        case TAGexpr_int:
            return to_expr_int(e)->v;
        case TAGexpr_bool:
        case TAGexpr_float:
        case TAGexpr_enum:
/* Noop. */
            break;

/* Binary expressions: */
        case TAGexpr_add:
            return eval_int_declaration_size(to_expr(to_expr_binop(e)->l), user_type_table) +
                   eval_int_declaration_size(to_expr(to_expr_binop(e)->r), user_type_table);
        case TAGexpr_mult:
            return eval_int_declaration_size(to_expr(to_expr_binop(e)->l), user_type_table) *
                   eval_int_declaration_size(to_expr(to_expr_binop(e)->r), user_type_table);
        case TAGexpr_and:
        case TAGexpr_or:
        case TAGexpr_imply:
        case TAGexpr_ne:
            break;
        case TAGexpr_sub:
            return eval_int_declaration_size(to_expr(to_expr_binop(e)->l), user_type_table) -
                   eval_int_declaration_size(to_expr(to_expr_binop(e)->r), user_type_table);
        case TAGexpr_div:
            return eval_int_declaration_size(to_expr(to_expr_binop(e)->l), user_type_table) /
                   eval_int_declaration_size(to_expr(to_expr_binop(e)->r), user_type_table);
        case TAGexpr_mod:
            return eval_int_declaration_size(to_expr(to_expr_binop(e)->l), user_type_table) %
                   eval_int_declaration_size(to_expr(to_expr_binop(e)->r), user_type_table);
        default:
            assert(0);
            abort();
    };

    assert(0);
    abort();
}

static int compute_array_size(extent_list ranges, const_user_type_entry_list user_type_table)
{
    int result = 1, from, to;

    register unsigned int iy;

    if (extent_listNIL != ranges) {
        for (iy = 0; iy < ranges->sz; iy++) {
            if (exprNIL == ranges->arr[iy]->from) {
                int to = eval_int_declaration_size(ranges->arr[iy]->to, user_type_table);
                if (to == -1) {
                    result = 0;
                }
                rfre_expr(ranges->arr[iy]->to);
                ranges->arr[iy]->from = to_expr(new_expr_int(to_type(new_int_type()), 0));
                ranges->arr[iy]->to = to_expr(new_expr_int(to_type(new_int_type()), to - 1));
                continue;
            }
            from = eval_int_declaration_size(ranges->arr[iy]->from, user_type_table);
            to = eval_int_declaration_size(ranges->arr[iy]->to, user_type_table);
            if (from == -1 || to == -1) {
                result = 0;
            }
            rfre_expr(ranges->arr[iy]->from);
            rfre_expr(ranges->arr[iy]->to);
            ranges->arr[iy]->from = to_expr(new_expr_int(to_type(new_int_type()), from));
            ranges->arr[iy]->to = to_expr(new_expr_int(to_type(new_int_type()), to));
        }
    }

    return result;
}

static int compute_array_sizes_types_predicate(predicate p, const_user_type_entry_list user_type_table)
{
    int result = 1;
    unsigned int ix;

    switch (p->tag) {
        case TAGsimple_predicate:
            break;
        case TAGforall_predicate:
            result &= compute_array_sizes_types_predicate(to_predicate(to_forall_predicate(p)->body), user_type_table);
            break;
        case TAGexists_predicate:
            result &= compute_array_sizes_types_predicate(to_predicate(to_exists_predicate(p)->body), user_type_table);
            break;
        case TAGcompound_predicate:
            for (ix = 0; ix < to_compound_predicate(p)->predicates->sz; ix++) {
                result &= compute_array_sizes_types_predicate(to_compound_predicate(p)->predicates->arr[ix], user_type_table);
            }
            break;
        case TAGif_predicate:
            if (to_if_predicate(p)->thenval != compound_predicateNIL) {
                result &= compute_array_sizes_types_predicate(to_predicate(to_if_predicate(p)->thenval), user_type_table);
            }
            if (to_if_predicate(p)->elseval != compound_predicateNIL) {
                result &= compute_array_sizes_types_predicate(to_predicate(to_if_predicate(p)->elseval), user_type_table);
            }
            break;
        case TAGswitch_predicate:
            for (ix = 0; ix < to_switch_predicate(p)->choices->sz; ix++) {
                result &= compute_array_sizes_types_predicate(to_predicate(to_switch_predicate(p)->choices->arr[ix]->predicate), user_type_table);
            }
            if (to_switch_predicate(p)->deflt != compound_predicateNIL) {
                result &= compute_array_sizes_types_predicate(to_predicate(to_switch_predicate(p)->deflt), user_type_table);
            }
            break;
        case TAGsystem_declaration:
            for (ix = 0; ix < to_system_declaration(p)->instances->sz; ix++) {
                system_instantiation inst = to_system_declaration(p)->instances->arr[ix];
                result &= compute_array_size(inst->ranges, user_type_table);
            }
            break;
        case TAGvariable_declaration:
            for (ix = 0; ix < to_variable_declaration(p)->instances->sz; ix++) {
                variable_instantiation inst = to_variable_declaration(p)->instances->arr[ix];
                result &= compute_array_size(inst->ranges, user_type_table);
            }
            break;
        case TAGattribute_declaration:
/* Noop. */
            break;
    }
    return result;
}

int compute_array_sizes_formals(formal_list formals, const_user_type_entry_list user_type_table)
{
    register unsigned int ix, iy;

    int result = 1;

    for (ix = 0; ix < formals->sz; ix++) {
        if (formals->arr[ix]->name->ranges != extent_listNIL) {
            for (iy = 0; iy < formals->arr[ix]->name->ranges->sz; iy++) {
                result &= compute_array_size(formals->arr[ix]->name->ranges, user_type_table);
            }
        }
    }

    return result;
}

int compute_array_sizes(model m, user_type_entry_list user_type_table)
{
    register unsigned int ix, iy;

    int result = 1;

    for (ix = 0; ix < user_type_table->sz; ix++) {
        if (TAGstruct_user_type_entry == user_type_table->arr[ix]->tag) {
            for (iy = 0; iy < to_struct_user_type_entry(user_type_table->arr[ix])->ranges->sz; iy++) {
                result &= compute_array_size(to_struct_user_type_entry(user_type_table->arr[ix])->ranges->arr[iy], user_type_table);
            }
        }
    }
    for (ix = 0; ix < m->defs->sz; ix++) {
        if (m->defs->arr[ix]->tag == TAGfunction_definition) {
            function_definition func = to_function_definition(m->defs->arr[ix]);
            result &= compute_array_sizes_formals(func->formals, user_type_table);
        }
        if (m->defs->arr[ix]->tag == TAGsystem_definition) {
            system_definition sys = to_system_definition(m->defs->arr[ix]);
            result &= compute_array_sizes_formals(sys->formals, user_type_table);
            result &= compute_array_sizes_types_predicate(to_predicate(sys->predicates), user_type_table);
        }
    }

    return result;
}
