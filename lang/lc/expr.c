#include "config.h"

#include <assert.h>
#include <math.h>

#include "lc.h"
#include "search.h"

int eval_int_expr(expr e,
                  const_index_entry_list index_table,
                  const_user_type_entry_list user_type_table)
{
    unsigned int pos;
    unsigned int var_parts = 1;
    expr_variable var = expr_variableNIL;

    assert(exprNIL != e);

    switch (e->tag) {
        case TAGexpr_variable:
            assert(to_expr_variable(e)->type != typeNIL);
            var = to_expr_variable(e);
            if (var->name->qualifiers != variable_qualifier_listNIL) {
                var_parts += var->name->qualifiers->sz;
            }
            if (user_type_table != user_type_entry_listNIL) {
                if (var_parts == 1 && search_user_type_entry_list_with_tag(user_type_table, var->name->name->sym, TAGconstant_user_type_entry, &pos)) {
                    constant_user_type_entry constant = to_constant_user_type_entry(user_type_table->arr[pos]);
                    assert(TAGint_type == constant->type->tag);
                    return eval_int_expr(constant->value, index_table, user_type_table);
                }
            }
            assert(index_table != index_entry_listNIL);
            assert(var->name->ranges == extent_listNIL);
            assert(var->name->qualifiers == variable_qualifier_listNIL);
            if (!search_index_entry_list(index_table, var->name->name->sym, &pos)) {
                assert(0);
                break;
            }
            return index_table->arr[pos]->value;

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
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) +
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_mult:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) *
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_and:
        case TAGexpr_or:
        case TAGexpr_imply:
        case TAGexpr_lt:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) <
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_gt:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) >
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_le:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) <=
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_ge:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) >=
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_ne:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) !=
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_sub:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) -
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_div:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) /
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_mod:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) %
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);
        case TAGexpr_eq:
            return eval_int_expr(to_expr(to_expr_binop(e)->l), index_table, user_type_table) ==
                   eval_int_expr(to_expr(to_expr_binop(e)->r), index_table, user_type_table);

/* Unary expressions: */
        case TAGexpr_not:
        case TAGexpr_negate:
            break;

/* Others: */
        case TAGexpr_if_else:
            break;
        case TAGexpr_cond:
            break;
        case TAGexpr_apply:
            break;
        case TAGexpr_cast:
            break;
        case TAGexpr_concatenation:
            break;
    };

    assert(0);
    abort();
}
