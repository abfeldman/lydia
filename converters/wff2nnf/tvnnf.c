#include <assert.h>

#include "tvnnf.h"

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

static tv_nnf_expr convert_wff(tv_wff_expr e)
{
    tv_nnf_expr v;

    switch (e->tag) {
	case TAGtv_wff_e_not:
	    v = convert_wff(to_tv_wff_e_not(e)->n);
	    assert(v->tag = TAGtv_nnf_e_var);
	    return to_tv_nnf_expr(new_tv_nnf_e_not(to_tv_nnf_e_var(v)));
	case TAGtv_wff_e_and:
	    return to_tv_nnf_expr(new_tv_nnf_e_and(convert_wff(to_tv_wff_e_and(e)->lhs),
						   convert_wff(to_tv_wff_e_and(e)->rhs)));
	case TAGtv_wff_e_or:
	    return to_tv_nnf_expr(new_tv_nnf_e_or(convert_wff(to_tv_wff_e_or(e)->lhs),
						  convert_wff(to_tv_wff_e_or(e)->rhs)));
	case TAGtv_wff_e_var:
	    return to_tv_nnf_expr(new_tv_nnf_e_var(to_tv_wff_e_var(e)->v));
	case TAGtv_wff_e_const:
	case TAGtv_wff_e_equiv:
	case TAGtv_wff_e_impl:
/* Should have been eliminated by the translation process. */
	    break;
    }
    assert(0);
    abort();
}

tv_nnf_expr convert_nnf(tv_wff_expr *e)
{
    if (*e == tv_wff_exprNIL) {
/* It is ok to have an empty Wff. */
	return tv_nnf_exprNIL;
    }
    *e = replace_equiv(*e);
    *e = replace_impl(*e);
    *e = push_not(*e);

    return convert_wff(*e);
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
