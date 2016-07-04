#include <stdlib.h>

#include "qsort.h"
#include "sorted_int_list.h"

int is_sorted_term_consistent(int_list pos, int_list neg)
{
    register unsigned int iw = 0, iz;
    for (iz = 0; iz < pos->sz; iz++) {
        while (iw < neg->sz && neg->arr[iw] < pos->arr[iz]) {
            iw += 1;
        }
        if (iw == neg->sz) {
            return 1;
        }
        if (neg->arr[iw] == pos->arr[iz]) {
            return 0;
        }
    }
    return 1;
}

static int cmp_ints(const void *a, const void *b)
{
    return *((const int *)a) - *((const int *)b);
}

int_list sort_int_list(int_list ints)
{
    if (ints == int_listNIL || ints->sz < 2) {
        return ints;
    }
    lydia_qsort(ints->arr, ints->sz, sizeof(ints->arr[0]), cmp_ints);
    return ints;
}

int search_sorted_int_list(int_list l, int v, unsigned int *pos)
{
    int b, e, m;
    if (l->sz == 0) {
        return 0;
    }
    b = 0;
    e = l->sz - 1;
    while (b <= e) {
        m = (b + e) / 2;
        if (v > l->arr[m]) {
            b = m + 1;
        } else if (v < l->arr[m]) {
            e = m - 1;
        } else {
            *pos = m;
            return 1;
        }
    }
    return 0;
}

int member_sorted_int_list(const_int_list l, int v)
{
    int b, e, m;
    if (l->sz == 0) {
        return 0;
    }
    b = 0;
    e = l->sz - 1;
    while (b <= e) {
        m = (b + e) / 2;
        if (v > l->arr[m]) {
            b = m + 1;
        } else if (v < l->arr[m]) {
            e = m - 1;
        } else {
            return 1;
        }
    }
    return 0;
}

int_list merge_sorted_int_list(int_list x, int_list y)
{
    register unsigned int ix, iy = 0, iz = 0;

    int_list result = new_int_list();
    result = setroom_int_list(result, x->sz + y->sz);

    for (ix = 0; ix < x->sz; ix++) {
        register int q = x->arr[ix];
        while (iy < y->sz && y->arr[iy] < q) {
            result->arr[iz++] = y->arr[iy];
            iy += 1;
        }
        if (iy == y->sz || y->arr[iy] != q) {
            result->arr[iz++] = q;
        }
    }
    while (iy < y->sz) {
        result->arr[iz++] = y->arr[iy];
        iy += 1;
    }
    result->sz = iz;

    return result;
}
