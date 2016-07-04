#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "tv.h"
#include "pi.h"
#include "defs.h"
#include "list.h"
#include "trie.h"
#include "array.h"
#include "stack.h"
#include "variable.h"

static tv_nf truncate_copy_nf(const_tv_nf input)
{
    if (input->tag == TAGtv_cnf) {
        return to_tv_nf(new_tv_cnf(rdup_values_set_list(input->domains),
                                   rdup_variable_list(input->variables),
                                   rdup_variable_list(input->encoded_variables),
                                   rdup_constant_list(input->constants),
                                   input->encoding,
                                   new_tv_clause_list()));
    }
    if (input->tag == TAGtv_dnf) {
        return to_tv_nf(new_tv_dnf(rdup_values_set_list(input->domains),
                                   rdup_variable_list(input->variables),
                                   rdup_variable_list(input->encoded_variables),
                                   rdup_constant_list(input->constants),
                                   input->encoding,
                                   new_tv_term_list()));
    }
    assert(0);
    abort();
}

static tv_literal_set resolve_literal(tv_literal_set x, tv_literal_set y, int l)
{
    if ((member_int_list(x->pos, l) && member_int_list(y->neg, l)) ||
        (member_int_list(x->neg, l) && member_int_list(y->pos, l))) {
        tv_literal_set result = rdup_tv_literal_set(x);
        result->pos = remove_int_list(merge_int_list(result->pos, y->pos), l);
        result->neg = remove_int_list(merge_int_list(result->neg, y->neg), l);
        return result;
    }

    return tv_literal_setNIL;
}

static tv_literal_set resolve(tv_literal_set x, tv_literal_set y)
{
    unsigned int i;

    for (i = 0; i < x->pos->sz; i++) {
        unsigned l = x->pos->arr[i];
        if (member_int_list(y->neg, l)) {
            tv_literal_set result = rdup_tv_literal_set(x);
            result->pos = remove_int_list(merge_int_list(result->pos, y->pos), l);
            result->neg = remove_int_list(merge_int_list(result->neg, y->neg), l);
            return result;
        }
    }

    for (i = 0; i < x->neg->sz; i++) {
        unsigned l = x->neg->arr[i];
        if (member_int_list(y->pos, l)) {
            tv_literal_set result = rdup_tv_literal_set(x);
            result->pos = remove_int_list(merge_int_list(result->pos, y->pos), l);
            result->neg = remove_int_list(merge_int_list(result->neg, y->neg), l);
            return result;
        }
    }

    return tv_literal_setNIL;
}

/* Returns 1 iff cx is subsumed by any member of l. */
static int is_subsumed_list(const_tv_literal_set_list l, const_tv_literal_set cx)
{
    unsigned int i;

    for (i = 0; i < l->sz; i++) {
        if (is_subsumed_literal_set(l->arr[i], cx)) {
            return 1;
        }
    }
    return 0;
}

static tv_literal_set_list remove_subsumed_literal_set_list(tv_literal_set_list literal_sets)
{
    tv_literal_set_list result = new_tv_literal_set_list();
    while (literal_sets->sz > 0) {
        tv_literal_set cl = literal_sets->arr[literal_sets->sz - 1];
        literal_sets->sz -= 1;
        if (!is_subsumed_list(literal_sets, cl) &&
            !is_subsumed_list(result, cl)) {
            result = append_tv_literal_set_list(result, cl);
        } else {
            rfre_tv_literal_set(cl);
        }
    }
    fre_tv_literal_set_list(literal_sets);
    return result;
}

static tv_literal_set_list remove_subsumed_list(tv_literal_set_list literal_sets, tv_literal_set cl)
{
    unsigned int i;

    for (i = 0; i < literal_sets->sz; i++) {
        if (is_subsumed_literal_set(cl, literal_sets->arr[i])) {
            literal_sets = delete_tv_literal_set_list(literal_sets, i);
            i -= 1;
        }
    }
    return literal_sets;
}

static int is_tautology(tv_literal_set cl)
{
    unsigned int i;

    for (i = 0; i < cl->pos->sz; i++) {
        if (member_int_list(cl->neg, cl->pos->arr[i])) {
            return 1;
        }
    }
    return 0;
}

static int search_sorted_int_list(array l, int v, unsigned int *pos)
{
    int b, e, m;
    if (l->sz == 0) {
        return 0;
    }
    b = 0;
    e = l->sz - 1;
    while (b <= e) {
        m = (b + e) / 2;
        if (v > p2i(l->arr[m])) {
            b = m + 1;
        } else if (v < p2i(l->arr[m])) {
            e = m - 1;
        } else {
            *pos = m;
            return 1;
        }
    }
    return 0;
}

static array resolve_int_list_literal(array cx, array cy, int l)
{
    unsigned int posx;
    unsigned int posy;
    unsigned int ix;

    if ((search_sorted_int_list(cx, l, &posx) && search_sorted_int_list(cy, l + 1, &posy)) ||
        (search_sorted_int_list(cx, l + 1, &posx) && search_sorted_int_list(cy, l, &posy))) {
        array cr = array_new(NULL, NULL);
        unsigned int iy = 0;
        for (ix = 0; ix < cx->sz; ix++) {
            if (ix == posx) {
                continue;
            }
            while ((iy < cy->sz) && (p2i(cy->arr[iy]) / 2) < (p2i(cx->arr[ix]) / 2)) {
                if (iy != posy) {
                    array_append(cr, cy->arr[iy]);
                }
                iy += 1;
            }
            if (iy != cy->sz) {
                if (p2i(cx->arr[ix]) == p2i(cy->arr[iy]) + (p2i(cx->arr[ix]) % 2 ? 1 : -1)) {
                    array_free(cr);
                    return NULL;
                }
                if (cx->arr[ix] == cy->arr[iy]) {
                    continue;
                }
            }
            array_append(cr, cx->arr[ix]);
        }
        while (iy < cy->sz) {
            if (iy != posy) {
                array_append(cr, cy->arr[iy]);
            }
            iy += 1;
        }
        return cr;
    }
    return NULL;
}

/*
static int_list resolve_int_list(int_list cx, int_list cy)
{
    int_list cr = new_int_list();
    unsigned int iy = 0;
    unsigned int clashes = 0;
    unsigned int ix;

    for (ix = 0; ix < cx->sz; ix++) {
        while ((iy < cy->sz) && (cy->arr[iy] / 2) < (cx->arr[ix] / 2)) {
            cr = append_int_list(cr, cy->arr[iy]);
            iy += 1;
        }
        if (iy != cy->sz) {
            if (cx->arr[ix] == cy->arr[iy] + (cx->arr[ix] % 2 ? 1 : -1)) {
                clashes += 1;
                if (2 == clashes) {
                    rfre_int_list(cr);
                    return int_listNIL;
                }
                iy += 1;
                continue;
            }
            if (cx->arr[ix] == cy->arr[iy]) {
                continue;
            }
        }
        cr = append_int_list(cr, cx->arr[ix]);
    }
    if (0 == clashes) {
        rfre_int_list(cr);
        return int_listNIL;
    }
    while (iy < cy->sz) {
        cr = append_int_list(cr, cy->arr[iy]);
        iy += 1;
    }
    return cr;
}
*/

tv_nf pi_tison_trie(const_tv_nf cf)
{
    unsigned int i, l, q, x, y, z;

    tv_nf result = truncate_copy_nf(cf);
    tv_literal_set_list input = get_literal_sets(cf);
    tv_literal_set_list output = get_literal_sets(result);
    tv_literal_set model;

    stack s, sx, sy;

    trie_node c, cx, cy;

    trie db = trie_new((trie_node_destroy_func_t)array_free,
                       (trie_node_clone_func_t)array_copy);
    for (i = 0; i < input->sz; i++) {
        array l = literal_set_to_sorted_int_list(input->arr[i]);
        if (trie_is_subsumed(db, l)) {
            array_free(l);
            continue;
        }
        trie_remove_subsumed(db, l);
        trie_add(db, l, array_copy(l));
        array_free(l);
    }
    trie_gc(db);
/* We have the literal_sets in the trie now. */

    if (db->root->edges == NULL) {
        trie_free(db);
        rfre_tv_nf(result);
        return rdup_tv_nf(cf);
    }

    for (l = 0; l < cf->variables->sz; l++) {
        sx = stack_new(NULL, NULL);
/* Outer walk. */
        stack_push(sx, db->root);
        while (NULL != (cx = stack_pop(sx))) {
            for (x = 0; x < cx->edges->sz; x++) {
                if (((trie_node)cx->kids->arr[x])->is_deleted) {
                    continue;
                }
                if (((trie_node)cx->kids->arr[x])->is_terminal) {
/* Inner walk. */
                    sy = stack_new(NULL, NULL);
                    for (q = 0; q < sx->sz; q++) {
                        stack_push(sy, sx->arr[q]);
                    }
                    stack_push(sy, cx);

/* Finish the c level. */
                    z = x + 1;
                    while (NULL != (cy = stack_pop(sy))) {
                        for (y = z; y < cy->edges->sz; y++) {
                            if (((trie_node)cy->kids->arr[y])->is_deleted) {
                                continue;
                            }
                            if (((trie_node)cy->kids->arr[y])->is_terminal) {
                                array r = resolve_int_list_literal(((trie_node)cx->kids->arr[x])->value, ((trie_node)cy->kids->arr[y])->value, l * 2);
                                if (NULL == r) {
                                    continue;
                                }
                                if (trie_is_subsumed(db, r)) {
                                    array_free(r);
                                    continue;
                                }
                                trie_remove_subsumed(db, r);
                                trie_add(db, r, array_copy(r));
                                array_free(r);
                            } 
                            if (((trie_node)cy->kids->arr[y])->edges != NULL) {
                                stack_push(sy, cy->kids->arr[y]);
                            }
                        }
                        z = 0;
                    }
                    stack_free(sy);
/* End of the inner walk. */
                }
                if (((trie_node)cx->kids->arr[x])->edges != NULL) {
                    stack_push(sx, cx->kids->arr[x]);
                }
            }
        }
/* End of the outer walk. */
        stack_free(sx);
        trie_gc(db);
    }

    assert(input->sz > 0);
    model = rdup_tv_literal_set(input->arr[0]);
    rfre_int_list(model->pos);
    rfre_int_list(model->neg);
    model->pos = int_listNIL;
    model->neg = int_listNIL;

/* Convert the trie back to a clausal form. */
    s = stack_new(NULL, NULL);
    stack_push(s, db->root);
    while (NULL != (c = stack_pop(s))) {
        for (i = 0; i < c->edges->sz; i++) {
            if (((trie_node)c->kids->arr[i])->is_terminal) {
                append_tv_literal_set_list(output, int_list_to_literal_set(((trie_node)c->kids->arr[i])->value, model));
            }
            if (((trie_node)c->kids->arr[i])->edges != NULL) {
                stack_push(s, c->kids->arr[i]);
            }
        }
    }
    stack_free(s);
    trie_free(db);

    fre_tv_literal_set(model);

    set_literal_sets(result, output);

    return result;
}

/* 
 * It turned out to be difficult to cut the branch on which you are sitting
 * (i.e. deleting literal_sets from the list we are looping on). I can't think of
 * a simple way to implement this.
 */
tv_nf pi_tison(const_tv_nf cf)
{
    unsigned int i, q, x, y;

    tv_nf result = rdup_tv_nf(cf);
    tv_literal_set_list output = get_literal_sets(result);

    output = remove_subsumed_literal_set_list(output);
    for (i = 0; i < result->variables->sz; i++) {
        for (x = 0; x < output->sz; x++) {
            tv_literal_set cx = rdup_tv_literal_set(output->arr[x]);
            for (y = x + 1; y < output->sz; y++) {
                tv_literal_set r = resolve_literal(cx, output->arr[y], i);
                if (tv_literal_setNIL != r) {
                    if (!is_tautology(r)) {
                        if (is_subsumed_list(output, r)) {
                            rfre_tv_literal_set(r);
                            continue;
                        }
                        for (q = 0; q < output->sz; q++) {
                            if (is_subsumed_literal_set(r, output->arr[q])) {
                                output = delete_tv_literal_set_list(output, q);
                                if (q <= x) {
                                    x -= 1;
                                }
                                if (q <= y) {
                                    y -= 1;
                                }
                                q -= 1;
                            }
                        }
                        output = append_tv_literal_set_list(output, r);
                    } else {
                        rfre_tv_literal_set(r);
                    }
                }
            }
            rfre_tv_literal_set(cx);
        }
    }

    set_literal_sets(result, output);

    return result;
}

tv_nf pi_brute_force(const_tv_nf cf)
{
    unsigned int x, y;

    tv_nf copy = rdup_tv_nf(cf); /* We don't want to destroy the input, do we? */
    tv_nf result = truncate_copy_nf(cf);

    tv_literal_set_list input = get_literal_sets(copy);
    tv_literal_set_list output = get_literal_sets(result);

    set_literal_sets(copy, (input = remove_subsumed_literal_set_list(input)));

    for (x = 0; x < input->sz; x++) {
        tv_literal_set cx = input->arr[x];
        if (is_subsumed_list(output, cx)) {
            continue;
        }
        output = remove_subsumed_list(output, cx);
        for (y = 0; y < output->sz; y++) {
            tv_literal_set r = resolve(cx, output->arr[y]);
            if (tv_literal_setNIL != r) {
                if (!is_tautology(r)) {
                    input = append_tv_literal_set_list(input, r);
                } else {
                    rfre_tv_literal_set(r);
                }
            }
        }
        output = append_tv_literal_set_list(output, rdup_tv_literal_set(cx));
    }

    rfre_tv_nf(copy);

    set_literal_sets(result, output);

    return result;
}
