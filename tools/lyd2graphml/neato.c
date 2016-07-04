#include <stdlib.h>

#include "neato.h"

void print_neato(FILE *outfile, g_graph g)
{
    register unsigned int ix;

    fprintf(outfile, "graph G {\n");
    for (ix = 0; ix < g->node_count; ix++) {
	fprintf(outfile, "  %d;\n", g->nodes[ix]->id);
    }
    for (ix = 0; ix < g->edge_count; ix++) {
	if (0 != (int)g->edges[ix]->weight) {
	    fprintf(outfile, "  %d -- %d [weight=%g];\n", g->edges[ix]->src_node, g->edges[ix]->dst_node, g->edges[ix]->weight);
	    continue;
	}
	fprintf(outfile, "  %d -- %d;\n", g->edges[ix]->src_node, g->edges[ix]->dst_node);
    }
    fprintf(outfile, "}\n");
}

/*
 * Local variables:
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
