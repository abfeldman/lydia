#ifndef SAFARI_DS_H
#define SAFARI_DS_H

#include "array.h"

#define SAFARI_LITERAL_FALSE        0
#define SAFARI_LITERAL_TRUE         1

typedef struct str_safari_literal *safari_literal;
typedef struct str_safari_node *safari_node;

struct str_safari_literal
{
    int var;
    int val;
};

struct str_safari_node
{
    array assignments;         /* array of literals */

    unsigned int cardinality;
};

extern safari_literal safari_literal_new(const int, const int);
extern safari_literal safari_literal_copy(safari_literal);
extern void safari_literal_free(safari_literal);

extern safari_node safari_node_new();
extern safari_node safari_node_copy(const safari_node);
extern void safari_node_free(safari_node);

#ifdef MPI
extern safari_node safari_node_mpi_bcast(safari_node, int);
#endif

#endif
