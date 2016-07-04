#include "tv.h"
#include "stat.h"
#include "defs.h"
#include "hash.h"
#include "util.h"
#include "obdd.h"
#include "config.h"
#include "flat_kb.h"
#include "wff2obdd.h"
#include "variable.h"
#include "hierarchy.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define pair(i, j) (((i) + (j)) * ((i) + (j) + 1) / 2 + (i))
#define memdup(buf, n) (memcpy(malloc((n)), (buf), (n)))

static int is_tv_wff_e_false(const_tv_wff_expr e)
{
    return (e->tag == TAGtv_wff_e_const) && (to_tv_wff_e_const(e)->c == LYDIA_FALSE);
}

static int is_tv_wff_e_true(const_tv_wff_expr e)
{
    return (e->tag == TAGtv_wff_e_const) && (to_tv_wff_e_const(e)->c == LYDIA_TRUE);
}
/*
static int is_tv_wff_e_const(const_tv_wff_expr e)
{
    return (e->tag == TAGtv_wff_e_const);
}
*/
static unsigned long node_hash_function(const char *key, unsigned int UNUSED(key_length))
{
    obdd_non_terminal_node node = to_obdd_non_terminal_node(key);

    return pair(node->var, pair(node->low, node->high)) % 15485863L;
}

static unsigned long pair_hash_function(const char *key, unsigned int UNUSED(key_length))
{
    unsigned int *q = (unsigned int *)key;

    return pair(q[0], q[1]) % 15485863L;
}

/* Use this for non-terminal nodes only. */
static int add_obdd_node(obdd_node_list nodes,
                         hash_table *index,
                         int var,
                         int low,
                         int high)
{
    int *pdata = NULL;

    obdd_non_terminal_node node;

    if (low == high) {
        return low;
    }

    node = new_obdd_non_terminal_node(var, low, high);

    if (0 == hash_find(index,
                       (char *)node,
                       sizeof(struct str_obdd_non_terminal_node),
                       (void *)&pdata)) {
        increase_int_counter("cache hits");
        rfre_obdd_non_terminal_node(node);
        return *pdata;
    }
    
    hash_add(index,
             (char *)node,
             sizeof(struct str_obdd_non_terminal_node),
             &nodes->sz,
             sizeof(int),
             NULL);
    append_obdd_node_list(nodes, to_obdd_node(node));

    return nodes->sz - 1;
}

static void negate_obdd(obdd_node_list nodes)
{
    register unsigned int ix;
    obdd_non_terminal_node node;

    for (ix = 2; ix < nodes->sz; ix++) {
        assert(nodes->arr[ix]->tag == TAGobdd_non_terminal_node);
        node = to_obdd_non_terminal_node(nodes->arr[ix]);
        if (node->low < 2) {
            node->low = !node->low;
        }
        if (node->high < 2) {
            node->high = !node->high;
        }
    }
}

static int combine_obdd(const_obdd_node_list l_nodes,
                        const_obdd_node_list r_nodes,
                        obdd_node_list result_nodes,
                        hash_table *cache,
                        hash_table *result_index,
                        unsigned int l_node_index,
                        unsigned int r_node_index,
                        const int operator)
{
    int result;
    unsigned int key[2];
    int *pdata = NULL;

    const_obdd_node l_node, r_node;

    obdd_non_terminal_node l_nt_node = obdd_non_terminal_nodeNIL;
    obdd_non_terminal_node r_nt_node = obdd_non_terminal_nodeNIL;

    int vf, vt, var;

    if (l_node_index < 2 && r_node_index < 2) {
        switch (operator) {
            case TAGtv_wff_e_and:
                return l_node_index && r_node_index;
            case TAGtv_wff_e_or:
                return l_node_index || r_node_index;
            case TAGtv_wff_e_impl:
                return !l_node_index || r_node_index;
                break;
            case TAGtv_wff_e_equiv:
                return l_node_index == r_node_index;
            default:
                assert(0);
                abort();
        }
    }

    key[0] = l_node_index;
    key[1] = r_node_index;

    if (0 == hash_find(cache, (char *)key, sizeof(unsigned int) * 2, (void *)&pdata)) {
        increase_int_counter("cache hits");
        return *pdata;
    }

    l_node = l_nodes->arr[l_node_index];
    r_node = r_nodes->arr[r_node_index];

    if (l_node->tag == TAGobdd_terminal_node ||
        r_node->tag == TAGobdd_terminal_node) { /* But not both of them. */
/**
 * Recall, that the variable of a terminal node has, by convention, a
 * higher index than that of any non-terminal node.
 */
        if (r_node->tag == TAGobdd_terminal_node) {
            assert(l_node->tag == TAGobdd_non_terminal_node);
            l_nt_node = to_obdd_non_terminal_node(l_node);

            vf = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_nt_node->low, r_node_index, operator);
            vt = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_nt_node->high, r_node_index, operator);
            var = l_nt_node->var;
        } else /* if (l_node->tag == TAGobdd_terminal_node) */ {
            assert(r_node->tag == TAGobdd_non_terminal_node);
            r_nt_node = to_obdd_non_terminal_node(r_node);

            vf = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_node_index, r_nt_node->low, operator);
            vt = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_node_index, r_nt_node->high, operator);
            var = r_nt_node->var;
        }
    } else {
        l_nt_node = to_obdd_non_terminal_node(l_node);
        r_nt_node = to_obdd_non_terminal_node(r_node);
        if (l_nt_node->var == r_nt_node->var) {
            vf = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_nt_node->low, r_nt_node->low, operator);
            vt = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_nt_node->high, r_nt_node->high, operator);
            var = l_nt_node->var;
        } else if (l_nt_node->var < r_nt_node->var) {
            vf = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_nt_node->low, r_node_index, operator);
            vt = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_nt_node->high, r_node_index, operator);
            var = l_nt_node->var;
        } else /* if (l_nt_node->var > r_nt_node->var) */ {
            vf = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_node_index, r_nt_node->low, operator);
            vt = combine_obdd(l_nodes, r_nodes, result_nodes, cache, result_index, l_node_index, r_nt_node->high, operator);
            var = r_nt_node->var;
        }
    }

    result = add_obdd_node(result_nodes, result_index, var, vf, vt);
    hash_add(cache,
             (char *)key,
             sizeof(unsigned int) * 2,
             &result,
             sizeof(int),
             NULL);

    return result;
}

static tv_wff_expr cond_tv_wff_expr(tv_wff_expr e, int var, lydia_bool val)
{
    switch (e->tag) {
        case TAGtv_wff_e_not:
            to_tv_wff_e_not(e)->n = cond_tv_wff_expr(to_tv_wff_e_not(e)->n, var, val);
            if (is_tv_wff_e_true(to_tv_wff_e_not(e)->n)) {
                rfre_tv_wff_expr(e);
                e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
                break;
            }
            if (is_tv_wff_e_false(to_tv_wff_e_not(e)->n)) {
                rfre_tv_wff_expr(e);
                e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
            }
            break;
        case TAGtv_wff_e_or:
            to_tv_wff_e_or(e)->lhs = cond_tv_wff_expr(to_tv_wff_e_or(e)->lhs, var, val);
            to_tv_wff_e_or(e)->rhs = cond_tv_wff_expr(to_tv_wff_e_or(e)->rhs, var, val);
            if (is_tv_wff_e_true(to_tv_wff_e_or(e)->lhs) ||
                is_tv_wff_e_true(to_tv_wff_e_or(e)->rhs)) {
                rfre_tv_wff_expr(e);
                e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                break;
            }
            if (is_tv_wff_e_false(to_tv_wff_e_or(e)->lhs)) {
                tv_wff_expr rhs = to_tv_wff_e_or(e)->rhs;
                rfre_tv_wff_expr(to_tv_wff_e_or(e)->lhs);
                fre_tv_wff_expr(e);
                e = rhs;
                break;
            }
            if (is_tv_wff_e_false(to_tv_wff_e_or(e)->rhs)) {
                tv_wff_expr lhs = to_tv_wff_e_or(e)->lhs;
                rfre_tv_wff_expr(to_tv_wff_e_or(e)->rhs);
                fre_tv_wff_expr(e);
                e = lhs;
            }
            break;
        case TAGtv_wff_e_and:
            to_tv_wff_e_and(e)->lhs = cond_tv_wff_expr(to_tv_wff_e_and(e)->lhs, var, val);
            to_tv_wff_e_and(e)->rhs = cond_tv_wff_expr(to_tv_wff_e_and(e)->rhs, var, val);
            if (is_tv_wff_e_false(to_tv_wff_e_and(e)->lhs) ||
                is_tv_wff_e_false(to_tv_wff_e_and(e)->rhs)) {
                rfre_tv_wff_expr(e);
                e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
                break;
            }
            if (is_tv_wff_e_true(to_tv_wff_e_and(e)->lhs)) {
                tv_wff_expr rhs = to_tv_wff_e_and(e)->rhs;
                rfre_tv_wff_expr(to_tv_wff_e_and(e)->lhs);
                fre_tv_wff_expr(e);
                e = rhs;
                break;
            }
            if (is_tv_wff_e_true(to_tv_wff_e_and(e)->rhs)) {
                tv_wff_expr lhs = to_tv_wff_e_and(e)->lhs;
                rfre_tv_wff_expr(to_tv_wff_e_and(e)->rhs);
                fre_tv_wff_expr(e);
                e = lhs;
            }
            break;
        case TAGtv_wff_e_equiv:
            to_tv_wff_e_equiv(e)->lhs = cond_tv_wff_expr(to_tv_wff_e_equiv(e)->lhs, var, val);
            to_tv_wff_e_equiv(e)->rhs = cond_tv_wff_expr(to_tv_wff_e_equiv(e)->rhs, var, val);
            if (is_tv_wff_e_true(to_tv_wff_e_equiv(e)->lhs)) {
                tv_wff_expr rhs = to_tv_wff_e_equiv(e)->rhs;
                rfre_tv_wff_expr(to_tv_wff_e_equiv(e)->lhs);
                fre_tv_wff_expr(e);
                e = rhs;
                break;
            }
            if (is_tv_wff_e_true(to_tv_wff_e_equiv(e)->rhs)) {
                tv_wff_expr lhs = to_tv_wff_e_equiv(e)->lhs;
                rfre_tv_wff_expr(to_tv_wff_e_equiv(e)->rhs);
                fre_tv_wff_expr(e);
                e = lhs;
                break;
            }
            if (is_tv_wff_e_false(to_tv_wff_e_equiv(e)->lhs)) {
                tv_wff_e_not rhs = new_tv_wff_e_not(to_tv_wff_e_equiv(e)->rhs);
                if (is_tv_wff_e_true(rhs->n)) {
                    rfre_tv_wff_expr(e);
                    e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
                    break;
                }
                if (is_tv_wff_e_false(rhs->n)) {
                    rfre_tv_wff_expr(e);
                    e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                    break;
                }
                rfre_tv_wff_expr(to_tv_wff_e_equiv(e)->lhs);
                fre_tv_wff_expr(e);
                e = to_tv_wff_expr(rhs);
                break;
            }
            if (is_tv_wff_e_false(to_tv_wff_e_equiv(e)->rhs)) {
                tv_wff_e_not lhs = new_tv_wff_e_not(to_tv_wff_e_equiv(e)->lhs);
                if (is_tv_wff_e_true(lhs->n)) {
                    rfre_tv_wff_expr(e);
                    e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
                    break;
                }
                if (is_tv_wff_e_false(lhs->n)) {
                    rfre_tv_wff_expr(e);
                    e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                    break;
                }
                rfre_tv_wff_expr(to_tv_wff_e_equiv(e)->rhs);
                fre_tv_wff_expr(e);
                e = to_tv_wff_expr(lhs);
            }
            break;
        case TAGtv_wff_e_impl:
            to_tv_wff_e_impl(e)->lhs = cond_tv_wff_expr(to_tv_wff_e_impl(e)->lhs, var, val);
            to_tv_wff_e_impl(e)->rhs = cond_tv_wff_expr(to_tv_wff_e_impl(e)->rhs, var, val);
            if (is_tv_wff_e_false(to_tv_wff_e_impl(e)->lhs) ||
                is_tv_wff_e_true(to_tv_wff_e_impl(e)->rhs)) {
                rfre_tv_wff_expr(e);
                e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                break;
            }
            if (is_tv_wff_e_true(to_tv_wff_e_impl(e)->lhs)) {
                tv_wff_expr rhs = to_tv_wff_e_impl(e)->rhs;
                rfre_tv_wff_expr(to_tv_wff_e_impl(e)->lhs);
                fre_tv_wff_expr(e);
                e = rhs;
                break;
            }
            if (is_tv_wff_e_false(to_tv_wff_e_impl(e)->rhs)) {
                tv_wff_e_not lhs = new_tv_wff_e_not(to_tv_wff_e_impl(e)->lhs);
                if (is_tv_wff_e_true(lhs->n)) {
                    rfre_tv_wff_expr(e);
                    e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_FALSE));
                    break;
                }
                if (is_tv_wff_e_false(lhs->n)) {
                    rfre_tv_wff_expr(e);
                    e = to_tv_wff_expr(new_tv_wff_e_const(LYDIA_TRUE));
                    break;
                }
                rfre_tv_wff_expr(to_tv_wff_e_impl(e)->rhs);
                fre_tv_wff_expr(e);
                e = to_tv_wff_expr(lhs);
            }
            break;
        case TAGtv_wff_e_var:
            if (to_tv_wff_e_var(e)->v == var) {
                rfre_tv_wff_expr(e);
                e = to_tv_wff_expr(new_tv_wff_e_const(val));
            }
            break;
        case TAGtv_wff_e_const:
            to_tv_wff_e_const(e)->c &= val;
            break;
    }
    return e;
}

static int build_obdd_conditioning(unsigned int var,
                                   unsigned int cnt,
                                   const_tv_wff_expr e,
                                   obdd_node_list nodes,
                                   hash_table *index)
{
    tv_wff_expr ef, et;
    int vf, vt;

    if (var >= cnt) {
        if (is_tv_wff_e_false(e)) {
            return 0;
        }
        if (is_tv_wff_e_true(e)) {
            return 1;
        }
        assert(0);
        abort();
    }
    ef = cond_tv_wff_expr(rdup_tv_wff_expr(e), var, LYDIA_FALSE);
    et = cond_tv_wff_expr(rdup_tv_wff_expr(e), var, LYDIA_TRUE);
    vf = build_obdd_conditioning(var + 1, cnt, ef, nodes, index);
    vt = build_obdd_conditioning(var + 1, cnt, et, nodes, index);
    rfre_tv_wff_expr(ef);
    rfre_tv_wff_expr(et);

    return add_obdd_node(nodes, index, var, vf, vt);
}

static obdd_node_list build_obdd(const_tv_wff_expr e, int *root)
{
    obdd_node_list result = new_obdd_node_list();
    obdd_node_list n, l, r;
    int n_root, l_root, r_root;

    hash_table index;
    hash_table cache;
    hash_init(&index, 2, node_hash_function, NULL);

    switch (e->tag) {
        case TAGtv_wff_e_not:
            n = build_obdd(to_tv_wff_e_not(e)->n, &n_root);
            if (n->arr[n_root]->tag == TAGobdd_terminal_node) {
                append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
                append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));
                *root = !to_obdd_terminal_node(n->arr[n_root])->value;
                break;
            }
            negate_obdd(n);
            concat_obdd_node_list(result, n);
            *root = n_root;
            break;
        case TAGtv_wff_e_or:
            l = build_obdd(to_tv_wff_e_or(e)->lhs, &l_root);
            r = build_obdd(to_tv_wff_e_or(e)->rhs, &r_root);
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));

            hash_init(&cache, 0, pair_hash_function, NULL);
            *root = combine_obdd(l, r, result, &cache, &index, l_root, r_root, TAGtv_wff_e_or);
            hash_destroy(&cache);

            rfre_obdd_node_list(l);
            rfre_obdd_node_list(r);
            break;
        case TAGtv_wff_e_and:
            l = build_obdd(to_tv_wff_e_and(e)->lhs, &l_root);
            r = build_obdd(to_tv_wff_e_and(e)->rhs, &r_root);
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));

            hash_init(&cache, 0, pair_hash_function, NULL);
            *root = combine_obdd(l, r, result, &cache, &index, l_root, r_root, TAGtv_wff_e_and);
            hash_destroy(&cache);

            rfre_obdd_node_list(l);
            rfre_obdd_node_list(r);
            break;
        case TAGtv_wff_e_equiv:
            l = build_obdd(to_tv_wff_e_equiv(e)->lhs, &l_root);
            r = build_obdd(to_tv_wff_e_equiv(e)->rhs, &r_root);
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));

            hash_init(&cache, 0, pair_hash_function, NULL);
            *root = combine_obdd(l, r, result, &cache, &index, l_root, r_root, TAGtv_wff_e_equiv);
            hash_destroy(&cache);

            rfre_obdd_node_list(l);
            rfre_obdd_node_list(r);
            break;
        case TAGtv_wff_e_impl:
            l = build_obdd(to_tv_wff_e_impl(e)->lhs, &l_root);
            r = build_obdd(to_tv_wff_e_impl(e)->rhs, &r_root);
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));

            hash_init(&cache, 0, pair_hash_function, NULL);
            *root = combine_obdd(l, r, result, &cache, &index, l_root, r_root, TAGtv_wff_e_impl);
            hash_destroy(&cache);

            rfre_obdd_node_list(l);
            rfre_obdd_node_list(r);
            break;
        case TAGtv_wff_e_var:
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));
            add_obdd_node(result, &index, to_tv_wff_e_var(e)->v, 0, 1);
            *root = 2;
            break;
        case TAGtv_wff_e_const:
            if (to_tv_wff_e_const(e)->c == LYDIA_FALSE) {
                append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
                append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));
                *root = 0;
                break;
            }
            /* to_tv_wff_e_const(e)->c == LYDIA_TRUE */
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
            append_obdd_node_list(result, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));
            *root = 1;
            break;
    }

    hash_destroy(&index);

    return result;
}

static int tv_wff_expr_to_obdd(const_variable_list variables,
                               tv_wff_expr e,
                               obdd_node_list nodes,
                               const int brute_force)
{
    int root = -1;

/* These are special cases. */
    if (e == tv_wff_exprNIL) {
        append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
        append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));
        return 1;
    }
    if (e->tag == TAGtv_wff_e_const) {
        append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
        append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));
        return to_tv_wff_e_const(e)->c;
    }

/* Add the terminal nodes. */
    if (brute_force) {
        hash_table index;

        append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_FALSE)));
        append_obdd_node_list(nodes, to_obdd_node(new_obdd_terminal_node(LYDIA_TRUE)));

        hash_init(&index, 2, node_hash_function, NULL);

        root = build_obdd_conditioning(0, variables->sz, e, nodes, &index);

        hash_destroy(&index);
    } else {
/* To Do: Constant folding in 'e'. */
        obdd_node_list result = build_obdd(e, &root);
        concat_obdd_node_list(nodes, result);
    }
    set_int_counter("nodes", nodes->sz);

    return root;
}

static tv_wff_expr flatten_expr_list(tv_wff_expr_list e)
{
    register unsigned int ix;
    tv_wff_expr result = tv_wff_exprNIL;

    if (e->sz > 0) {
        result = rdup_tv_wff_expr(e->arr[0]);
    }
    for (ix = 1; ix < e->sz; ix++) {
        result = to_tv_wff_expr(new_tv_wff_e_and(result, rdup_tv_wff_expr(e->arr[ix])));
    }
    return result;
}

int wff2obdd(const_serializable input,
             serializable *output,
             const int brute_force)
{
    register unsigned int ix;

    tv_wff_expr e;

    start_stopwatch("compilation time");
    if (input->tag == TAGtv_wff_flat_kb) {
        tv_wff input_wff = to_tv_wff(to_tv_wff_flat_kb(input)->constraints);
        *output = to_serializable(new_obdd_flat_kb(to_tv_wff_flat_kb(input)->name,
                                                   to_kb(new_obdd(rdup_values_set_list(input_wff->domains),
                                                                  rdup_variable_list(input_wff->variables),
                                                                  rdup_variable_list(input_wff->encoded_variables),
                                                                  rdup_constant_list(input_wff->constants),
                                                                  input_wff->encoding,
                                                                  new_obdd_node_list(),
                                                                  -1))));
        
        e = flatten_expr_list(input_wff->e);
        to_obdd(to_obdd_flat_kb(*output)->constraints)->root = tv_wff_expr_to_obdd(input_wff->variables,
                                                                                   e,
                                                                                   to_obdd(to_obdd_flat_kb(*output)->constraints)->nodes,
                                                                                   brute_force);
        rfre_tv_wff_expr(e);
    } else if (input->tag == TAGtv_wff_hierarchy) {
        node_list nodes = new_node_list();
        *output = to_serializable(new_obdd_hierarchy(nodes));
        for (ix = 0; ix < to_hierarchy(input)->nodes->sz; ix++) {
            tv_wff node_input = to_tv_wff(to_hierarchy(input)->nodes->arr[ix]->constraints);
            obdd node_output = new_obdd(rdup_values_set_list(node_input->domains),
                                        rdup_variable_list(node_input->variables),
                                        rdup_variable_list(node_input->encoded_variables),
                                        rdup_constant_list(node_input->constants),
                                        node_input->encoding,
                                        new_obdd_node_list(),
                                        -1);
            node result;
            e = flatten_expr_list(node_input->e);
            node_output->root = tv_wff_expr_to_obdd(node_input->variables, e, node_output->nodes, brute_force);
            rfre_tv_wff_expr(e);
            result = new_node(rdup_lydia_symbol(to_hierarchy(input)->nodes->arr[ix]->type),
                              rdup_edge_list(to_hierarchy(input)->nodes->arr[ix]->edges),
                              to_kb(node_output));
            append_node_list(nodes, result);
        }
    } else {
        assert(0);
        abort();
    }
    stop_stopwatch("compilation time");

    return 1;
}

void wff2obdd_init()
{
    stat_init();
    init_stopwatch("compilation time", "propositional Wff to OBDD conversion time: %d s %d.%d ms", "compilation");
    init_int_counter("cache hits", "cache hits: %d", "internals");
    init_int_counter("nodes", "OBDD nodes: %d", "output");
}

void wff2obdd_destroy()
{
    stat_destroy();
}
