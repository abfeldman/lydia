#include "trie.h"
#include "defs.h"
#include "debug.h"
#include "pp_variable.h"

static tv_variables_cache tv_cache;

static void best_kernels_visitor(void *outfile, void *node)
{
    cdas_node_print((FILE *)outfile, tv_cache, (cdas_node)node);
}

void cdas_best_kernels_print(FILE *outfile,
                             tv_variables_cache cache,
                             trie best_kernels)
{
    tv_cache = cache;

    fprintf(outfile, "Best Kernels:\n");
    fprintf(outfile, "{\n");
    trie_nodes_visit(best_kernels,
                     best_kernels_visitor,
                     outfile);
    fprintf(outfile, "}\n");
}

void cdas_assumable_list_print(FILE *outfile,
                               tv_variables_cache tv_cache,
                               array assignment)
{
    register unsigned int ix;

    fprintf(outfile, "{");
    for (ix = 0; ix < assignment->sz; ix++) {
        cdas_literal constituent_kernel = (cdas_literal)assignment->arr[ix];

        if (ix != 0) {
            fprintf(outfile, ", ");
        }

        pp_variable_name(outfile, tv_cache->list->arr[tv_cache->a_to_v[constituent_kernel->var]]->name);
        fprintf(outfile, " = %s", constituent_kernel->sign ? "true" : "false");
    }
    fprintf(outfile, "}\n");
}

void cdas_node_print(FILE *outfile,
                     tv_variables_cache tv_cache,
                     cdas_node node)
{
    if (NULL == node) {
        fprintf(outfile, " node: %p\n", (void *)node);
        return;
    }
    fprintf(outfile, " node: %p\n", (void *)node);
    fprintf(outfile, "  conflict: %p\n", (void *)node->conflict);
    fprintf(outfile, "  kernel: %u\n", node->kernel);
    fprintf(outfile, "  cost: %g\n", node->cost);
    fprintf(outfile, "  cardinality: %u\n", node->cardinality);
    fprintf(outfile, "  assignments: ");
    cdas_assumable_list_print(outfile, tv_cache, node->assignments);
}

void cdas_node_list_print(FILE *outfile,
                          tv_variables_cache tv_cache,
                          array nodes)
{
    register unsigned int ix;

    if (NULL == nodes) {
        return;
    }

    for (ix = 0; ix < nodes->sz; ix++) {
        cdas_node node = (cdas_node)nodes->arr[ix];

        if (ix != 0) {
            fprintf(outfile, "\n");
        }

        cdas_node_print(outfile, tv_cache, node);
    }
}

void cdas_priority_queue_print(FILE *outfile,
                               tv_variables_cache tv_cache,
                               priority_queue queue)
{
    register unsigned int ix;

    fprintf(outfile, "Priority Queue:\n");
    fprintf(outfile, "{\n");

    if (NULL == queue) {
        fprintf(outfile, "}\n");
        return;
    }

    for (ix = 0; ix < queue->sz; ix++) {
        cdas_node node = (cdas_node)queue->arr[ix];

        if (ix != 0) {
            fprintf(outfile, "\n");
        }

        cdas_node_print(outfile, tv_cache, node);
    }
    fprintf(outfile, "}\n");
}

void cdas_constituent_kernels_print(FILE *outfile,
                                    tv_variables_cache tv_cache,
                                    cdas_context kgp)
{
    register unsigned int ix;

    fprintf(outfile, "Constituent Kernels:\n");
    fprintf(outfile, "{\n");
    for (ix = 0; ix < kgp->constituent_kernels->sz; ix++) {
        array constituent_kernels = (array)kgp->constituent_kernels->arr[ix];
        fprintf(outfile, " %p: ", (void *)constituent_kernels);
        cdas_assumable_list_print(outfile, tv_cache, constituent_kernels);
    }
    fprintf(outfile, "}\n");
}

void cdas_label_print(FILE *outfile, array label)
{
    register unsigned int ix;

    for (ix = 0; ix < label->sz; ix++) {
        if (ix != 0) {
            fprintf(outfile, ", ");
        }
        fprintf(outfile, "%d", (int)label->arr[ix]);
    }
    fprintf(outfile, "\n");
}
