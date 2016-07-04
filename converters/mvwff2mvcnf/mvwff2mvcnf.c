/* filename:     mvwff2mvcnf.c
 * description:  functions for converting from mv_wff
 * author:       Tom Janssen (TU Delft)
 */

#include "mvwff2mvcnf.h"
#include <assert.h>
#include <math.h>

mv_wff_expr eliminate_equiv(mv_wff_expr e)
{
    /* A <-> B  =>  (A -> B) ^ (B -> A) 
     */

    mv_wff_expr a, b;
    mv_wff_e_impl impl_a, impl_b;
    
    mv_wff_e_equiv e_eq;
    mv_wff_e_impl e_impl;
    mv_wff_e_not e_not;
    mv_wff_e_and e_and;
    mv_wff_e_or e_or;

    switch(e->tag) {
        case TAGmv_wff_e_equiv:
            e_eq = to_mv_wff_e_equiv(e);
            a = eliminate_equiv(e_eq->lhs);
            b = eliminate_equiv(e_eq->rhs);
            fre_mv_wff_expr(e);
            impl_a = new_mv_wff_e_impl(a, 
                                       b);
            impl_b = new_mv_wff_e_impl(rdup_mv_wff_expr(b), 
                                       rdup_mv_wff_expr(a));
            e_and = new_mv_wff_e_and(to_mv_wff_expr(impl_a),
                                     to_mv_wff_expr(impl_b));
            return to_mv_wff_expr(e_and);
        case TAGmv_wff_e_impl:
            e_impl = to_mv_wff_e_impl(e);
            e_impl->lhs = eliminate_equiv(e_impl->lhs);
            e_impl->rhs = eliminate_equiv(e_impl->rhs);
            break;
        case TAGmv_wff_e_not:
            e_not = to_mv_wff_e_not(e);
            e_not->n = eliminate_equiv(e_not->n);
            break;
        case TAGmv_wff_e_and:
            e_and = to_mv_wff_e_and(e);
            e_and->lhs = eliminate_equiv(e_and->lhs);
            e_and->rhs = eliminate_equiv(e_and->rhs);
            break;
        case TAGmv_wff_e_or:
            e_or = to_mv_wff_e_or(e);
            e_or->lhs = eliminate_equiv(e_or->lhs);
            e_or->rhs = eliminate_equiv(e_or->rhs);
            break;
        default:
            break;
    }
    
    return e;
}

mv_wff_expr eliminate_impl(mv_wff_expr e)
{
    /* A -> B  =>  ~A V B 
     */

    mv_wff_expr a, b;
    mv_wff_e_not not_a;
    
    mv_wff_e_equiv e_eq;
    mv_wff_e_impl e_impl;
    mv_wff_e_not e_not;
    mv_wff_e_and e_and;
    mv_wff_e_or e_or;

    switch(e->tag) {
        case TAGmv_wff_e_equiv:
            e_eq = to_mv_wff_e_equiv(e);
            e_eq->lhs = eliminate_impl(e_eq->lhs);
            e_eq->rhs = eliminate_impl(e_eq->rhs);
            break;
        case TAGmv_wff_e_impl:
            e_impl = to_mv_wff_e_impl(e);
            a = eliminate_impl(e_impl->lhs);
            b = eliminate_impl(e_impl->rhs);
            fre_mv_wff_expr(e);
            not_a = new_mv_wff_e_not(a);
            e_or = new_mv_wff_e_or(to_mv_wff_expr(not_a),
                                   b);
            return to_mv_wff_expr(e_or);
        case TAGmv_wff_e_not:
            e_not = to_mv_wff_e_not(e);
            e_not->n = eliminate_impl(e_not->n);
            break;
        case TAGmv_wff_e_and:
            e_and = to_mv_wff_e_and(e);
            e_and->lhs = eliminate_impl(e_and->lhs);
            e_and->rhs = eliminate_impl(e_and->rhs);
            break;
        case TAGmv_wff_e_or:
            e_or = to_mv_wff_e_or(e);
            e_or->lhs = eliminate_impl(e_or->lhs);
            e_or->rhs = eliminate_impl(e_or->rhs);
            break;
        default:
            break;
    }
    
    return e;
}

void mvwff2mvcnf_eliminate_implications(mv_wff_expr *e)
{
    *e = eliminate_equiv(*e);
    *e = eliminate_impl(*e);
}

mv_wff_expr reduce_negation_scopes(mv_wff_expr e)
{
    /* ~~A  =>  A
     * ~(A V B)  =>  ~A ^ ~B
     * ~(A ^ B)  =>  ~A V ~B 
     */

    mv_wff_expr a, b;
    mv_wff_e_not not_a, not_b;
    
    mv_wff_e_equiv e_eq;
    mv_wff_e_impl e_impl;
    mv_wff_e_not e_not;
    mv_wff_e_and e_and;
    mv_wff_e_or e_or;

    switch(e->tag) {
        case TAGmv_wff_e_equiv:
            e_eq = to_mv_wff_e_equiv(e);
            e_eq->lhs = reduce_negation_scopes(e_eq->lhs);
            e_eq->rhs = reduce_negation_scopes(e_eq->rhs);
            break;
        case TAGmv_wff_e_impl:
            e_impl = to_mv_wff_e_impl(e);
            e_impl->lhs = reduce_negation_scopes(e_impl->lhs);
            e_impl->rhs = reduce_negation_scopes(e_impl->rhs);
            break;
        case TAGmv_wff_e_not:
            e_not = to_mv_wff_e_not(e);
            switch(e_not->n->tag)
            {
                case TAGmv_wff_e_not: /* ~~A */
                    a = to_mv_wff_e_not(e_not->n)->n;
                    fre_mv_wff_expr(e_not->n);
                    fre_mv_wff_expr(e);
                    return reduce_negation_scopes(a);
                case TAGmv_wff_e_or: /* ~(A V B) */
                    a = to_mv_wff_e_or(e_not->n)->lhs;
                    b = to_mv_wff_e_or(e_not->n)->rhs;
                    not_a = new_mv_wff_e_not(a);
                    not_b = new_mv_wff_e_not(b);
                    e_and = new_mv_wff_e_and(to_mv_wff_expr(not_a),
                                             to_mv_wff_expr(not_b));
                    fre_mv_wff_expr(e_not->n);
                    fre_mv_wff_expr(e);
                    return reduce_negation_scopes(to_mv_wff_expr(e_and));
                case TAGmv_wff_e_and: /* ~(A ^ B) */
                    a = to_mv_wff_e_and(e_not->n)->lhs;
                    b = to_mv_wff_e_and(e_not->n)->rhs;
                    not_a = new_mv_wff_e_not(a);
                    not_b = new_mv_wff_e_not(b);
                    e_or = new_mv_wff_e_or(to_mv_wff_expr(not_a),
                                           to_mv_wff_expr(not_b));
                    fre_mv_wff_expr(e_not->n);
                    fre_mv_wff_expr(e);
                    return reduce_negation_scopes(to_mv_wff_expr(e_or));
                default:
                    e_not->n = reduce_negation_scopes(e_not->n);
                    return e;
            }
            break;
        case TAGmv_wff_e_and:
            e_and = to_mv_wff_e_and(e);
            e_and->lhs = reduce_negation_scopes(e_and->lhs);
            e_and->rhs = reduce_negation_scopes(e_and->rhs);
            break;
        case TAGmv_wff_e_or:
            e_or = to_mv_wff_e_or(e);
            e_or->lhs = reduce_negation_scopes(e_or->lhs);
            e_or->rhs = reduce_negation_scopes(e_or->rhs);
            break;
        default:
            break;
    }
    
    return e;

}

void mvwff2mvcnf_reduce_negation_scopes(mv_wff_expr *e)
{
    *e = reduce_negation_scopes(*e);
}

mv_wff_expr distribute_disjunctions(mv_wff_expr e, int *changes)
{
    /* A V (B ^ C)  =>  (A V B) ^ (A V C)
     * (A ^ B) V C  =>  (A V C) ^ (B V C)
     */

    mv_wff_e_or or_a, or_b;
    
    mv_wff_e_equiv e_eq;
    mv_wff_e_impl e_impl;
    mv_wff_e_not e_not;
    mv_wff_e_and e_and;
    mv_wff_e_or e_or;

    switch(e->tag) {
        case TAGmv_wff_e_equiv:
            e_eq = to_mv_wff_e_equiv(e);
            e_eq->lhs = distribute_disjunctions(e_eq->lhs, changes);
            e_eq->rhs = distribute_disjunctions(e_eq->rhs, changes);
            break;
        case TAGmv_wff_e_impl:
            e_impl = to_mv_wff_e_impl(e);
            e_impl->lhs = distribute_disjunctions(e_impl->lhs, changes);
            e_impl->rhs = distribute_disjunctions(e_impl->rhs, changes);
            break;
        case TAGmv_wff_e_not:
            e_not = to_mv_wff_e_not(e);
            e_not->n = distribute_disjunctions(e_not->n, changes);
            break;
        case TAGmv_wff_e_and:
            e_and = to_mv_wff_e_and(e);
            e_and->lhs = distribute_disjunctions(e_and->lhs, changes);
            e_and->rhs = distribute_disjunctions(e_and->rhs, changes);
            break;
        case TAGmv_wff_e_or:
            e_or = to_mv_wff_e_or(e);
            if(e_or->rhs->tag == TAGmv_wff_e_and) {
                (*changes)++;
                or_a = new_mv_wff_e_or(e_or->lhs,
                                       to_mv_wff_e_and(e_or->rhs)->lhs);
                or_b = new_mv_wff_e_or(rdup_mv_wff_expr(e_or->lhs),
                                       to_mv_wff_e_and(e_or->rhs)->rhs);
                e_and = new_mv_wff_e_and(to_mv_wff_expr(or_a),
                                         to_mv_wff_expr(or_b));
                fre_mv_wff_expr(e_or->rhs);
                fre_mv_wff_expr(e);
                return distribute_disjunctions(to_mv_wff_expr(e_and), changes);
            } else {
                e_or->rhs = distribute_disjunctions(e_or->rhs, changes);
            }
            if(e_or->lhs->tag == TAGmv_wff_e_and) {
                (*changes)++;
                or_a = new_mv_wff_e_or(to_mv_wff_e_and(e_or->lhs)->lhs,
                                       e_or->rhs);
                or_b = new_mv_wff_e_or(to_mv_wff_e_and(e_or->lhs)->rhs,
                                       rdup_mv_wff_expr(e_or->rhs));
                e_and = new_mv_wff_e_and(to_mv_wff_expr(or_a),
                                         to_mv_wff_expr(or_b));
                fre_mv_wff_expr(e_or->lhs);
                fre_mv_wff_expr(e);
                return distribute_disjunctions(to_mv_wff_expr(e_and), changes);
            } else {
                e_or->lhs = distribute_disjunctions(e_or->lhs, changes);
            }
            break;
        default:
            break;
    }
    
    return e;
    
}

void mvwff2mvcnf_distribute_disjunctions(mv_wff_expr *e)
{
    int changes;

    do {
        changes = 0;
        *e = distribute_disjunctions(*e, &changes);
    } while (changes > 0);
}

mv_wff_expr distribute_conjunctions(mv_wff_expr e, int *changes)
{
    /* A ^ (B V C)  =>  (A ^ B) V (A ^ C)
     * (A V B) ^ C  =>  (A ^ C) V (B ^ C)
     */

    mv_wff_e_and and_a, and_b;
    
    mv_wff_e_equiv e_eq;
    mv_wff_e_impl e_impl;
    mv_wff_e_not e_not;
    mv_wff_e_and e_and;
    mv_wff_e_or e_or;

    switch(e->tag) {
        case TAGmv_wff_e_equiv:
            e_eq = to_mv_wff_e_equiv(e);
            e_eq->lhs = distribute_conjunctions(e_eq->lhs, changes);
            e_eq->rhs = distribute_conjunctions(e_eq->rhs, changes);
            break;
        case TAGmv_wff_e_impl:
            e_impl = to_mv_wff_e_impl(e);
            e_impl->lhs = distribute_conjunctions(e_impl->lhs, changes);
            e_impl->rhs = distribute_conjunctions(e_impl->rhs, changes);
            break;
        case TAGmv_wff_e_not:
            e_not = to_mv_wff_e_not(e);
            e_not->n = distribute_conjunctions(e_not->n, changes);
            break;
        case TAGmv_wff_e_and:
            e_and = to_mv_wff_e_and(e);
            if(e_and->rhs->tag == TAGmv_wff_e_or) {
                (*changes)++;
                and_a = new_mv_wff_e_and(e_and->lhs,
                                       to_mv_wff_e_or(e_and->rhs)->lhs);
                and_b = new_mv_wff_e_and(rdup_mv_wff_expr(e_and->lhs),
                                       to_mv_wff_e_or(e_and->rhs)->rhs);
                e_or = new_mv_wff_e_or(to_mv_wff_expr(and_a),
                                         to_mv_wff_expr(and_b));
                fre_mv_wff_expr(e_and->rhs);
                fre_mv_wff_expr(e);
                return distribute_conjunctions(to_mv_wff_expr(e_or), changes);
            } else {
                e_and->rhs = distribute_conjunctions(e_and->rhs, changes);
            }
            if(e_and->lhs->tag == TAGmv_wff_e_or) {
                (*changes)++;
                and_a = new_mv_wff_e_and(to_mv_wff_e_or(e_and->lhs)->lhs,
                                       e_and->rhs);
                and_b = new_mv_wff_e_and(to_mv_wff_e_or(e_and->lhs)->rhs,
                                       rdup_mv_wff_expr(e_and->rhs));
                e_or = new_mv_wff_e_or(to_mv_wff_expr(and_a),
                                         to_mv_wff_expr(and_b));
                fre_mv_wff_expr(e_and->lhs);
                fre_mv_wff_expr(e);
                return distribute_conjunctions(to_mv_wff_expr(e_or), changes);
            } else {
                e_and->lhs = distribute_conjunctions(e_and->lhs, changes);
            }
            break;
        case TAGmv_wff_e_or:
            e_or = to_mv_wff_e_or(e);
            e_or->lhs = distribute_conjunctions(e_or->lhs, changes);
            e_or->rhs = distribute_conjunctions(e_or->rhs, changes);
            break;
        default:
            break;
    }
    
    return e;
    
}

void mvwff2mvcnf_distribute_conjunctions(mv_wff_expr *e)
{
    int changes;

    do {
        changes = 0;
        *e = distribute_conjunctions(*e, &changes);
    } while (changes > 0);
}

mv_wff_expr eliminate_constants(mv_wff_expr e)
{
    mv_wff_expr a;

    mv_wff_e_equiv e_eq;
    mv_wff_e_impl e_impl;
    mv_wff_e_not e_not;
    mv_wff_e_and e_and;
    mv_wff_e_or e_or;

    switch(e->tag)
    {
        case TAGmv_wff_e_equiv:
            e_eq = to_mv_wff_e_equiv(e);
            e_eq->lhs = eliminate_constants(e_eq->lhs);
            e_eq->rhs = eliminate_constants(e_eq->rhs);
            if(e_eq->lhs->tag == TAGmv_wff_e_const && 
                    !to_mv_wff_e_const(e_eq->lhs)->c) {
                /* False <-> A  =>  ~A */
                e_not = new_mv_wff_e_not(e_eq->rhs);
                rfre_mv_wff_expr(e_eq->lhs);
                fre_mv_wff_expr(e);
                return eliminate_constants(to_mv_wff_expr(e_not));
            }
            if(e_eq->rhs->tag == TAGmv_wff_e_const &&
                    !to_mv_wff_e_const(e_eq->rhs)->c) {
                /* A <-> False  =>  ~A */
                e_not = new_mv_wff_e_not(e_eq->lhs);
                rfre_mv_wff_expr(e_eq->rhs);
                fre_mv_wff_expr(e);
                return eliminate_constants(to_mv_wff_expr(e_not));
            }
            if(e_eq->lhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_eq->lhs)->c) {
                /* True <-> A  =>  A */
                a = e_eq->rhs;
                rfre_mv_wff_expr(e_eq->lhs);
                fre_mv_wff_expr(e);
                return a;
            }
            if(e_eq->rhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_eq->rhs)->c) {
                /* A <-> True  =>  A */
                a = e_eq->lhs;
                rfre_mv_wff_expr(e_eq->rhs);
                fre_mv_wff_expr(e);
                return a;
            }
            break;
        case TAGmv_wff_e_impl:
            e_impl = to_mv_wff_e_impl(e);
            e_impl->lhs = eliminate_constants(e_impl->lhs);
            e_impl->rhs = eliminate_constants(e_impl->rhs);
            if(e_impl->lhs->tag == TAGmv_wff_e_const &&
                    !to_mv_wff_e_const(e_impl->lhs)->c) {
                /* False -> A  =>  True*/
                a = to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
                rfre_mv_wff_expr(e);
                return a;
            }
            if(e_impl->rhs->tag == TAGmv_wff_e_const &&
                    !to_mv_wff_e_const(e_impl->rhs)->c) {
                /* A -> False  =>  ~A */
                e_not = new_mv_wff_e_not(e_impl->lhs);
                rfre_mv_wff_expr(e_impl->rhs);
                fre_mv_wff_expr(e);
                return eliminate_constants(to_mv_wff_expr(e_not));
            }
            if(e_impl->lhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_impl->lhs)->c) {
                /* True -> A  =>  A */
                a = e_impl->rhs;
                rfre_mv_wff_expr(e_impl->lhs);
                fre_mv_wff_expr(e);
                return a;
            }
            if(e_impl->rhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_impl->rhs)->c) {
                /* A -> True  =>  True */
                a = to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
                rfre_mv_wff_expr(e);
                return a;
            }
            break;
        case TAGmv_wff_e_not:
            e_not = to_mv_wff_e_not(e);
            e_not->n = eliminate_constants(e_not->n);
            if(e_not->n->tag == TAGmv_wff_e_const) {
                if(to_mv_wff_e_const(e_not->n)->c) {
                    /* ~True  =>  False */
                    rfre_mv_wff_expr(e);
                    return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_FALSE));
                } else if(!to_mv_wff_e_const(e_not->n)->c) {
                    /* ~False  =>  True */
                    rfre_mv_wff_expr(e);
                    return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
                } else {
                    assert(0);
                }
            }
            break;
        case TAGmv_wff_e_or:
            e_or = to_mv_wff_e_or(e);
            e_or->lhs = eliminate_constants(e_or->lhs);
            e_or->rhs = eliminate_constants(e_or->rhs);
            if(e_or->lhs->tag == TAGmv_wff_e_const &&
                    !to_mv_wff_e_const(e_or->lhs)) {
                /* False V A  =>  A */
                a = e_or->rhs;
                rfre_mv_wff_expr(e_or->lhs);
                fre_mv_wff_expr(e);
                return a;
            }
            if(e_or->rhs->tag == TAGmv_wff_e_const &&
                    !to_mv_wff_e_const(e_or->rhs)) {
                /* A V False  =>  A */
                a = e_or->lhs;
                rfre_mv_wff_expr(e_or->rhs);
                fre_mv_wff_expr(e);
                return a;
            }
            if(e_or->lhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_or->lhs)) {
                /* True V A  =>  True */
                rfre_mv_wff_expr(e);
                return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
            }
            if(e_or->rhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_or->rhs)) {
                /* A V True  =>  True */
                rfre_mv_wff_expr(e);
                return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_TRUE));
            }
            break;
        case TAGmv_wff_e_and:
            e_and = to_mv_wff_e_and(e);
            e_and->lhs = eliminate_constants(e_and->lhs);
            e_and->rhs = eliminate_constants(e_and->rhs);
            if(e_and->lhs->tag == TAGmv_wff_e_const &&
                    !to_mv_wff_e_const(e_and->lhs)) {
                /* False ^ A  =>  False */
                rfre_mv_wff_expr(e);
                return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_FALSE));
            }
            if(e_and->rhs->tag == TAGmv_wff_e_const &&
                    !to_mv_wff_e_const(e_and->rhs)) {
                /* A ^ False  =>  False */
                rfre_mv_wff_expr(e);
                return to_mv_wff_expr(new_mv_wff_e_const(LYDIA_FALSE));
            }
            if(e_and->lhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_and->lhs)) {
                /* True ^ A  =>  A */
                a = e_and->rhs;
                rfre_mv_wff_expr(e_and->lhs);
                fre_mv_wff_expr(e);
                return a;
            }
            if(e_and->rhs->tag == TAGmv_wff_e_const &&
                    to_mv_wff_e_const(e_and->rhs)) {
                /* A ^ True  =>  A */
                a = e_and->lhs;
                rfre_mv_wff_expr(e_and->rhs);
                fre_mv_wff_expr(e);
                return a;
            }
            break;
        default:
            break;
    }

    return e;
}

void mvwff2mvcnf_eliminate_constants(mv_wff_expr *e)
{
    *e = eliminate_constants(*e);
}

void create_clause(const_mv_wff_expr e, mv_clause clause)
{
    switch(e->tag)
    {
        case TAGmv_wff_e_equiv:
        case TAGmv_wff_e_impl:
        case TAGmv_wff_e_and:
            /* can only create clause from disjunction */
            assert(0);
            break;
        case TAGmv_wff_e_const:
            /* should have been eliminated from expression */
            assert(0);
            break;
        case TAGmv_wff_e_or:
            create_clause(to_mv_wff_e_or(e)->lhs, clause);
            create_clause(to_mv_wff_e_or(e)->rhs, clause);
            break;
        case TAGmv_wff_e_not:
            /* negated expressions other than vars should have
             * been eliminated or converted */
            assert(to_mv_wff_e_not(e)->n->tag == TAGmv_wff_e_var);
            clause->neg = append_mv_literal_list(clause->neg, 
                    new_mv_literal(to_mv_wff_e_var(to_mv_wff_e_not(e)->n)->var,
                        to_mv_wff_e_var(to_mv_wff_e_not(e)->n)->val));
            break;
        case TAGmv_wff_e_var:
            clause->pos = append_mv_literal_list(clause->pos,
                    new_mv_literal(to_mv_wff_e_var(e)->var,
                        to_mv_wff_e_var(e)->val));
            break;
        default:
            assert(0);
            break;
    }
}

void create_clause_list(const_mv_wff_expr e, mv_clause_list clauses)
{
    mv_clause cl;
    switch(e->tag)
    {
        case TAGmv_wff_e_and:
            create_clause_list(to_mv_wff_e_and(e)->lhs, clauses);
            create_clause_list(to_mv_wff_e_and(e)->rhs, clauses);
            break;
        case TAGmv_wff_e_equiv:
        case TAGmv_wff_e_impl:
        case TAGmv_wff_e_const:
        case TAGmv_wff_e_or:
        case TAGmv_wff_e_not:
        case TAGmv_wff_e_var:
        default:
            cl = new_mv_clause(new_mv_literal_list(), new_mv_literal_list());
            create_clause(e, cl);
            append_mv_clause_list(clauses, cl);
            break;
    }
}

mv_clause_list expr_to_clauses(mv_wff_expr *e)
{
    mv_clause_list result = new_mv_clause_list();

    if(*e == mv_wff_exprNIL) {
        return result;
    }

    mvwff2mvcnf_eliminate_implications(e);
    mvwff2mvcnf_reduce_negation_scopes(e);
    mvwff2mvcnf_distribute_disjunctions(e);
    mvwff2mvcnf_eliminate_constants(e);

    create_clause_list(*e, result);

    return result;
}

mv_cnf flat_mvwff2mvcnf(const_mv_wff input)
{
    register unsigned int ix;
    mv_cnf output;
    output = new_mv_cnf(rdup_values_set_list(input->domains),
                        rdup_variable_list(input->variables),
                        rdup_variable_list(input->encoded_variables),
                        rdup_constant_list(input->constants),
                        input->encoding,
                        new_mv_clause_list());
    for (ix = 0; ix < input->e->sz; ix++) {
        mv_clause_list clauses;
        mv_wff_expr expr = rdup_mv_wff_expr(input->e->arr[ix]);
        clauses = expr_to_clauses(&expr);
        concat_mv_clause_list(output->clauses, clauses);
        rfre_mv_wff_expr(expr);
    }
    return output;
}

serializable mvwff2mvcnf(const_serializable wff)
{
    register unsigned int ix;
    mv_wff_hierarchy input_mvwffhier;
    mv_cnf output_mvcnf;
    mv_cnf_hierarchy output_mvcnfhier;

    switch(wff->tag) {
        case TAGmv_wff_flat_kb:
            output_mvcnf = flat_mvwff2mvcnf(to_mv_wff(to_mv_wff_flat_kb(wff)->constraints));
            return to_serializable(new_mv_cnf_flat_kb(to_mv_wff_flat_kb(wff)->name,
                                                      to_kb(output_mvcnf)));
        case TAGmv_wff_hierarchy:
            input_mvwffhier = to_mv_wff_hierarchy(wff);
            output_mvcnfhier = new_mv_cnf_hierarchy(new_node_list());
            for (ix = 0; ix < input_mvwffhier->nodes->sz; ix++) {
                output_mvcnf = flat_mvwff2mvcnf(to_mv_wff(input_mvwffhier->nodes->arr[ix]->constraints));
                output_mvcnf->clauses = sort_mv_clauses(minimize_clauses(output_mvcnf->variables,
                                                                         output_mvcnf->domains,
                                                                         output_mvcnf->clauses));
                append_node_list(output_mvcnfhier->nodes, 
                                 new_node(input_mvwffhier->nodes->arr[ix]->type,
                                          rdup_edge_list(input_mvwffhier->nodes->arr[ix]->edges),
                                          to_kb(output_mvcnf)));
            }
            return to_serializable(output_mvcnfhier);
        default:
            assert(0);
    }

    return NULL;
}


void create_term(const_mv_wff_expr e, mv_term term)
{
    switch(e->tag)
    {
        case TAGmv_wff_e_equiv:
        case TAGmv_wff_e_impl:
        case TAGmv_wff_e_or:
            /* can only create term from conjunction */
            assert(0);
            break;
        case TAGmv_wff_e_const:
            /* should have been eliminated from expression */
            assert(0);
            break;
        case TAGmv_wff_e_and:
            create_term(to_mv_wff_e_and(e)->lhs, term);
            create_term(to_mv_wff_e_and(e)->rhs, term);
            break;
        case TAGmv_wff_e_not:
            /* negated expressions other than vars should have
             * been eliminated or converted */
            assert(to_mv_wff_e_not(e)->n->tag == TAGmv_wff_e_var);
            term->neg = append_mv_literal_list(term->neg, 
                    new_mv_literal(to_mv_wff_e_var(to_mv_wff_e_not(e)->n)->var,
                        to_mv_wff_e_var(to_mv_wff_e_not(e)->n)->val));
            break;
        case TAGmv_wff_e_var:
            term->pos = append_mv_literal_list(term->pos,
                    new_mv_literal(to_mv_wff_e_var(e)->var,
                        to_mv_wff_e_var(e)->val));
            break;
        default:
            assert(0);
            break;
    }
}

void create_term_list(const_mv_wff_expr e, mv_term_list terms)
{
    mv_term term;
    switch(e->tag)
    {
        case TAGmv_wff_e_or:
            create_term_list(to_mv_wff_e_or(e)->lhs, terms);
            create_term_list(to_mv_wff_e_or(e)->rhs, terms);
            break;
        case TAGmv_wff_e_equiv:
        case TAGmv_wff_e_impl:
        case TAGmv_wff_e_const:
        case TAGmv_wff_e_and:
        case TAGmv_wff_e_not:
        case TAGmv_wff_e_var:
        default:
            term = new_mv_term(new_mv_literal_list(), new_mv_literal_list());
            create_term(e, term);
            append_mv_term_list(terms, term);
            break;
    }
}

mv_term_list expr_to_terms(mv_wff_expr *e)
{
    mv_term_list result = new_mv_term_list();

    if(*e == mv_wff_exprNIL) {
        return result;
    }

    mvwff2mvcnf_eliminate_implications(e);
    mvwff2mvcnf_reduce_negation_scopes(e);
    mvwff2mvcnf_distribute_conjunctions(e);
    mvwff2mvcnf_eliminate_constants(e);

    create_term_list(*e, result);

    return result;
}

mv_dnf flat_mvwff2mvdnf(const_mv_wff input)
{
    register unsigned int ix;
    mv_dnf output;
    output = new_mv_dnf(rdup_values_set_list(input->domains),
            rdup_variable_list(input->variables),
            rdup_variable_list(input->encoded_variables),
            rdup_constant_list(input->constants),
            input->encoding,
            new_mv_term_list());
    for (ix = 0; ix < input->e->sz; ix++) {
        mv_term_list terms;
        mv_wff_expr expr = rdup_mv_wff_expr(input->e->arr[ix]);
        terms = expr_to_terms(&expr);
        concat_mv_term_list(output->terms, terms);
        rfre_mv_wff_expr(expr);
    }
    return output;
}

serializable mvwff2mvdnf(const_serializable wff)
{
    register unsigned int ix;
    mv_wff input_mvwff;
    mv_wff_hierarchy input_mvwffhier;
    mv_dnf output_mvdnf;
    mv_dnf_hierarchy output_mvdnfhier;

    switch(wff->tag) {
        case TAGmv_wff_flat_kb:
            input_mvwff = to_mv_wff(to_mv_wff_flat_kb(wff)->constraints);
            output_mvdnf = flat_mvwff2mvdnf(input_mvwff);
            return to_serializable(new_mv_wff_flat_kb(rdup_lydia_symbol(to_mv_wff_flat_kb(wff)->name),
                                                      to_kb(output_mvdnf)));
        case TAGmv_wff_hierarchy:
            input_mvwffhier = to_mv_wff_hierarchy(wff);
            output_mvdnfhier = new_mv_dnf_hierarchy(new_node_list());
            for (ix = 0; ix < input_mvwffhier->nodes->sz; ix++) {
                input_mvwff = to_mv_wff(input_mvwffhier->nodes->arr[ix]->constraints);
                output_mvdnf = flat_mvwff2mvdnf(input_mvwff);
                output_mvdnf->terms = sort_mv_terms(output_mvdnf->terms);
                append_node_list(output_mvdnfhier->nodes, 
                                 new_node(input_mvwffhier->nodes->arr[ix]->type,
                                          rdup_edge_list(input_mvwffhier->nodes->arr[ix]->edges),
                                          to_kb(output_mvdnf)));
            }
            return to_serializable(output_mvdnfhier);
        default:
            assert(0);
            abort();
    }

    return NULL;
}


/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */

