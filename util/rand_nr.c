#include "rand_nr.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

rand_nr_ctx rand_nr_start(unsigned int max)
{
    rand_nr_ctx result = (rand_nr_ctx)malloc(sizeof(struct str_rand_nr_ctx));
    if (NULL == result) {
        return result;
    }
    assert(max > 0);
    result->drawn = (signed char *)malloc(max * sizeof(signed char));
    if (NULL == result->drawn) {
        free(result);
        return NULL;
    }
    result->max = max;
    result->left = max;
    memset(result->drawn, 0, max * sizeof(signed char));

    return result;
}

unsigned int rand_nr(rand_nr_ctx ctx)
{
    register unsigned int ix, iy, iz;

    if (0 == ctx->left) {
        return (unsigned int)-1;
    }
    ix = (unsigned int)(rand() / (((double)RAND_MAX + 1) / ctx->left));

    for (iy = iz = 0; iy < ctx->max; iy++) {
        if (!ctx->drawn[iy]) {
            if (ix == iz) {
                break;
            }
            iz += 1;
        }
    }
    assert(iy < ctx->max);

    ctx->drawn[iy] = 1;
    ctx->left -= 1;

    return iy;
}

void rand_nr_end(rand_nr_ctx ctx)
{
    assert(NULL != ctx);
    assert(NULL != ctx->drawn);

    free(ctx->drawn);
    free(ctx);
}
