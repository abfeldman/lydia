#ifndef RAND_NR_H
#define RAND_NR_H

#ifdef __cplusplus
extern "C"
{
#endif

struct str_rand_nr_ctx
{
    unsigned int max;
    unsigned int left;
    signed char *drawn;
};

typedef struct str_rand_nr_ctx *rand_nr_ctx;

rand_nr_ctx rand_nr_start(unsigned int);
unsigned int rand_nr(rand_nr_ctx);
void rand_nr_end(rand_nr_ctx);

#ifdef __cplusplus
}
#endif

#endif
