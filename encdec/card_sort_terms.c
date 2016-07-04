#include "sorted_int_list.h"
#include "hierarchy.h"
#include "variable.h"
#include "qsort.h"
#include "util.h"
#include "tv.h"
#include "mv.h"

#include <assert.h>
#include <math.h>

typedef struct str_term_sort_bucket *term_sort_bucket;
typedef const struct str_term_sort_bucket *const_term_sort_bucket;

struct str_term_sort_bucket
{
    const_tv_term term;
    unsigned int cardinality;
};

static unsigned int get_dense_encoded_term_cardinality(const_tv_variables_cache tv_cache,
                                                       const_mv_variables_cache mv_cache,
                                                       const_tv_term term)
{
    unsigned int result = 0;
    register unsigned int ix;

    const_int_list pos = term->pos;

    char *p;
    char *q;

    unsigned int *buffer = (unsigned int *)malloc(sizeof(unsigned int) * mv_cache->assumables);
    if (NULL == buffer) {
/* To Do: Normal error handling. */
        assert(0);
        abort();
    }
    memset(buffer, 0, sizeof(unsigned int) * mv_cache->assumables);

    for (ix = 0; ix < pos->sz; ix++) {
        int var_idx = pos->arr[ix];
        if (tv_cache->is_assumable[var_idx]) {
            const_bool_variable var = to_bool_variable(tv_cache->list->arr[var_idx]);
            assert(TAGbool_variable == tv_cache->list->arr[var_idx]->tag);
            if (NULL != (p = strrchr(var->name->name->name, '$'))) {
                int bit = strtol(p + 1, &q, 10);
                assert(q != NULL && *q == '\0');

                buffer[mv_cache->v_to_a[var->encoded_variable]] |= (1 << bit);
            }
        }
    }
    for (ix = 0; ix < mv_cache->assumables; ix++) {
        register unsigned int n = mv_cache->domain_size[mv_cache->a_to_v[ix]];
        register unsigned int k, m = n - 1, q = 2;
        while (m > 1) {
            m = m >> 1;
            q = q << 1;
        }
        k = q - n;

        if (buffer[ix] / 2 < k) {
            buffer[ix] /= 2;
        } else {
            buffer[ix] -= k;
        }
        if (!get_health_bool(mv_cache->list->arr[mv_cache->a_to_v[ix]], buffer[ix])) {
            result += 1;
        }
    }

    free(buffer);

    return result;
}

static term_sort_bucket term_sort_bucket_new(const_tv_term term,
                                             unsigned int cardinality)
{
    term_sort_bucket result = (term_sort_bucket)malloc(sizeof(struct str_term_sort_bucket));
    if (NULL == result) {
        return result;
    }

    result->term = term;
    result->cardinality = cardinality;

    return result;
}

unsigned int get_tv_term_cardinality(tv_variables_cache cache,
                                     const_tv_term state)
{
    unsigned int result = 0;

    const_int_list pos = state->pos;
    const_int_list neg = state->neg;

    register unsigned int ix;

    for (ix = 0; ix < neg->sz; ix++) {
        if (cache->is_assumable[neg->arr[ix]]) {
            if (!(cache->health_states[cache->v_to_a[neg->arr[ix]]] & 1)) {
                result += 1;
            }
        }
    }
    for (ix = 0; ix < pos->sz; ix++) {
        if (cache->is_assumable[pos->arr[ix]]) {
            if (!(cache->health_states[cache->v_to_a[pos->arr[ix]]] & 2)) {
                result += 1;
            }
        }
    }

    return result;
}

static int cmp_term_sort_buckets(const void *itema, const void *itemb)
{
    const_term_sort_bucket a = *(const_term_sort_bucket *)itema;
    const_term_sort_bucket b = *(const_term_sort_bucket *)itemb;

    return a->cardinality > b->cardinality ? 1 : (a->cardinality < b->cardinality ? -1 : cmp_tv_term(a->term, b->term));
}

tv_term_list cardinality_sort_terms(tv_variables_cache tv_cache,
                                    mv_variables_cache mv_cache,
                                    tv_term_list terms,
                                    int encoding)
{
    register unsigned int ix;

    term_sort_bucket bucket;
    tv_term term;

    int cardinality;

    array buf = array_new((array_element_destroy_func_t)free, NULL);
    if (NULL == buf) {
        return tv_term_listNIL;
    }
    for (ix = 0; ix < terms->sz; ix++) {
        term = terms->arr[ix];

        sort_int_list(term->neg);
        sort_int_list(term->pos);

        switch (encoding) {
            case ENCODING_NONE:
            case ENCODING_SPARSE:
                cardinality = get_tv_term_cardinality(tv_cache, term);
                break;
            case ENCODING_DENSE:
                cardinality = get_dense_encoded_term_cardinality(tv_cache,
                                                                 mv_cache,
                                                                 term);
                break;
            default:
                assert(0);
                abort();
        }
        bucket = term_sort_bucket_new(terms->arr[ix], cardinality);
        if (!array_append(buf, bucket)) {
            array_free(buf);
            return tv_term_listNIL;
        }
    }
    lydia_qsort(buf->arr,
                buf->sz,
                sizeof(buf->arr[0]),
                cmp_term_sort_buckets);
    for (ix = 0; ix < buf->sz; ix++) {
        terms->arr[ix] = (tv_term)((term_sort_bucket)buf->arr[ix])->term;
    }
    array_free(buf);

    return terms;
}
