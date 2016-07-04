#ifndef DS_H
#define DS_H

#include "tv.h"
#include "trie.h"
#include "array.h"
#include "priority_queue.h"

#define CDAS_LITERAL_FALSE        0
#define CDAS_LITERAL_TRUE         1

typedef struct str_cdas_literal *cdas_literal;
typedef struct str_cdas_node *cdas_node;
typedef struct str_cdas_context *cdas_context;

struct str_cdas_literal
{
    int var;
    signed char sign;
};

struct str_cdas_context
{
    priority_queue nodes;

    array constituent_kernels; /* array of arrays of constituent kernels */
    trie constituent_kernels_trie;
};

struct str_cdas_node
{
    array assignments;         /* array of literals */
    double cost;

/* the conflict this node resolves */
    array conflict;
/* the kernel in the conflict this node uses */
    unsigned int kernel;

    unsigned int cardinality;
};

extern cdas_literal cdas_literal_new(const int, const signed char);
extern cdas_literal cdas_literal_copy(cdas_literal);
extern void cdas_literal_free(cdas_literal);

extern cdas_node cdas_node_new();
extern cdas_node cdas_node_copy(const cdas_node);
extern void cdas_node_free(cdas_node);

extern cdas_context cdas_context_new();
extern void cdas_context_free(cdas_context);

#endif
