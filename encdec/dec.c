#include "variable.h"
#include "dec.h"

#include <assert.h>

mv_term decode_term(const_tv_term term,
                    const_variable_list variables,
                    const_variable_list encoded_variables,
                    const_values_set_list domains,
                    int encoding)
{
    register unsigned int ix;
    register unsigned int k;
    register unsigned int n;
    register unsigned int m;
    register unsigned int q;

    unsigned int *buffer;

    const_bool_variable var;
    mv_literal literal;

    mv_term result;

    char *p;
    char *s;

    int var_idx;
    int val;
    int bit;

    result = new_mv_term(new_mv_literal_list(), new_mv_literal_list());

    if (ENCODING_NONE == encoding) {
        assert(0);
        abort();
    } else if (ENCODING_SPARSE == encoding) {
        for (ix = 0; ix < term->pos->sz; ix++) {
            const_bool_variable var = to_bool_variable(variables->arr[term->pos->arr[ix]]);
            assert(var->encoded_variable != -1);
            if (NULL != (p = strchr(var->name->name->name, '$'))) {
                val = strtol(p + 1, &s, 10);
                assert(s != NULL && *s == '\0');

                literal = new_mv_literal(var->encoded_variable, val);
                append_mv_literal_list(result->pos, literal);
            }
        }
    } else if (ENCODING_DENSE == encoding) {
        buffer = (unsigned int *)malloc(sizeof(unsigned int) * encoded_variables->sz);
        if (NULL == buffer) {
            rfre_mv_term(result);
            return mv_termNIL;
        }
        memset(buffer, 0, sizeof(unsigned int) * encoded_variables->sz);

        for (ix = 0; ix < term->pos->sz; ix++) {
            var_idx = term->pos->arr[ix];
            var = to_bool_variable(variables->arr[var_idx]);

            assert(TAGbool_variable == variables->arr[var_idx]->tag);

            if (NULL != (p = strrchr(var->name->name->name, '$'))) {
                bit = strtol(p + 1, &s, 10);
                assert(s != NULL && *s == '\0');

                buffer[var->encoded_variable] |= (1 << bit);
            }
        }

        for (ix = 0; ix < encoded_variables->sz; ix++) {
/*
            if (TAGbool_variable == encoded_variables->arr[ix]->tag) {
                continue;
            }
*/
            assert(TAGenum_variable == encoded_variables->arr[ix]->tag);

            n = domains->arr[to_enum_variable(encoded_variables->arr[ix])->values_set]->entries->sz;
            m = n - 1;
            q = 2;

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
        }

        for (ix = 0; ix < term->neg->sz; ix++) {
            var_idx = term->neg->arr[ix];
            var = to_bool_variable(variables->arr[var_idx]);
            if ((unsigned int)-1 != buffer[var->encoded_variable]) {
                literal = new_mv_literal(var->encoded_variable, 
                                         buffer[var->encoded_variable]);
                append_mv_literal_list(result->pos, literal);
                buffer[var->encoded_variable] = (unsigned int)-1;
            }
        }
        for (ix = 0; ix < term->pos->sz; ix++) {
            var_idx = term->pos->arr[ix];
            var = to_bool_variable(variables->arr[var_idx]);
            if ((unsigned int)-1 != buffer[var->encoded_variable]) {
                literal = new_mv_literal(var->encoded_variable,
                                         buffer[var->encoded_variable]);
                append_mv_literal_list(result->pos, literal);
                buffer[var->encoded_variable] = (unsigned int)-1;
            }
        }
        free(buffer);
    } else {
        assert(0);
        abort();
    }

    return result;
}
