#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "list.h"
#include "trie.h"
#include "qsort.h"
#include "types.h"
#include "simplify.h"
#include "sorted_int_list.h"

/*
 * Given two clauses `a' and `b', return -1 if it is smarter to try
 * to solve clause `a' before clause `b', return 1 for the reverse case,
 * or 0 if we have no opinion.
 */
static int cmp_bestorder(const void *a, const void *b)
{
    const_tv_literal_set sa = *((const tv_literal_set *)a);
    const_tv_literal_set sb = *((const tv_literal_set *)b);
    unsigned int na = sa->pos->sz + sa->neg->sz;
    unsigned int nb = sb->pos->sz + sb->neg->sz;

/*
 * For the moment we assume that the clause with the fewest 
 * terms should be tried first.
 */
    if (na < nb) {
        return -1;
    }
    if (na > nb) {
        return 1;
    }
    return cmp_tv_literal_set(sa, sb);
}

static tv_literal_set_list bestorder_literal_sets(tv_literal_set_list literal_sets)
{
    if (literal_sets == tv_literal_set_listNIL || literal_sets->sz < 2) {
        return literal_sets;
    }
    lydia_qsort(literal_sets->arr, literal_sets->sz, sizeof(literal_sets->arr[0]), cmp_bestorder);
    return literal_sets;
}

/*
 * Given two sorted int lists 'a', and 'b', we know that both
 * are sorted and have no duplicates, and that 'a' is exactly 1
 * longer than 'b'. Return 1 iff the variables they have in
 * common are equal, and assign to '*pos' the index in `a' of
 * the extra int.
 */
int search_extra_int(const_int_list a, const_int_list b, unsigned int *pos)
{
    unsigned int ix;
    unsigned int res;

    assert(a->sz == b->sz + 1);
    
    /* Search for the first difference. */
    for (ix = 0; ix < b->sz; ix++) {
        if (a->arr[ix] != b->arr[ix]) {
            /* We have found a difference. */
            break;
        }
    }

    /*
     * This may be the position of the difference...
     * (note that is even 1 if we've exhausted list b.)
     */

    res = ix;

    /* ... but make sure that there are no further differences. */
  
    while (ix < b->sz) {
        if (a->arr[ix+1] != b->arr[ix]) {
            /* We have an other difference, so not a candidate. */
            return 0;
        }
        ix++;
    }

    /* Ok, there is only one difference. Rejoice. */
    *pos = res;

    return 1;
}

/*
 * Given two terms, return 1 iff these terms are equivalent,
 * except for exactly one variable that only occurs positive in one
 * clause, and negative in the other.
 *
 * Also update clause 'a' to represent the generalized clause.
 */
static int join_literal_sets(tv_literal_set a, tv_literal_set b)
{
    const_int_list posa = a->pos;
    const_int_list posb = b->pos;
    const_int_list nega = a->neg;
    const_int_list negb = b->neg;

/*
 * Looking at the sizes of the lists, there are only two
 * interesting cases:
 */
    if ((posa->sz + 1 == posb->sz) && (nega->sz == negb->sz + 1)) {
/*
 * posb has one var more than posa, and nega has
 * one var more than negb. Now find them and make sure
 * they are equal.
 */
        unsigned int extrapos;
        unsigned int extraneg;

        if (!search_extra_int(posb, posa, &extrapos)) {
            return 0;
        }
        if (!search_extra_int(nega, negb, &extraneg)) {
            return 0;
        }
        if (posb->arr[extrapos] == nega->arr[extraneg]) {
            delete_int_list(a->neg, extraneg);
            return 1;
        }
        return 0;
    } else if ((posa->sz == posb->sz + 1) && (nega->sz + 1 == negb->sz)) {
/*
 * posa has one var more than posb, and negb has
 * one var more than nega. Now find them and make sure
 * they are equal.
 */
        unsigned int extrapos;
        unsigned int extraneg;

        if (!search_extra_int(posa, posb, &extrapos)) {
            return 0;
        }
        if (!search_extra_int(negb, nega, &extraneg)) {
            return 0;
        }
        if (posa->arr[extrapos] == negb->arr[extraneg]) {
            delete_int_list(a->pos, extrapos);
            return 1;
        }
        return 0;
    }
    return 0;
}

/* Given a list of clauses, try to simplify them. */
tv_literal_set_list simplify_literal_sets(tv_literal_set_list literal_sets)
{
    register unsigned int ix, iy;
    int try_again;

    trie trie = trie_new(NULL, NULL);

    for (ix = 0; ix < literal_sets->sz; ix++) {
        array l = literal_set_to_sorted_int_list(literal_sets->arr[ix]);

        if (trie_is_subsumed(trie, l)) {
            array_free(l);

            rfre_tv_literal_set(literal_sets->arr[ix]);
            literal_sets->arr[ix] = tv_literal_setNIL;
            continue;
        }

        trie_add(trie, l, NULL);

        array_free(l);
    }

    trie_free(trie);

    do {
        try_again = 0;
/* For the moment, we only eliminate more specific literal_sets. */
        for (ix = 0; ix < literal_sets->sz; ix++) {
            tv_literal_set cx = literal_sets->arr[ix];

            if (cx == tv_literal_setNIL) {
                continue;
            }
            for (iy = ix + 1; iy < literal_sets->sz; iy++) {
                tv_literal_set cy = literal_sets->arr[iy];

                if (cy == tv_literal_setNIL) {
                    continue;
                }
                if (join_literal_sets(cx, cy)) {
                    rfre_tv_literal_set(cy);
                    literal_sets->arr[iy] = tv_literal_setNIL;
                    try_again = 1;
                }
            }
        }
    } while(try_again);

/* Now clean up the mess. */
    ix = literal_sets->sz;

    while (ix > 0) {
        ix--;
        if (literal_sets->arr[ix] == tv_literal_setNIL) {
            delete_tv_literal_set_list(literal_sets, ix);
        }
    }

/* Finally, do a sort. */
    literal_sets = bestorder_literal_sets(literal_sets);

    return literal_sets;
}
