#include "tv.h"
#include "util.h"
#include "flat_kb.h"
#include "variable.h"
#include "cnf2horn.h"
#include "hierarchy.h"
#include "serializable.h"

#include <stdio.h>
#include <assert.h>

static int gc_unused_variables(horn input)
{
    register unsigned int ix, iy;

    unsigned int *counts;

    array map;

    counts = (unsigned int *)malloc(input->variables->sz * sizeof(unsigned int));
    if (NULL == counts) {
        return 0;
    }
    memset(counts, 0, input->variables->sz * sizeof(unsigned int));

    for (ix = 0; ix < input->clauses->sz; ix++) {
        material_implication clause = input->clauses->arr[ix];
        for (iy = 0; iy < clause->antecedents->sz; iy++) {
            counts[clause->antecedents->arr[iy]] += 1;
        }
        if (clause->consequent != -1) {
            counts[clause->consequent] += 1;
        }
    }

    map = array_new(NULL, NULL);
    for (ix = iy = 0; ix < input->variables->sz; ix++) {
        if (counts[ix] != 0) {
            array_append(map, (void *)(iy++));
        } else {
            array_append(map, NULL);
        }
    }
    for (ix = input->variables->sz - 1; ix < input->variables->sz; ix--) {
        if (counts[ix] == 0) {
            delete_variable_list(input->variables, ix);
        }
    }
    for (ix = 0; ix < input->clauses->sz; ix++) {
        material_implication clause = input->clauses->arr[ix];
        for (iy = 0; iy < clause->antecedents->sz; iy++) {
            clause->antecedents->arr[iy] = (int)map->arr[clause->antecedents->arr[iy]];
        }
        if (clause->consequent != -1) {
            clause->consequent = (int)map->arr[clause->consequent];
        }
    }
    array_free(map);

    free(counts);

    return 1;
}

static horn convert(tv_cnf input)
{
    register unsigned int ix, iy, iz;

    horn result;

    signed char *signs;

    signs = (signed char *)malloc(input->variables->sz * sizeof(signed char));
    if (NULL == signs) {
        return NULL;
    }

    memset(signs, 0, input->variables->sz * sizeof(signed char));

    result = new_horn(rdup_values_set_list(input->domains),
                                new_variable_list(),
                                rdup_variable_list(input->encoded_variables),
                                rdup_constant_list(input->constants),
                                input->encoding,
                                new_material_implication_list());

    for (ix = 0; ix < input->variables->sz; ix++) {
        variable neg = rdup_variable(input->variables->arr[ix]);
        variable pos = rdup_variable(input->variables->arr[ix]);

        char *buf = (char *)malloc(strlen(neg->name->name->name) + 20);
        if (NULL == buf) {
            /* To Do: Normal error handling. */
            assert(0);
            abort();
        }
        buf[0] = '~';
        strncpy(buf + 1, neg->name->name->name, strlen(neg->name->name->name) + 1);
        neg->name->name = add_lydia_symbol(buf);

        append_variable_list(result->variables, neg);
        append_variable_list(result->variables, pos);
    }
    for (ix = 0; ix < input->clauses->sz; ix++) {
        tv_clause clause = input->clauses->arr[ix];
        material_implication implication;

        array buffer = array_new(NULL, NULL);

        for (iy = 0; iy < clause->neg->sz; iy++) {
            array_append(buffer, (void *)(clause->neg->arr[iy] * 2));
            signs[clause->neg->arr[iy]] |= 1;
        }
        for (iy = 0; iy < clause->pos->sz; iy++) {
            array_append(buffer, (void *)(clause->pos->arr[iy] * 2 + 1));
            signs[clause->pos->arr[iy]] |= 2;
        }
        array_int_sort(buffer);

        assert(buffer->sz > 0);

        for (iz = 0; iz < buffer->sz; iz++) {
            implication = new_material_implication(new_int_list(), -1);
            for (iy = 0; iy < buffer->sz; iy++) {
                if (iy == iz) {
                    implication->consequent = (int)buffer->arr[iy];
                } else {
                    int var = (int)buffer->arr[iy] / 2;
                    int sign = (int)buffer->arr[iy] % 2;
                    append_int_list(implication->antecedents, var * 2 + !sign);
                }
            }
            append_material_implication_list(result->clauses, implication);
        }

        array_free(buffer);
    }
    for (ix = 0; ix < input->variables->sz; ix++) {
        material_implication implication;

        if (signs[ix] != 3) {
            continue;
        }

        implication = new_material_implication(new_int_list(), -1);

        append_int_list(implication->antecedents, ix * 2);
        append_int_list(implication->antecedents, ix * 2 + 1);
        
        append_material_implication_list(result->clauses, implication);
    }
    free(signs);

    gc_unused_variables(result);

    return result;
}

int cnf2horn(const_serializable input, serializable *output)
{
    if (input->tag == TAGtv_cnf_flat_kb) {
        tv_cnf_flat_kb input_kb = to_tv_cnf_flat_kb(input);
        tv_cnf input_cnf = to_tv_cnf(input_kb->constraints);

        horn_flat_kb output_kb = new_horn_flat_kb(input_kb->name,
                                                                      to_kb(convert(input_cnf)));
        *output = to_serializable(output_kb);
    } else if (input->tag == TAGtv_cnf_hierarchy) {
/* To Do: Implement the hierarchical version. */
        assert(0);
        abort();
    } else {
        assert(0);
    }

    return 1;
}

void cnf2horn_init()
{
}

void cnf2horn_destroy()
{
}
