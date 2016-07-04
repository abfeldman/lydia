#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "transv.h"

int_list_list new_adjacency_matrix(tv_literal_set_list literal_sets, tv_literal_set_list pi)
{
    unsigned int x, y;
    int_list_list matrix = new_int_list_list();

    int tmp = 0;
    for (y = 0; y < literal_sets->sz; y++) {
        matrix = append_int_list_list(matrix, new_int_list());
        for (x = 0; x < pi->sz; x++) {
            int subsumes = (int)is_subsumed_literal_set(pi->arr[x], literal_sets->arr[y]);
            matrix->arr[y] = append_int_list(matrix->arr[y], subsumes);
            tmp += subsumes;
        }
    }
    return matrix;
}

int find_lone_one(int_list lt, unsigned int *offset)
{
    unsigned int i;
    int result = 0;
    for (i = 0; i < lt->sz; i++) {
        if (lt->arr[i] == 1) {
            *offset = i;
            result = !result;
            if (!result) {
                return result;
            }
        }
    }
    return result;
}

/* Returns 1 iff every element in li is less than or equal to the
   respective element in lj. */
int is_le(int_list li, int_list lj)
{
    unsigned int i;
    int result = 1;
    assert(li->sz == lj->sz);
    for (i = 0; i < li->sz; i++) {
        if (li->arr[i] > lj->arr[i]) {
            result = 0;
        }
    }
    return result;
}

int is_zero_column(int_list_list matrix, unsigned int col)
{
    unsigned int i;
    for (i = 0; i < matrix->sz; i++) {
        if (0 != matrix->arr[i]->arr[col]) {
            return 0;
        }
    }
    return 1;
}

void delete_column(int_list_list matrix, unsigned int col)
{
    unsigned int i;
    for (i = 0; i < matrix->sz; i++) {
        matrix->arr[i] = delete_int_list(matrix->arr[i], col);
    }
}

void era(int_list_list matrix, tv_literal_set_list pi)
{
    unsigned int i, j, q, y;
    int reduction;
    do {
        reduction = 0;
/* Step 1: */
        for (y = 0; y < matrix->sz; y++) {
            unsigned int k;
            if (find_lone_one(matrix->arr[y], &k)) {
                for (q = 0; q < matrix->sz; q++) {
                    if (q != y && 1 == matrix->arr[q]->arr[k]) {
                        matrix = delete_int_list_list(matrix, q);
                        if (q <= y) {
                            y -= 1;
                        }
                        q -= 1;
                        reduction = 1;
                    }
                }
            }
        }
/* Step 2: */
        for (i = 0; i < matrix->sz; i++) {
            for (j = 0; j < matrix->sz; j++) {
                if (i == j) {
                    continue;
                }
                if (is_le(matrix->arr[j], matrix->arr[i])) {
                    matrix = delete_int_list_list(matrix, j);
                    if (j <= i) {
                        i -= 1;
                    }
                    j -= 1;
                    reduction = 1;
                }
            }
        }
/* Step 3: */
        if (0 == matrix->sz) {
/* Can this happen? */
            break;
        }
        for (i = 0; i < matrix->arr[0]->sz; i++) {
            if (is_zero_column(matrix, i)) {
                delete_column(matrix, i);
                pi = delete_tv_literal_set_list(pi, i);
                i -= 1;
                reduction = 1;
            }
        }
    } while (reduction);
}

int get_sum_col(int_list_list matrix, unsigned int col)
{
    unsigned int i;
    int result = 0;
    for (i = 0;i < matrix->sz; i++) {
        result += matrix->arr[i]->arr[col];
    }
    return result;
}

unsigned int get_max_col(int_list_list matrix)
{
    unsigned int max = 0;
    unsigned int result = 0;
    unsigned int i;

    assert(matrix->sz != 0);

    for (i = 0; i < matrix->arr[0]->sz; i++) {
        unsigned int j = get_sum_col(matrix, i);
        if (j >= max) {
            max = j;
            result = i;
        }
    }
    return result;
}

/* This function destroys the input matrix. */
tv_literal_set_list greedy_era(int_list_list matrix, tv_literal_set_list pi)
{
    tv_literal_set_list input = rdup_tv_literal_set_list(pi);
    tv_literal_set_list result = new_tv_literal_set_list();
    unsigned int q;

    while (matrix->sz > 0) {
        unsigned c = get_max_col(matrix);
        result = append_tv_literal_set_list(result, rdup_tv_literal_set(input->arr[c]));
        input = delete_tv_literal_set_list(input, c);
        for (q = 0; q < matrix->sz; q++) {
            if (1 == matrix->arr[q]->arr[c]) {
                matrix = delete_int_list_list(matrix, q);
                q -= 1;
            }
        }
    }

    rfre_tv_literal_set_list(input);

    return result;
}
