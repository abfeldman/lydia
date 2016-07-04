#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tv.h"
#include "qsort.h"

static int cmp_ints(const void *a, const void *b)
{
    return abs(*((const int *)a)) - abs(*((const int *)b));
}

static void pp_dimacs_term(FILE *outfile, const_tv_term term)
{
    register unsigned int ix;

    int_list result = new_int_list();
    for (ix = 0; ix < term->pos->sz; ix++) {
	append_int_list(result, term->pos->arr[ix] + 1);
    }
    for (ix = 0; ix < term->neg->sz; ix++) {
	append_int_list(result, -term->neg->arr[ix] - 1);
    }
    if (result->sz > 1) {
	lydia_qsort(result->arr, result->sz, sizeof(result->arr[0]), cmp_ints);
    }
    for (ix = 0; ix < result->sz; ix++) {
	if (0 != ix) {
	    fprintf(outfile, " ");
	}
	fprintf(outfile, "%d", result->arr[ix]);
    }
    rfre_int_list(result);

    fprintf(outfile, "\n");
}

static void pp_dimacs_term_list(FILE *outfile, const_tv_term_list terms)
{
    register unsigned int ix;
    for (ix = 0; ix < terms->sz; ix++) {
	pp_dimacs_term(outfile, terms->arr[ix]);
    }
}

void pp_dimacs_dnf(FILE *outfile, const_tv_dnf dnf)
{
    pp_dimacs_term_list(outfile, dnf->terms);
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
