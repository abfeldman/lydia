#include <assert.h>

#include "sorted_int_list.h"
#include "flat_kb.h"
#include "wff2cnf.h"

static tv_wff_expr replace_equiv(tv_wff_expr e)
{
    if (e->tag == TAGtv_wff_e_equiv) {
        tv_wff_expr lhs = replace_equiv(to_tv_wff_e_equiv(e)->lhs);
        tv_wff_expr rhs = replace_equiv(to_tv_wff_e_equiv(e)->rhs);
        fre_tv_wff_expr(e);
        return to_tv_wff_expr(new_tv_wff_e_and(to_tv_wff_expr(new_tv_wff_e_impl(rdup_tv_wff_expr(lhs), rhs)),
                                               to_tv_wff_expr(new_tv_wff_e_impl(rdup_tv_wff_expr(rhs), lhs))));
    }

    switch (e->tag) {
        case TAGtv_wff_e_not:
            to_tv_wff_e_not(e)->n = replace_equiv(to_tv_wff_e_not(e)->n);
            break;
        case TAGtv_wff_e_and:
            to_tv_wff_e_and(e)->lhs = replace_equiv(to_tv_wff_e_and(e)->lhs);
            to_tv_wff_e_and(e)->rhs = replace_equiv(to_tv_wff_e_and(e)->rhs);
            break;
        case TAGtv_wff_e_or:
            to_tv_wff_e_or(e)->lhs = replace_equiv(to_tv_wff_e_or(e)->lhs);
            to_tv_wff_e_or(e)->rhs = replace_equiv(to_tv_wff_e_or(e)->rhs);
            break;
        case TAGtv_wff_e_impl:
            to_tv_wff_e_impl(e)->lhs = replace_equiv(to_tv_wff_e_impl(e)->lhs);
            to_tv_wff_e_impl(e)->rhs = replace_equiv(to_tv_wff_e_impl(e)->rhs);
            break;
        default:
            break;
    }

    return e;
}

static tv_wff_expr replace_impl(tv_wff_expr e)
{
    if (e->tag == TAGtv_wff_e_impl) {
        tv_wff_expr lhs = replace_impl(to_tv_wff_e_impl(e)->lhs);
        tv_wff_expr rhs = replace_impl(to_tv_wff_e_impl(e)->rhs);
        fre_tv_wff_expr(e);
        return to_tv_wff_expr(new_tv_wff_e_or(to_tv_wff_expr(new_tv_wff_e_not(lhs)), rhs));
    }

    switch (e->tag) {
        case TAGtv_wff_e_not:
            to_tv_wff_e_not(e)->n = replace_impl(to_tv_wff_e_not(e)->n);
            break;
        case TAGtv_wff_e_and:
            to_tv_wff_e_and(e)->lhs = replace_impl(to_tv_wff_e_and(e)->lhs);
            to_tv_wff_e_and(e)->rhs = replace_impl(to_tv_wff_e_and(e)->rhs);
            break;
        case TAGtv_wff_e_or:
            to_tv_wff_e_or(e)->lhs = replace_impl(to_tv_wff_e_or(e)->lhs);
            to_tv_wff_e_or(e)->rhs = replace_impl(to_tv_wff_e_or(e)->rhs);
            break;
        case TAGtv_wff_e_equiv:
            assert(0); /* Should have been eliminated in pass 1. */
            break;
        default:
            break;
    }

    return e;
}

static tv_wff_expr push_not(tv_wff_expr e)
{
    if (e->tag == TAGtv_wff_e_not) {
        if (to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_not) { /* Double negation. */
            tv_wff_expr r = to_tv_wff_e_not(to_tv_wff_e_not(e)->n)->n;
            fre_tv_wff_expr(to_tv_wff_e_not(e)->n);
            fre_tv_wff_expr(e);
            return push_not(r);
        }
        if (to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_or) { /* de Morgan */
            tv_wff_expr lhs = to_tv_wff_e_or(to_tv_wff_e_not(e)->n)->lhs;
            tv_wff_expr rhs = to_tv_wff_e_or(to_tv_wff_e_not(e)->n)->rhs;
            tv_wff_expr r = to_tv_wff_expr(new_tv_wff_e_and(to_tv_wff_expr(new_tv_wff_e_not(lhs)),
                                                            to_tv_wff_expr(new_tv_wff_e_not(rhs))));
            fre_tv_wff_expr(to_tv_wff_e_not(e)->n);
            fre_tv_wff_expr(e);
            return push_not(r);
        }
        if (to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_and) { /* de Morgan */
            tv_wff_expr lhs = to_tv_wff_e_and(to_tv_wff_e_not(e)->n)->lhs;
            tv_wff_expr rhs = to_tv_wff_e_and(to_tv_wff_e_not(e)->n)->rhs;
            tv_wff_expr r = to_tv_wff_expr(new_tv_wff_e_or(to_tv_wff_expr(new_tv_wff_e_not(lhs)), 
                                                           to_tv_wff_expr(new_tv_wff_e_not(rhs))));
            fre_tv_wff_expr(to_tv_wff_e_not(e)->n);
            fre_tv_wff_expr(e);
            return push_not(r);
        }
        to_tv_wff_e_not(e)->n = push_not(to_tv_wff_e_not(e)->n);
        return e;
    }

    switch (e->tag) {
        case TAGtv_wff_e_and:
            to_tv_wff_e_and(e)->lhs = push_not(to_tv_wff_e_and(e)->lhs);
            to_tv_wff_e_and(e)->rhs = push_not(to_tv_wff_e_and(e)->rhs);
            break;
        case TAGtv_wff_e_or:
            to_tv_wff_e_or(e)->lhs = push_not(to_tv_wff_e_or(e)->lhs);
            to_tv_wff_e_or(e)->rhs = push_not(to_tv_wff_e_or(e)->rhs);
            break;
        case TAGtv_wff_e_equiv:
        case TAGtv_wff_e_impl:
            assert(0); /* Should have been eliminated in pass 1. */
            break;
        default:
            break;
    }

    return e;
}

static tv_wff_expr distribute_or(tv_wff_expr e, int *success)
{
    tv_wff_expr r;

    if (e->tag == TAGtv_wff_e_or) {
        if (to_tv_wff_e_or(e)->rhs->tag == TAGtv_wff_e_and) {
            *success = 1;
            r = to_tv_wff_expr(new_tv_wff_e_and(to_tv_wff_expr(new_tv_wff_e_or(to_tv_wff_e_or(e)->lhs, to_tv_wff_e_and(to_tv_wff_e_or(e)->rhs)->lhs)),
                                                to_tv_wff_expr(new_tv_wff_e_or(rdup_tv_wff_expr(to_tv_wff_e_or(e)->lhs), to_tv_wff_e_and(to_tv_wff_e_or(e)->rhs)->rhs))));
            fre_tv_wff_expr(to_tv_wff_e_or(e)->rhs);
            fre_tv_wff_expr(e);
            return r;
        } else {
            to_tv_wff_e_and(e)->rhs = distribute_or(to_tv_wff_e_and(e)->rhs, success);
        }
        if (to_tv_wff_e_or(e)->lhs->tag == TAGtv_wff_e_and) {
            *success = 1;
            r = to_tv_wff_expr(new_tv_wff_e_and(to_tv_wff_expr(new_tv_wff_e_or(to_tv_wff_e_and(to_tv_wff_e_or(e)->lhs)->lhs, to_tv_wff_e_or(e)->rhs)),
                                                to_tv_wff_expr(new_tv_wff_e_or(to_tv_wff_e_and(to_tv_wff_e_or(e)->lhs)->rhs, rdup_tv_wff_expr(to_tv_wff_e_or(e)->rhs)))));
            fre_tv_wff_expr(to_tv_wff_e_or(e)->lhs);
            fre_tv_wff_expr(e);
            return r;
        } else {
            to_tv_wff_e_and(e)->lhs = distribute_or(to_tv_wff_e_and(e)->lhs, success);
        }

        return e;
    }

    switch (e->tag) {
        case TAGtv_wff_e_and:
            to_tv_wff_e_and(e)->lhs = distribute_or(to_tv_wff_e_and(e)->lhs, success);
            to_tv_wff_e_and(e)->rhs = distribute_or(to_tv_wff_e_and(e)->rhs, success);
            break;
        case TAGtv_wff_e_equiv:
        case TAGtv_wff_e_impl:
            assert(0); /* Should have been eliminated in pass 1. */
            break;
        default:
            break;
    }

    return e;
}

static tv_wff_expr fold_constants(tv_wff_expr e)
{
    tv_wff_expr r;

    switch (e->tag) {
        case TAGtv_wff_e_or:
            to_tv_wff_e_or(e)->lhs = fold_constants(to_tv_wff_e_or(e)->lhs);
            to_tv_wff_e_or(e)->rhs = fold_constants(to_tv_wff_e_or(e)->rhs);
            if (to_tv_wff_e_or(e)->lhs->tag == TAGtv_wff_e_const && !to_tv_wff_e_const(to_tv_wff_e_or(e)->lhs)->c) {
                r = to_tv_wff_e_or(e)->rhs;
                rfre_tv_wff_expr(to_tv_wff_e_or(e)->lhs);
                fre_tv_wff_expr(e);
                return r;
            }
            if (to_tv_wff_e_or(e)->rhs->tag == TAGtv_wff_e_const && !to_tv_wff_e_const(to_tv_wff_e_or(e)->rhs)->c) {
                r = to_tv_wff_e_or(e)->lhs;
                rfre_tv_wff_expr(to_tv_wff_e_or(e)->rhs);
                fre_tv_wff_expr(e);
                return r;
            }
            if (to_tv_wff_e_or(e)->lhs->tag == TAGtv_wff_e_const && to_tv_wff_e_const(to_tv_wff_e_or(e)->lhs)->c) {
                rfre_tv_wff_expr(e);
                return to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
            }
            if (to_tv_wff_e_or(e)->rhs->tag == TAGtv_wff_e_const && to_tv_wff_e_const(to_tv_wff_e_or(e)->rhs)->c) {
                rfre_tv_wff_expr(e);
                return to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
            }
            break;
        case TAGtv_wff_e_and:
            to_tv_wff_e_and(e)->lhs = fold_constants(to_tv_wff_e_and(e)->lhs);
            to_tv_wff_e_and(e)->rhs = fold_constants(to_tv_wff_e_and(e)->rhs);
            if (to_tv_wff_e_and(e)->lhs->tag == TAGtv_wff_e_const && to_tv_wff_e_const(to_tv_wff_e_and(e)->lhs)->c) {
                r = to_tv_wff_e_and(e)->rhs;
                rfre_tv_wff_expr(to_tv_wff_e_and(e)->lhs);
                fre_tv_wff_expr(e);
                return r;
            }
            if (to_tv_wff_e_and(e)->rhs->tag == TAGtv_wff_e_const && to_tv_wff_e_const(to_tv_wff_e_and(e)->rhs)->c) {
                r = to_tv_wff_e_and(e)->lhs;
                rfre_tv_wff_expr(to_tv_wff_e_and(e)->rhs);
                fre_tv_wff_expr(e); 
                return r;
            }
            if (to_tv_wff_e_and(e)->lhs->tag == TAGtv_wff_e_const && !to_tv_wff_e_const(to_tv_wff_e_and(e)->lhs)->c) {
                rfre_tv_wff_expr(e);
                return to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
            }
            if (to_tv_wff_e_and(e)->rhs->tag == TAGtv_wff_e_const && !to_tv_wff_e_const(to_tv_wff_e_and(e)->rhs)->c) {
                rfre_tv_wff_expr(e);
                return to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
            }
            break;
        case TAGtv_wff_e_not:
            if (to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_const) {
                if (to_tv_wff_e_const(to_tv_wff_e_not(e)->n)->c) {
                    rfre_tv_wff_expr(e);
                    return to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
                } else if (!to_tv_wff_e_const(to_tv_wff_e_not(e)->n)->c) {
                    rfre_tv_wff_expr(e);
                    return to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                } else {
                    assert(0);
                }
            }
            break;
        case TAGtv_wff_e_equiv:
        case TAGtv_wff_e_impl:
            assert(0); /* Should have been eliminated in pass 1. */
            break;
        default:
            break;
    }

    return e;
}

static tv_wff_expr expr_to_term(tv_wff_expr e, int_list pos, int_list neg)
{
    switch (e->tag) {
        case TAGtv_wff_e_or:
            expr_to_term(to_tv_wff_e_or(e)->lhs, pos, neg);
            expr_to_term(to_tv_wff_e_or(e)->rhs, pos, neg);
            break;
        case TAGtv_wff_e_not:
            assert(to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_var || to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_const);
            if (to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_var) {
                neg = append_int_list(neg, to_tv_wff_e_var(to_tv_wff_e_not(e)->n)->v);
            }
            break;
        case TAGtv_wff_e_var:
            pos = append_int_list(pos, to_tv_wff_e_var(e)->v);
            break;
        case TAGtv_wff_e_const:
/* Noop. */
            break;
        case TAGtv_wff_e_and:
        case TAGtv_wff_e_equiv:
        case TAGtv_wff_e_impl:
            assert(0); /* Should have been eliminated in pass 1. */
            break;
        default:
            break;
    }
    return e;
}

static tv_wff_expr eliminate_tautologies(tv_wff_expr e)
{
    int_list pos;
    int_list neg;
    int tautology;

    switch (e->tag) {
        case TAGtv_wff_e_and:
            if (to_tv_wff_e_and(e)->lhs->tag == TAGtv_wff_e_or) {
                pos = new_int_list();
                neg = new_int_list();
                expr_to_term(to_tv_wff_e_and(e)->lhs, pos, neg);
                tautology = !is_sorted_term_consistent(sort_int_list(pos), sort_int_list(neg));
                rfre_int_list(pos);
                rfre_int_list(neg);
                if (tautology) {
                    rfre_tv_wff_expr(to_tv_wff_e_and(e)->lhs);
                    to_tv_wff_e_and(e)->lhs = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                }
            } else {
                to_tv_wff_e_and(e)->lhs = eliminate_tautologies(to_tv_wff_e_and(e)->lhs);
            }
            if (to_tv_wff_e_and(e)->rhs->tag == TAGtv_wff_e_or) {
                pos = new_int_list();
                neg = new_int_list();
                expr_to_term(to_tv_wff_e_and(e)->rhs, pos, neg);
                tautology = !is_sorted_term_consistent(sort_int_list(pos), sort_int_list(neg));
                rfre_int_list(pos);
                rfre_int_list(neg);
                if (tautology) {
                    rfre_tv_wff_expr(to_tv_wff_e_and(e)->rhs);
                    to_tv_wff_e_and(e)->rhs = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                }
            } else {
                to_tv_wff_e_and(e)->rhs = eliminate_tautologies(to_tv_wff_e_and(e)->rhs);
            }
            break;
        case TAGtv_wff_e_or:
            pos = new_int_list();
            neg = new_int_list();
            expr_to_term(e, pos, neg);
            tautology = !is_sorted_term_consistent(sort_int_list(pos), sort_int_list(neg));
            rfre_int_list(pos);
            rfre_int_list(neg);
            if (tautology) {
                rfre_tv_wff_expr(e);
                return tv_wff_exprNIL;
            }
            break;
        case TAGtv_wff_e_equiv:
        case TAGtv_wff_e_impl:
            assert(0); /* Should have been eliminated in pass 1. */
            break;
        default:
            break;
    }

    return e;
}

static int member_tv_clause_list(const_tv_clause_list haystack, const_tv_clause needle)
{
    register unsigned int i;

    for (i = 0; i < haystack->sz; i++) {
        if (isequal_int_list(haystack->arr[i]->pos, needle->pos) &&
            isequal_int_list(haystack->arr[i]->neg, needle->neg)) {
            return 1;
        }
    }

    return 0;
}

static tv_clause_list collect_clauses(tv_wff_expr e, tv_clause_list clauses, tv_clause cl)
{
    switch (e->tag) {
        case TAGtv_wff_e_and:
            if (to_tv_wff_e_and(e)->lhs->tag != TAGtv_wff_e_and) {
                tv_clause l = new_tv_clause(new_int_list(), new_int_list());
                clauses = collect_clauses(to_tv_wff_e_and(e)->lhs, clauses, l);
                if (!member_tv_clause_list(clauses, l)) {
                    sort_int_list(l->pos);
                    sort_int_list(l->neg);
                    clauses = append_tv_clause_list(clauses, l);
                } else {
                    rfre_tv_clause(l);
                }
            } else {
                clauses = collect_clauses(to_tv_wff_e_and(e)->lhs, clauses, cl);
            }
            if (to_tv_wff_e_and(e)->rhs->tag != TAGtv_wff_e_and) {
                tv_clause r = new_tv_clause(new_int_list(), new_int_list());
                clauses = collect_clauses(to_tv_wff_e_and(e)->rhs, clauses, r);
                if (!member_tv_clause_list(clauses, r)) {
                    sort_int_list(r->pos);
                    sort_int_list(r->neg);
                    clauses = append_tv_clause_list(clauses, r);
                } else {
                    rfre_tv_clause(r);
                }
            } else {
                clauses = collect_clauses(to_tv_wff_e_and(e)->rhs, clauses, cl);
            }
            break;
        case TAGtv_wff_e_or:
            clauses = collect_clauses(to_tv_wff_e_or(e)->lhs, clauses, cl);
            clauses = collect_clauses(to_tv_wff_e_or(e)->rhs, clauses, cl);
            break;
        case TAGtv_wff_e_var:
            if (!member_int_list(cl->pos, to_tv_wff_e_var(e)->v)) {
                cl->pos = append_int_list(cl->pos, to_tv_wff_e_var(e)->v);
            }
            break;
        case TAGtv_wff_e_not:
            assert(to_tv_wff_e_not(e)->n->tag == TAGtv_wff_e_var);
            if (!member_int_list(cl->neg, to_tv_wff_e_var(to_tv_wff_e_not(e)->n)->v)) {
                cl->neg = append_int_list(cl->neg, to_tv_wff_e_var(to_tv_wff_e_not(e)->n)->v);
            }
            break;
        default:
            assert(0);
    }
    return clauses;
}

static tv_clause_list tv_wff_expr_to_clauses(tv_wff_expr *e, tv_clause_list clauses)
{
    int success;

    if (*e == tv_wff_exprNIL) {
/* It is ok to have an empty Wff. */
        return clauses;
    }
    *e = replace_equiv(*e);
    *e = replace_impl(*e);
    *e = push_not(*e);
    *e = fold_constants(*e);
    do {
        success = 0;
        *e = distribute_or(*e, &success);
    } while (success);

    *e = eliminate_tautologies(*e);
    if (*e == tv_wff_exprNIL) {
/* Folding reduced the sentence to validty. */
        return clauses;
    }
    *e = fold_constants(*e);

    if ((*e)->tag == TAGtv_wff_e_and) {
        clauses = collect_clauses(*e, clauses, tv_clauseNIL);
    } else if ((*e)->tag == TAGtv_wff_e_const) {
        if (to_tv_wff_e_const(*e)->c) {
            return clauses;
        } else if (!to_tv_wff_e_const(*e)->c) {
            rfre_tv_clause_list(clauses);
            return append_tv_clause_list(new_tv_clause_list(), new_tv_clause(new_int_list(), new_int_list()));
        } else {
            assert(0);
        }
    } else {
        tv_clause cl = new_tv_clause(new_int_list(), new_int_list());
        sort_int_list(cl->pos);
        sort_int_list(cl->neg);
        clauses = append_tv_clause_list(clauses, cl);
        clauses = collect_clauses(*e, clauses, cl);
    }
    return clauses;
}

tv_cnf tv_wff_to_tv_cnf(tv_wff wff)
{
    tv_cnf cnf;

    tv_clause_list clauses;

    register unsigned int ix;

    cnf = new_tv_cnf(rdup_values_set_list(wff->domains),
                     rdup_variable_list(wff->variables),
                     rdup_variable_list(wff->encoded_variables),
                     rdup_constant_list(wff->constants),
                     wff->encoding,
                     new_tv_clause_list());

    for (ix = 0; ix < wff->e->sz; ix++) {
        clauses = new_tv_clause_list();
        clauses = tv_wff_expr_to_clauses(&(wff->e->arr[ix]), clauses);
        concat_tv_clause_list(cnf->clauses, clauses);
    }

    return cnf;
}

int wff2cnf(const_serializable input, serializable *output)
{
    tv_wff wff;
    tv_cnf cnf;

    node cnf_node;

    register unsigned int ix;

    if (input->tag == TAGtv_wff_flat_kb) {
        wff = to_tv_wff(to_tv_wff_flat_kb(input)->constraints);
        *output = to_serializable(new_tv_cnf_flat_kb(to_tv_wff_flat_kb(input)->name,
                                                     to_kb(tv_wff_to_tv_cnf(wff))));
        if (tv_clause_listNIL == to_tv_cnf(to_tv_cnf_flat_kb(*output)->constraints)->clauses) {
            rfre_serializable(*output);
            return 0;
        }
    } else if (input->tag == TAGtv_wff_hierarchy) {
        *output = to_serializable(new_tv_cnf_hierarchy(new_node_list()));
        for (ix = 0; ix < to_tv_wff_hierarchy(input)->nodes->sz; ix++) {
            wff = to_tv_wff(to_tv_wff_hierarchy(input)->nodes->arr[ix]->constraints);
            cnf = tv_wff_to_tv_cnf(wff);
            if (tv_clause_listNIL == cnf->clauses) {
                rfre_serializable(*output);
                return 0;
            }
            cnf_node = new_node(to_tv_wff_hierarchy(input)->nodes->arr[ix]->type,
                                rdup_edge_list(to_tv_wff_hierarchy(input)->nodes->arr[ix]->edges),
                                to_kb(cnf));
            append_node_list(to_tv_cnf_hierarchy(*output)->nodes, cnf_node);
        }
    } else {
        assert(0);
    }

    return 1;
}
