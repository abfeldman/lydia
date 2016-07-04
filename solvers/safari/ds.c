#include "ds.h"
#include "defs.h"
#include "variable.h"

#include <stdlib.h>
#include <assert.h>
#ifdef MPI
# include <mpi.h>
#endif

extern int proc_id;
extern int proc_count;

#ifdef MPI
safari_node safari_node_mpi_bcast(safari_node node, int root)
{
    register unsigned int ix;
    safari_literal lit;

    unsigned int buf[2];
    unsigned int *assignments;

    if (root == proc_id) {
        buf[0] = node->cardinality;
        buf[1] = node->assignments->sz;
    }

    MPI_Bcast(buf, 2, MPI_UNSIGNED, root, MPI_COMM_WORLD);

    assignments = (unsigned int *)malloc(buf[1] * sizeof(unsigned int));
    if (NULL == assignments) {
        assert(0);
        abort();
    }
    if (root == proc_id) {
        for (ix = 0; ix < node->assignments->sz; ix++) {
            lit = (safari_literal)node->assignments->arr[ix];
            assignments[ix] = lit->var * 2 + lit->val;
        }
    }

    MPI_Bcast(assignments, buf[1], MPI_UNSIGNED, root, MPI_COMM_WORLD);

    if (root != proc_id) {
        node = safari_node_new();

        node->cardinality = buf[0];
        for (ix = 0; ix < buf[1]; ix++) {
            lit = safari_literal_new(assignments[ix] / 2, assignments[ix] % 2);
            array_append(node->assignments, lit);
        }
    }

    free(assignments);

    return node;
}
#endif

safari_literal safari_literal_new(const int var, const int val)
{
    safari_literal literal = (safari_literal)malloc(sizeof(struct str_safari_literal));
    if (NULL == literal) {
        return literal;
    }

    literal->var = var;
    literal->val = val;

    return literal;
}

safari_literal safari_literal_copy(safari_literal literal)
{
    safari_literal result = (safari_literal)malloc(sizeof(struct str_safari_literal));
    if (NULL == result) {
        return result;
    }

    result->var = literal->var;
    result->val = literal->val;

    return result;
}

void safari_literal_free(safari_literal literal)
{
    free(literal);
}

safari_node safari_node_new()
{
    safari_node node = (safari_node)malloc(sizeof(struct str_safari_node));
    if (NULL == node) {
        return node;
    }

    node->assignments = array_new((array_element_destroy_func_t)safari_literal_free,
                                  (array_element_clone_func_t)safari_literal_copy);
    node->cardinality = 0;

    return node;
}

safari_node safari_node_copy(const safari_node node)
{
    safari_node result = (safari_node)malloc(sizeof(struct str_safari_node));
    if (NULL == result) {
        return result;
    }

    result->assignments = array_copy(node->assignments);
    result->cardinality = node->cardinality;

    return result;
}

void safari_node_free(safari_node node)
{
    if (NULL == node) {
        return;
    }

    if (NULL != node->assignments) {
        array_free(node->assignments);
    }

    free(node);
}
