/**
 *  \file array.c
 *  \brief Implementation of dynamic arrays.
 */

#include "array.h"
#include "qsort.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int cmp_ints(const void *a, const void *b)
{
    return *((const int *)a) - *((const int *)b);
}

/**
 * Allocate a new array.
 *
 * @param destroy an array element destructor routine;
 * @param clone an array element clone routine.
 * @returns an array handler, NULL if memory allocation error.
 */
array array_new(array_element_destroy_func_t destroy,
                array_element_clone_func_t clone)
{
    array ah = (array)malloc(sizeof(struct str_array));
    if (NULL == (void*)ah) {
        return NULL;
    }

    ah->sz = 0;
    ah->room = ARRAY_ROOM;
    ah->arr = (void **)malloc(ARRAY_ROOM * sizeof(void *));
    ah->destroy = destroy;
    ah->clone = clone;
    if (NULL == ah->arr) {
        free(ah);
        return NULL;
    }

    return ah;
}

/**
 * Deep copy an array.
 *
 * @param ah an array handler.
 * @returns an array handler, NULL if memory allocation error.
 */
array array_copy(array ah)
{
    register unsigned int ix;

    array result;

    if (NULL == ah) {
        return NULL;
    }

    result = array_new(ah->destroy, ah->clone);
    if (NULL != ah->clone) {
        for (ix = 0; ix < ah->sz; ix++) {
            array_append(result, ah->clone(ah->arr[ix]));
        }
    } else {
        for (ix = 0; ix < ah->sz; ix++) {
            array_append(result, ah->arr[ix]);
        }
    }

    return result;
}

/**
 * Free an array and its elements.
 *
 * @param ah an array handler.
 */
void array_free(array ah)
{
    register unsigned int ix;

    if (NULL == ah) {
        return;
    }

    if (NULL != ah->destroy) {
        for (ix = 0; ix < ah->sz; ix++) {
            ah->destroy(ah->arr[ix]);
        }
    }
    free(ah->arr);
    free(ah);
}

/**
 * Check if an array is empty.
 *
 * @param ah an array handler;
 * @returns 1 if the array is empty, 0 otherwise.
 */
int array_empty(array ah)
{
    return ah->sz == 0;
}

void array_clear(array ah)
{
    ah->sz = 0;
}

/**
 * Get the size of an array.
 *
 * @param ah an array handler;
 * @returns the number of elements in the array.
 */
size_t array_size(array ah)
{
    return ah->sz;
}

array array_setroom(array ah, const size_t rm)
{
    void *arr;

    if (ah->room >= rm) {
        return ah;
    }
    arr = (void **)realloc(ah->arr, rm * sizeof(void *));
    if (NULL == arr) {
        return NULL;
    }
    ah->arr = arr;
    ah->room = rm;

    return ah;
}

array array_append(array ah, void *el)
{
    if (ah->sz >= ah->room) {
        if (NULL == array_setroom(ah, 1 + 2 * ah->sz)) {
            return NULL;
        }
    }
    ah->arr[ah->sz] = el;
    ah->sz++;

    return ah;
}

array array_reverse(array ah)
{
    register unsigned int ix = 0;
    size_t other;
    void **arr, *h;

    if (ah == NULL) {
        return ah;
    }
    other = ah->sz - 1;
    arr = ah->arr;
    while (ix < other) {
        h = arr[ix];
        arr[ix] = arr[other];
        arr[other] = h;
        ix++;
        other--;
    }
    return ah;
}

array array_insert(array ah, const unsigned int pos, void *el)
{
    register size_t ix, iy = pos;
    void **arr;

    assert(ah != NULL);

    if (ah->sz >= ah->room) {
        array_setroom(ah, 1 + 2 * ah->sz);
    }
    if (iy > ah->sz) {
        iy = ah->sz;
    }
    arr = ah->arr;
    for (ix = ah->sz; ix > iy; ix--) {
        arr[ix] = arr[ix - 1];
    }
    ah->sz += 1;
    arr[iy] = el;

    return ah;
}

array array_delete(array ah, const unsigned int pos)
{
    register unsigned int ix;

    assert(ah != NULL);

    if (pos >= ah->sz) {
        return ah;
    }
    if (ah->destroy != NULL && ah->arr[pos] != NULL) {
        ah->destroy(ah->arr[pos]);
    }
    ah->sz -= 1;
    for (ix = pos; ix < ah->sz; ix++) {
        ah->arr[ix] = ah->arr[ix + 1];
    }
    return ah;
}

array array_concat(array la, array lb)
{
    size_t cnt;
    void **sp, **dp;

    assert(la != NULL);
    array_setroom(la, la->sz + lb->sz);
    cnt = lb->sz;
    sp = lb->arr;
    dp = &la->arr[la->sz];
    while (cnt != 0) {
        *dp++ = *sp++;
        cnt -= 1;
    }
    la->sz += lb->sz;
    array_free(lb);
    return la;
}

array array_int_sort(array list)
{
    if (list == NULL || list->sz < 2) {
        return list;
    }
    lydia_qsort(list->arr, list->sz, sizeof(list->arr[0]), cmp_ints);

    return list;
}

/**
 * Linear scan of an array in search for a given value.
 * Return the index of the value, or -1 if it is not in the array.
 * WARNING: If members are pointers then pointer comparison will occur.
 */
int array_index_of(array l, void *v)
{
    unsigned int i;

    for (i = 0; i < l->sz; i++) {
        if (l->arr[i] == v) {
            return i;
        }
    }

    return -1;
}


/**
 * Check if two arrays have an intersections
 * Return 1 if they intersect, of 0 otherwise.
 */
int array_intersects(array a, array b)
{
    int i;

    for (i = 0; i < a->sz; i++) {
        if (array_contains(b, a->arr[i])) {
            return 1;
        }
    }

    return 0;
}

/**
 * Check if an item is in the array. Return 1 if it is in the array,
 * or zero otherwise.
 */
int array_contains(array l, void *v)
{
    if (array_index_of(l, v) == -1) {
        return 0;
    }

    return 1;
}


/**
 * Binary search for an int in a sorted array of ints.
 */
int array_sorted_int_member(array l, int v)
{
    int b, e, m;
    if (l->sz == 0) {
        return 0;
    }
    b = 0;
    e = l->sz - 1;
    while (b <= e) {
        m = (b + e) / 2;
        if (v > (int)(l->arr[m])) {
            b = m + 1;
        } else if (v < (int)(l->arr[m])) {
            e = m - 1;
        } else {
            return 1;
        }
    }
    return 0;
}

/**
 * Insert a value into a sorted array.
 */
array array_sorted_insert(array ah, void *value)
{
    register size_t ix;
    void **arr;

    assert(ah != NULL);

    if (ah->sz >= ah->room) {
        array_setroom(ah, 1 + 2 * ah->sz);
    }

    arr = ah->arr;
    ix = ah->sz;
    while (ix > 0) {
        if (value < arr[ix - 1]) {
            arr[ix] = arr[ix - 1];
            ix = ix-1;
        } else {
            break;
        }

    }
    arr[ix] = value;

    ah->sz += 1;

    return ah;
}



/**
 * Prints an array to a file
 */
void pp_int_array(FILE *outfile, array array_to_print)
{
    int i;

    if (array_to_print->sz == 0) {
        fprintf(outfile, "[]");
        return;
    } else {
        fprintf(outfile, "[%d", POINTER_TO_INT(array_to_print->arr[0]));
    }

    for (i=1; i < array_to_print->sz; i++) {
        fprintf(outfile, ",%d", POINTER_TO_INT(array_to_print->arr[i]));
    }

    fprintf(outfile,"]");
}
