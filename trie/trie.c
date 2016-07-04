/**
 *  \file trie.c
 *  \brief Implementation of tries.
 */

#include "defs.h"
#include "trie.h"
#include "qsort.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


/**
 * Print all the values in the trie
 */
void debug_trie_values_int_array(FILE* outfile, trie a_trie)
{
	array values = trie_get_all_values(a_trie);
	unsigned int i;

	fprintf(outfile, "Trie values:\n");
	for(i=0;i<values->sz;i++){
		pp_int_array(outfile, values->arr[i]);
		fprintf(outfile, "\n");
	}
}


/**
 * @todo This function will become obsolete once we remove the trie
 * pointer from the node structure.
 */
static void trie_node_set_trie(trie_node node, trie trie)
{
    register unsigned int ix;

    node->trie_ = trie;

    if (NULL == node->edges) {
        assert(NULL == node->kids);
        return;
    }

    assert(node->edges->sz == node->kids->sz);

    for (ix = 0; ix < node->edges->sz; ix++) {
        trie_node_set_trie(node->kids->arr[ix], trie);
    }
}

static void trie_node_set_deleted(trie trie,
                                  trie_node node,
                                  signed char is_deleted)
{
    trie->deleted_nodes += is_deleted - node->is_deleted;
    node->is_deleted = is_deleted;
}

static trie_node trie_node_new(trie trie)
{
    trie_node node = (trie_node)malloc(sizeof(struct str_trie_node));
    if (NULL == node) {
        return node;
    }

    node->trie_ = trie;
    node->edges = NULL;
    node->kids = NULL;
    node->is_terminal = 0;
    node->is_deleted = 0;
    node->value = NULL;

    node->trie_->node_count += 1;

    return node;
}

static trie_node trie_node_copy(trie_node node)
{
    trie_node result = (trie_node)malloc(sizeof(struct str_trie_node));
    if (NULL == result) {
        return result;
    }

    result->edges = array_copy(node->edges);
    result->kids = array_copy(node->kids);
    result->trie_ = node->trie_;
    result->is_terminal = node->is_terminal;
    result->is_deleted = node->is_deleted;
    result->value = (node->trie_->clone != NULL && node->value != NULL ? node->trie_->clone(node->value) : node->value);

    result->trie_->node_count += 1;

    return result;
}

static void trie_node_free(trie_node node)
{
    node->trie_->node_count -= 1;

    if (NULL != node->edges) {
        array_free(node->edges);
    }
    if (NULL != node->kids) {
        array_free(node->kids);
    }
    if (NULL != node->trie_->destroy && node->value != NULL && node->is_terminal) {
        node->trie_->destroy(node->value);
    }
    free(node);
}

/**
 * Allocate a new trie.
 *
 * @param destroy a trie value destructor routine;
 * @param clone a trie value clone routine.
 * @returns a trie pointer, NULL if memory allocation error.
 */
trie trie_new(trie_node_destroy_func_t destroy, trie_node_clone_func_t clone)
{
    trie result = (trie)malloc(sizeof(struct str_trie));
    if (NULL == result) {
        return result;
    }

    result->node_count = 0;
    result->deleted_nodes = 0;
    result->destroy = destroy;
    result->clone = clone;
    result->root = trie_node_new(result);
    if (NULL == result->root) {
        free(result);
        return NULL;
    }
    trie_node_set_deleted(result, result->root, 1);

    return result;
}

/**
 * Deep copy of a trie.
 *
 * @param t a trie.
 * @returns an trie pointer, NULL if memory allocation error.
 */
trie trie_copy(trie t)
{
    trie result = (trie)malloc(sizeof(struct str_trie));
    if (NULL == result) {
        return result;
    }

    result->root = trie_node_copy(t->root);
    result->destroy = t->destroy;
    result->clone = t->clone;

    result->node_count = t->node_count;
    result->deleted_nodes = t->deleted_nodes;

    trie_node_set_trie(result->root, result);

    return result;
}

void trie_free(trie t)
{
    trie_node_free(t->root);
    free(t);
}

static signed char trie_remove_subsumed_internal(trie_node node,
                                                 array key,
                                                 unsigned int pos)
{
    register unsigned int ix;

    signed char f = 1;

    if (node->is_deleted) {
        return 1;
    }
    if (pos == key->sz) {
        trie_node_set_deleted(node->trie_, node, 1);
        if (NULL != node->kids) {
            for (ix = 0; ix < node->edges->sz; ix++) {
                trie_remove_subsumed_internal(node->kids->arr[ix], key, pos);
            }
        }
        return 1;
    }
    if (NULL == node->edges) {
        return 0;
    }
/**
 *  @bug The regression test of "squeezy" triggers the assertion below.
 */
/*
    assert(node->edges->sz > 0);
*/
    assert(node->edges->sz == node->kids->sz);

    for (ix = 0; ix < node->edges->sz; ix++) {
        if (node->edges->arr[ix] < key->arr[pos]) {
            f &= trie_remove_subsumed_internal(node->kids->arr[ix],
                                               key,
                                               pos);
        }
        if (node->edges->arr[ix] == key->arr[pos]) {
            f &= trie_remove_subsumed_internal(node->kids->arr[ix],
                                               key,
                                               pos + 1);
        }
        if (node->edges->arr[ix] > key->arr[pos]) {
            f = 0;
        }
    }

    f &= !node->is_terminal;
    if (f) {
        trie_node_set_deleted(node->trie_, node, 1);
    }

    return f;
}

static signed char trie_add_internal(trie trie,
                                     trie_node node,
                                     array key,
                                     unsigned int pos,
                                     void *value)
{
/** @bug Fix the memory allocation error handling. */
    register unsigned int ix;
    register unsigned int iy;

    trie_node kid;

    if (pos == key->sz) {
        if (node->value != NULL) {
            assert(NULL != node->trie_->destroy);
            node->trie_->destroy(node->value);
        }
        node->value = value;
        node->is_terminal = 1;

        trie_node_set_deleted(trie, node, 0);

        return 1;
    }

    trie_node_set_deleted(trie, node, 0);

    if (NULL == node->edges) {
        node->edges = array_new(NULL, NULL);
    }
    if (NULL == node->kids) {
        node->kids = array_new((array_element_destroy_func_t)trie_node_free,
                               (array_element_clone_func_t)trie_node_copy);
    }

    for (ix = 0; ix < node->kids->sz; ix++) {
        if (node->edges->arr[ix] == key->arr[pos]) {
            trie_add_internal(trie,
                              (trie_node)node->kids->arr[ix],
                              key,
                              pos + 1,
                              value);
            return 1;
        }
        if (node->edges->arr[ix] > key->arr[pos]) {
            break;
        }
    }

    kid = trie_node_new(trie);
    if (NULL == kid) {
        return 0;
    }
    kid->is_terminal = 1;
    kid->value = value;

    for (iy = key->sz - 1; iy > pos; iy--) {
        trie_node parent = kid;
        kid = trie_node_new(trie);
        if (NULL == kid) {
            return 0;
        }
        kid->edges = array_new(NULL, NULL);
        kid->kids = array_new((array_element_destroy_func_t)trie_node_free,
                              (array_element_clone_func_t)trie_node_copy);
        array_append(kid->edges, key->arr[iy]);
        array_append(kid->kids, parent);
    }
    array_insert(node->edges, ix, key->arr[pos]);
    array_insert(node->kids, ix, kid);

    return 1;
}

static signed char trie_merge_right_siblings(trie_node dst,
                                             trie_node src,
                                             unsigned int start,
                                             signed char inclusive)
{
/** @bug Fix the memory allocation error handling. */
    register unsigned int ix, iy = 0;
    
    if (NULL == src->edges) {
        return 1;
    }
    if (NULL == dst->edges) {
        dst->edges = array_new(NULL, NULL);
        dst->kids = array_new((array_element_destroy_func_t)trie_node_free,
                              (array_element_clone_func_t)trie_node_copy);
    }

    for (ix = start + 1 - inclusive; ix < src->edges->sz; ix++) {
        while (iy < dst->edges->sz &&
               dst->edges->arr[iy] < src->edges->arr[ix]) {
            iy += 1;
        }
        array_insert(dst->edges, iy, src->edges->arr[ix]);
        array_insert(dst->kids, iy, src->kids->arr[ix]);
    }
    src->edges->sz = start + 1;
    src->kids->sz = start + 1;
/** @bug Update the node counter in the trie. */

    return 1;
}

static signed char trie_multiply_internal(trie trie,
                                          trie_node node,
                                          array key,
                                          unsigned int pos,
                                          void *value)
{
/** @bug We have messed with the combination of values. */
/** @bug Fix the memory allocation error handling. */
    register unsigned int ix;

    if (pos == key->sz || node->is_deleted) {
        return 1;
    }

    if (NULL == node->edges) {
        node->edges = array_new(NULL, NULL);
        node->kids = array_new((array_element_destroy_func_t)trie_node_free,
                               (array_element_clone_func_t)trie_node_copy);
    }
    if (node->edges->sz == 0) {
        trie_node new_node = trie_node_new(trie);

        if (pos + 1 == key->sz) {
            new_node->is_terminal = 1;
            new_node->value = trie->clone == NULL ? NULL : trie->clone(value);
        }

        array_append(node->edges, (void *)key->arr[pos]);
        array_append(node->kids, new_node);

        trie_multiply_internal(trie,
                               new_node,
                               key,
                               pos + 1,
                               value);
    }
    for (ix = 0; ix < node->edges->sz; ix++) {
        trie_node kid_node = (trie_node)node->kids->arr[ix];
        if (node->edges->arr[ix] < key->arr[pos]) {
            if (node->is_terminal) {
                kid_node->is_terminal = node->is_terminal;
                kid_node->value = node->value;

                node->is_terminal = 0;
                node->value = NULL;
            }
            trie_multiply_internal(trie,
                                   kid_node,
                                   key,
                                   pos,
                                   value);
            continue;
        }
        if (node->edges->arr[ix] == key->arr[pos]) {
            if (kid_node->is_deleted) {
                trie_node_set_deleted(trie, kid_node, 0);
            }
            if (node->is_terminal) {
                kid_node->is_terminal = node->is_terminal;
                kid_node->value = node->value;

                node->is_terminal = 0;
                node->value = NULL;
            }

            trie_merge_right_siblings(kid_node, node, ix, 0);
            trie_multiply_internal(trie,
                                   kid_node,
                                   key,
                                   pos + 1,
                                   value);
            continue;
        }
        if (node->edges->arr[ix] > key->arr[pos]) {
            trie_node new_node;

            new_node = trie_node_new(trie);

            new_node->edges = array_new(NULL, NULL);
            new_node->kids = array_new((array_element_destroy_func_t)trie_node_free,
                                       (array_element_clone_func_t)trie_node_copy);

            trie_merge_right_siblings(new_node, node, ix, 1);
            node->edges->arr[ix] = (void *)key->arr[pos];
            node->kids->arr[ix] = new_node;

            trie_multiply_internal(trie,
                                   new_node,
                                   key,
                                   pos + 1,
                                   value);
        }
    }

    return 1;
}

static signed char trie_add_trie_internal(trie_node l, trie_node r)
{
/** @bug Fix the memory allocation error handling. */
    register unsigned int ix;
    register unsigned int iy;

    unsigned int new_node_id;
    trie_node new_node;

    assert(!r->is_deleted);

    trie_node_set_deleted(l->trie_, l, 0);
    if (r->is_terminal) {
/**
 * @bug The logic here is faulty. In this implementation we assume
 * that the two tries contain no pairs of equal keys. If they do, we
 * trigger an assertion, instead we should combine the two values in
 * the resulting trie with the help of a user-defined function.
 */
        assert(!l->is_terminal);
        l->is_terminal = 1;

        l->value = (l->trie_->clone != NULL && r->value != NULL ?
                        l->trie_->clone(r->value) :
                        r->value);
    }

    if (NULL == r->edges) {
        return 1;
    }
    if (NULL == l->edges) {
        assert(NULL == l->kids);

        l->edges = array_new(NULL, NULL);
        l->kids = array_new((array_element_destroy_func_t)trie_node_free,
                            (array_element_clone_func_t)trie_node_copy);
    }

    iy = 0;
    for (ix = 0; ix < l->edges->sz; ix++) {
        while (iy < r->edges->sz && r->edges->arr[iy] < l->edges->arr[ix]) {
            if (!((trie_node)r->kids->arr[iy])->is_deleted) {
                new_node_id = p2ui(r->edges->arr[iy]);
                new_node = trie_node_copy((trie_node)r->kids->arr[iy]);
                trie_node_set_trie(new_node, l->trie_);

                array_insert(l->edges, ix, ui2p(new_node_id));
                array_insert(l->kids, ix, new_node);
            }

            iy += 1;
        }
        if (iy < r->edges->sz && r->edges->arr[iy] == l->edges->arr[ix]) {
            if (!((trie_node)r->kids->arr[iy])->is_deleted) {
                trie_add_trie_internal((trie_node)l->kids->arr[ix],
                                       (trie_node)r->kids->arr[iy]);
            }

            iy += 1;
        }
    }

    for (; iy < r->edges->sz; iy++) {
        assert((l->edges->sz == 0) || (r->edges->arr[iy] > l->edges->arr[l->edges->sz - 1]));

        if (!((trie_node)r->kids->arr[iy])->is_deleted) {
            new_node_id = p2ui(r->edges->arr[iy]);
            new_node = trie_node_copy((trie_node)r->kids->arr[iy]);

            trie_node_set_trie(new_node, l->trie_);

            array_append(l->edges, ui2p(new_node_id));
            array_append(l->kids, new_node);
        }
    }

    return 1;
}

static signed char trie_is_subsumed_internal(trie_node node,
                                             array key,
                                             unsigned int pos)
{
    register unsigned int nx = 0;

    if (node->is_deleted) {
        return 0;
    }
    if (node->edges == NULL || node->edges->sz == 0) {
        return node->is_terminal;
    }
        
    while (nx < node->kids->sz) {
        while (pos < key->sz && key->arr[pos] < node->edges->arr[nx]) {
            pos += 1;
        }
        if (pos == key->sz) {
            return 0;
        }
        while (pos < key->sz && key->arr[pos] <= node->edges->arr[nx]) {
            if (node->edges->arr[nx] == key->arr[pos]) {
                if (trie_is_subsumed_internal(node->kids->arr[nx],
                                              key,
                                              pos + 1)) {
                    return 1;
                }
            }
            pos += 1;
        }
        nx += 1;
    }

    return 0;
}

static signed char trie_remove_subsumed_trie_internal(trie_node l, trie_node r)
{
    register unsigned int ix;
    register unsigned int iy;

    signed char g = 0;

    assert(!l->is_deleted);
    assert(!r->is_deleted);

    if (r->is_terminal) {
        if (NULL != l->edges) {
            assert(NULL != l->kids);
            assert(l->edges->sz == l->kids->sz);

            for (ix = 0; ix < l->edges->sz; ix++) {
                trie_node l_kid = l->kids->arr[ix];

                if (!l_kid->is_deleted) {
                    trie_remove_subsumed_trie_internal(l_kid, r);
                }
            }
        }
        trie_node_set_deleted(l->trie_, l, 1);
        return 1;
    }

    if (NULL == l->edges || NULL == r->edges) {
        return 0;
    }

    assert(l->kids != NULL);
    assert(r->kids != NULL);
    assert(l->edges->sz == l->kids->sz);
    assert(r->edges->sz == r->kids->sz);

    for (ix = 0; ix < r->edges->sz; ix++) {
        unsigned int r_kid_id = p2ui(r->edges->arr[ix]);
        trie_node r_kid = r->kids->arr[ix];

        signed char f = 1;

        if (r_kid->is_deleted) {
            continue;
        }

        for (iy = 0; iy < l->edges->sz; iy++) {
            unsigned int l_kid_id = p2ui(l->edges->arr[iy]);
            trie_node l_kid = l->kids->arr[iy];

            if (l_kid->is_deleted) {
                continue;
            }

            if (l_kid_id < r_kid_id) {
                f &= trie_remove_subsumed_trie_internal(l_kid, r);
            }
            if (l_kid_id == r_kid_id) {
                f &= trie_remove_subsumed_trie_internal(l_kid, r_kid);
            }
            if (l_kid_id > r_kid_id) {
                f = 0;
            }
        }
        f &= !l->is_terminal;
        if (f) {
            trie_node_set_deleted(l->trie_, l, 1);
        }
        g |= f;
    }

    return g;
}

static signed char trie_remove_not_subsumed_trie_internal(trie_node l,
                                                          trie_node r)
{
    register unsigned int ix;
    register unsigned int iy;

    signed char g = 0;

    assert(!l->is_deleted);
    assert(!r->is_deleted);

    if (r->is_terminal) {
        if (NULL != l->edges) {
            assert(NULL != l->kids);
            assert(l->edges->sz == l->kids->sz);

            for (ix = 0; ix < l->edges->sz; ix++) {
                trie_node l_kid = l->kids->arr[ix];

                if (!l_kid->is_deleted) {
                    trie_remove_not_subsumed_trie_internal(l_kid, r);
                }
            }
        }
        l->is_deleted = 2;
        return 1;
    }

    if (NULL == l->edges || NULL == r->edges) {
        return 0;
    }

    assert(l->kids != NULL);
    assert(r->kids != NULL);
    assert(l->edges->sz == l->kids->sz);
    assert(r->edges->sz == r->kids->sz);

    for (ix = 0; ix < r->edges->sz; ix++) {
        unsigned int r_kid_id = p2ui(r->edges->arr[ix]);
        trie_node r_kid = r->kids->arr[ix];

        signed char f = 1;

        if (r_kid->is_deleted) {
            continue;
        }

        for (iy = 0; iy < l->edges->sz; iy++) {
            unsigned int l_kid_id = p2ui(l->edges->arr[iy]);
            trie_node l_kid = l->kids->arr[iy];

            if (l_kid->is_deleted) {
                continue;
            }

            if (l_kid_id < r_kid_id) {
                f &= trie_remove_not_subsumed_trie_internal(l_kid, r);
            }
            if (l_kid_id == r_kid_id) {
                f &= trie_remove_not_subsumed_trie_internal(l_kid, r_kid);
            }
            if (l_kid_id > r_kid_id) {
                f = 0;
            }
        }
        f &= !l->is_terminal;
        if (f) {
            l->is_deleted = 2;
        }
        g |= f;
    }

    return g;
}

static signed char trie_remove_not_subsumed_delete_marked(trie_node node)
{
    register unsigned int ix;

    signed char f = 1;

    if (NULL == node->edges) {
        assert(NULL == node->kids);

        if (node->is_deleted == 2) {
            node->is_deleted = 0;
            return 0;
        }

        node->is_deleted = 1;
        return 1;
    }

    assert(node->edges->sz == node->kids->sz);

    for (ix = 0; ix < node->edges->sz; ix++) {
        trie_node l_kid = (trie_node)node->kids->arr[ix];

        f &= trie_remove_not_subsumed_delete_marked(l_kid);
    }
    node->is_deleted = f;

    return f;
}


signed char trie_multiply_trie_internal(unsigned int l_id,
                                        trie_node l,
                                        unsigned int r_id,
                                        trie_node r,
                                        array buffer,
                                        trie result,
                                        trie_terminal_operator_func_t terminal_operator)
{
/** @bug Fix the memory allocation error handling. */
    register unsigned int ix;
    register unsigned int iy;

    if (l->is_terminal && r->is_terminal) {
        array key = array_copy(buffer);

        array_int_sort(key);
        for (ix = 1; ix < key->sz; ix++) {
            if (key->arr[ix] == key->arr[ix - 1]) {
                array_delete(key, ix);
                ix -= 1;
            }
        }
        trie_remove_subsumed(result, key);
        if (!trie_is_subsumed(result, key)) {
            trie_add(result,
                     key,
                     terminal_operator == NULL ?
                         NULL :
                         terminal_operator(l->value, r->value));
        }

        array_free(key);

        return 1;
    }
    if (l->is_terminal) {
        assert(r->edges != NULL && r->edges->sz > 0);

        for (ix = 0; ix < r->edges->sz; ix++) {
            unsigned int r_kid_id = p2ui(r->edges->arr[ix]);
            trie_node r_kid = r->kids->arr[ix];

            if (r_kid->is_deleted) {
                continue;
            }

            array_append(buffer, ui2p(r_kid_id));
            trie_multiply_trie_internal(l_id,
                                        l,
                                        r_kid_id,
                                        r_kid,
                                        buffer,
                                        result,
                                        terminal_operator);
            array_delete(buffer, buffer->sz - 1);
        }

        return 1;
    }

    if (r->is_terminal) {
        assert(l->edges != NULL && l->edges->sz > 0);

        for (ix = 0; ix < l->edges->sz; ix++) {
            unsigned int l_kid_id = p2ui(l->edges->arr[ix]);
            trie_node l_kid = l->kids->arr[ix];

            if (l_kid->is_deleted) {
                continue;
            }

            array_append(buffer, ui2p(l_kid_id));
            trie_multiply_trie_internal(l_kid_id,
                                        l_kid,
                                        r_id, r,
                                        buffer,
                                        result,
                                        terminal_operator);
            array_delete(buffer, buffer->sz - 1);
        }

        return 1;
    }

    assert(l->edges != NULL && l->edges->sz > 0);
    assert(r->edges != NULL && r->edges->sz > 0);

    for (ix = 0; ix < l->edges->sz; ix++) {
        unsigned int l_kid_id = p2ui(l->edges->arr[ix]);
        trie_node l_kid = l->kids->arr[ix];

        if (l_kid->is_deleted) {
            continue;
        }

        for (iy = 0; iy < r->edges->sz; iy++) {
            unsigned int r_kid_id = p2ui(r->edges->arr[iy]);
            trie_node r_kid = r->kids->arr[iy];

            if (r_kid->is_deleted) {
                continue;
            }

            array_append(buffer, ui2p(l_kid_id));
            array_append(buffer, ui2p(r_kid_id));
            trie_multiply_trie_internal(l_kid_id,
                                        l_kid,
                                        r_kid_id,
                                        r_kid,
                                        buffer,
                                        result,
                                        terminal_operator);
            array_delete(buffer, buffer->sz - 1);
            array_delete(buffer, buffer->sz - 1);
        }
    }

    return 1;
}

static signed char trie_gc_internal(trie_node node)
{
    register unsigned int ix;

    signed char g, f = node->is_deleted;

    if (NULL == node->kids || 0 == node->kids->sz) {
        return f;
    }
    for (ix = 0; ix < node->kids->sz; ix++) {
        if ((g = trie_gc_internal(node->kids->arr[ix]))) {
            array_delete(node->kids, ix);
            array_delete(node->edges, ix);
            ix -= 1;                    
        }
        f &= g;
    }
    return f;
}

static void trie_nodes_visit_internal(trie_node node,
                                      trie_node_visit_func_t visitor,
                                      void *ctx)
{
    register unsigned int ix;

    if (node->is_deleted) {
        return;
    }
    if (node->is_terminal && node->value != NULL) {
        visitor(ctx, node->value);
    }
    if (node->kids == NULL) {
        return;
    }
    for (ix = 0; ix < node->kids->sz; ix++) {
        trie_nodes_visit_internal(node->kids->arr[ix], visitor, ctx);
    }
}

static void trie_get_all_values_internal(array result, trie_node node)
{
    register unsigned int ix;

    if (node->is_deleted) {
        return;
    }
    if (node->is_terminal && node->value != NULL) {
        array_append(result, node->value);
    }
    if (node->kids == NULL) {
        return;
    }
    for (ix = 0; ix < node->kids->sz; ix++) {
        trie_get_all_values_internal(result, node->kids->arr[ix]);
    }
}

/**
 * Adds a key/value pair to l.
 *
 * @param t a trie;
 * @param key a key;
 * @param value a value.
 * @returns 0 in case of a memory allocation error.
 */
signed char trie_add(trie t, array key, void *value)
{
    return trie_add_internal(t, t->root, key, 0, value);
}

/**
 * Checks if a key is subsumed by a trie (or if the trie subsumes the
 * key). Note that the subsumption is non-strict, i.e., the function
 * returns 1 if the key is contained in the trie.
 *
 * @param t a trie;
 * @param key a key to check for.
 * @returns 1 iff the key is a (non-strict) superset of any of the
 * keys in t.
 */
signed char trie_is_subsumed(trie t, array key)
{
    return trie_is_subsumed_internal(t->root, key, 0);
}

static signed char trie_is_subsumed_internal_fixed(trie_node node,
                                             array key,
                                             unsigned int pos)
{
    register unsigned int nx = 0;
    int has_undeleted_kid;

    if (node->is_deleted) {
        return 0;
    }
    if (node->edges == NULL || node->edges->sz == 0) {
        return node->is_terminal;
    }

    /** @bugfix: if a node has only deleted kids, it is a terminal node. **/
    has_undeleted_kid=0;
    for(nx=0;nx<node->kids->sz;nx++){
		if(((trie_node)(node->kids->arr[nx]))->is_deleted==0){
			has_undeleted_kid=1;
			break;
		}
	}
	if(has_undeleted_kid==0){
		return 1;
	}

    while (nx < node->kids->sz) {
        while (pos < key->sz && key->arr[pos] < node->edges->arr[nx]) {
            pos += 1;
        }
        if (pos == key->sz) {
            return 0;
        }
        while (pos < key->sz && key->arr[pos] <= node->edges->arr[nx]) {
            if (node->edges->arr[nx] == key->arr[pos]) {
                if (trie_is_subsumed_internal_fixed(node->kids->arr[nx],
                                              key,
                                              pos + 1)) {
                    return 1;
                }
            }
            pos += 1;
        }
        nx += 1;
    }

    return 0;
}

/**
 * TODO: Either replace trie_is_subsumed() or choose a different name.
 *
 * Checks if a key is subsumed by a trie (or if the trie subsumes the
 * key). Note that the subsumption is non-strict, i.e., the function
 * returns 1 if the key is contained in the trie.
 *
 * @param t a trie;
 * @param key a key to check for.
 * @returns 1 iff the key is a (non-strict) superset of any of the
 * keys in t.
 */
signed char trie_is_subsumed_fixed(trie t, array key){
	return trie_is_subsumed_internal_fixed(t->root, key, 0);
}

/**
 * Marks for deletion nodes in t with keys subsumed by key. Note that
 * the deletion is non-strict, i.e., nodes with keys equal to key will
 * be deleted.
 *
 * @param t a trie;
 * @param key a key.
 */
void trie_remove_subsumed(trie t, array key)
{
    trie_remove_subsumed_internal(t->root, key, 0);
}

/**
 * Combines each key in a trie with a key.
 *
 * @param t a trie;
 * @param key a key;
 * @param value a value;
 * @returns 0 in case of a memory allocation error.
 */
signed char trie_multiply(trie t, array key, void *value)
{
    if ((!t->root->is_terminal &&
         (t->root->edges == NULL || t->root->edges->sz == 0))) {
/** @todo Check if the root node is deleted instead. */
        return 1;
    }
    return trie_multiply_internal(t, t->root, key, 0, value);
}

/**
 * Marks for deletion nodes in l belonging to keys subsumed by any key
 * in r. Note that the deletion is non-strict, i.e., nodes in l
 * belonging to a key equal to a key in r will be deleted.
 *
 * @param l a trie;
 * @param r a trie.
 */
void trie_remove_subsumed_trie(trie l, trie r)
{
    if (!l->root->is_deleted && !r->root->is_deleted) {
        trie_remove_subsumed_trie_internal(l->root, r->root);
    }
}

/**
 * Marks for deletion nodes in l belonging to keys not subsumed by any
 * key in r. Nodes in l belonging to a key equal to a key in r will
 * not be deleted.
 *
 * @param l a trie;
 * @param r a trie.
 */
void trie_remove_not_subsumed_trie(trie l, trie r)
{
/**
 * @todo The current implementation traverses the trie twice. The
 * first traversal is the same as in "trie_remove_subsumed_trie",
 * except that nodes belonging to keys in "l" which are subsumed by
 * keys in "r" instead of being deleted are marked with an internal
 * flag. In the second phase all non-deleted nodes are marked for
 * deletion, all nodes marked in the first phase are unmarked and all
 * nodes having deleted ancestors only are marked for deletion.  The
 * question is if this optimal or it can be done in one traversal
 * only.
 */
    if (!l->root->is_deleted && !r->root->is_deleted) {
        trie_remove_not_subsumed_trie_internal(l->root, r->root);
        trie_remove_not_subsumed_delete_marked(l->root);
    }
}

/**
 * Adds all keys/values in r to l.
 *
 * @param l a trie;
 * @param r a trie.
 * @returns 0 in case of a memory allocation error.
 */
signed char trie_add_trie(trie l, trie r)
{
    if (r->root->is_deleted) {
        return 1;
    }

    return trie_add_trie_internal(l->root, r->root);
}

/**
 * Replaces the keys in l with the Cartesian product of the keys in l
 * and r. The result will contain only keys which are not subsumed by
 * any other key. The values of the new keys are computed with the
 * help of a user-defined function.
 *
 * @param l a trie;
 * @param r a trie;
 * @param combine a pointer to a function which combines two a value
 * from the left trie with a value from the right trie.
 * @returns 0 in case of a memory allocation error.
 */
signed char trie_multiply_trie(trie l,
                               trie r,
                               trie_terminal_operator_func_t combine)
{
/**
 * @todo This implementation is extremely inefficient. At present we
 * simply double recurse the two tries, and whenever two leaf nodes
 * are reached we add the combined key in a new trie checking for
 * subsumptions. This function is heavily used by the ATMS and it
 * should be rewritten. As a first step we can improve the performance
 * by collecting and storing the new keys in an array, sorting the
 * array and not calling "trie_remove_subsumed".
 */
/** @bug Fix the memory allocation error handling. */
    array buffer;
    trie result;

    if ((l->root->edges == NULL && !l->root->is_terminal) ||
        (r->root->edges == NULL && !r->root->is_terminal)) {

        trie_node_free(l->root);

        l->root = trie_node_new(l);
        l->node_count = 1;
        l->deleted_nodes = 0;

        return 1;
    }

    buffer = array_new(NULL, NULL);
    result = trie_new(l->destroy, l->clone);

    trie_multiply_trie_internal(0,
                                l->root,
                                0,
                                r->root,
                                buffer,
                                result,
                                combine);

    array_free(buffer);

    trie_node_free(l->root);

    l->root = result->root;
    l->node_count = result->node_count;
    l->deleted_nodes = result->deleted_nodes;

    trie_node_set_trie(l->root, l);

    free(result);

    return 1;
}

/**
 * Checks if a trie is empty, i.e., contains no keys. Of course, the
 * empty trie is different from a trie containing the empty key only.
 *
 * @param t a trie.
 * @returns 1 iff a trie contains deleted nodes only.
 */
signed char trie_is_empty(trie t)
{
/** @bug Remove the next conditional and fix the bug. */
    if (!t->root->is_terminal && t->root->edges == NULL) {
        return 1;
    }

    return t->root->is_deleted;
}

/**
 * Frees the memory of all nodes in t marked for deletion and compacts
 * the trie.
 *
 * @param t a trie.
 */
void trie_gc(trie t)
{
    trie_gc_internal(t->root);
}

/**
 * Traverses a trie and calls a user-defined function for each terminal node.
 *
 * @param t a trie;
 * @param visitor a function to be called for each terminal node;
 * @param ctx a pointer to be passed as the first argument of the
 * user-defined visitor function.
 */
void trie_nodes_visit(trie t, trie_node_visit_func_t visitor, void *ctx)
{
    trie_nodes_visit_internal(t->root, visitor, ctx);
}

array trie_get_all_values(trie t)
{
    array result = array_new(NULL, NULL);

    trie_get_all_values_internal(result, t->root);

    return result;
}


/**
 * Checks if there is an element in the trie that is subsumed by the given key.
 *
 * @param t a trie;
 * @param key a key to check for.
 * @returns 1 iff tthere is an element in the trie that is subsumed by the given key or 0 otherwise.
 */
int trie_element_subsumed_by_key(trie t, array key)
{
    return trie_element_subsumed_by_key_internal(t->root, key, 0);
}

int trie_element_subsumed_by_key_internal(trie_node node,
                                             array key,
                                             unsigned int pos)
{
    register unsigned int nx = 0;
    int rc;

    if (node->is_deleted) {
        return 0;
    }
    if (node->edges == NULL || node->edges->sz == 0) {
        return 0;
    }


    for(nx=0;nx<node->edges->sz;nx++){
    	if(key->arr[pos] < node->edges->arr[nx])
    		return 0;

    	/* If key matched with the current child of node - recurse */
    	if(key->arr[pos] == node->edges->arr[nx]){
    		if(pos+1==key->sz)
				return 1;
			else
				return trie_element_subsumed_by_key_internal(node->kids->arr[nx],key,pos+1);
    	}

    	/* If passed the key value, maybe the current child of node roots a trie that contains a subsumed element */
    	if(key->arr[pos] > node->edges->arr[nx]){
    		rc = trie_element_subsumed_by_key_internal(node->kids->arr[nx],key,pos);
			if (rc==1)
				return 1;
    	}

    	/* Only continue if found an exact match for key[pos] */

    }
    return 0;
}

/**
 * Checks if the given key is a key in the trie.
 *
 * @param t a trie;
 * @param key a key to check for.
 * @returns 1 iff tthere is key is already a key in the trie.
 */
int trie_contains_key(trie t, array key)
{
    return trie_contains_key_internal(t->root, key, 0);
}

int trie_contains_key_internal(trie_node node,
                                             array key,
                                             unsigned int pos)
{
    register unsigned int nx = 0;
    int rc;

    if (node->edges == NULL || node->edges->sz == 0) {
        return pos==key->sz;
    }


    for(nx=0;nx<node->edges->sz;nx++){
    	if(key->arr[pos] < node->edges->arr[nx])
    		return 0;

    	/* If key matched with the current child of node - recurse */
    	if(key->arr[pos] == node->edges->arr[nx]){
    		if(pos+1==key->sz){
				if(((trie_node)node->kids->arr[nx])->is_terminal)
					return 1;
				else
					return 0;
    		}
			else{
				return trie_contains_key_internal(node->kids->arr[nx],key,pos+1);
			}
    	}
    }
    return 0;
}
