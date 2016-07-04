#include <assert.h>
#include <string.h>

#include "pp_variable.h"
#include "tv.h"

static void pp_clause_list(FILE *fp, const_tv_clause_list clauses, const_variable_list variables)
{
    unsigned int ix, iy;
    for (ix = 0; ix < variables->sz; ix++) {
        fprintf(fp, "c variable %d: ", ix + 1);
        pp_variable_name(fp, variables->arr[ix]->name);
        fprintf(fp, "\n");
    }
    fprintf(fp, "p cnf %d %d\n", variables->sz, clauses->sz);
    for (ix = 0; ix < clauses->sz; ix++) {
        const_int_list pos = clauses->arr[ix]->pos;
        const_int_list neg = clauses->arr[ix]->neg;

        int first = 1;
        for (iy = 0; iy < pos->sz; iy++) {
            if (!first) {
                fprintf(fp, " ");
            } else {
                first = 0;
            }
            fprintf(fp, "%d", pos->arr[iy] + 1);
        }
        for (iy = 0; iy < neg->sz; iy++) {
            if (!first) {
                fprintf(fp, " ");
            } else {
                first = 0;
            }
            fprintf(fp, "-%d", neg->arr[iy] + 1);
        }
        if (!first) {
            fprintf(fp, " ");
        }
        fprintf(fp, "%d\n", 0);
    }
}

void pp_cnf(FILE *fp, const_tv_cnf cnf)
{
    pp_clause_list(fp, cnf->clauses, cnf->variables);
}
