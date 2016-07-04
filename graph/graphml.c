/**
 *  \file graphml.c
 *  \brief Graph printing in GraphML format.
 */

#include <stdlib.h>

#include "graphml.h"

void print_graphml(FILE *outfile, g_graph g)
{
    register unsigned int ix;

    fprintf(outfile,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"\n"
            "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
            "         xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\"\n"
            "         xmlns:y=\"http://www.yworks.com/xml/graphml\">\n"
            "  <key id=\"d0\" for=\"node\" yfiles.type=\"nodegraphics\" />\n"
            "  <key id=\"d2\" for=\"edge\" yfiles.type=\"edgegraphics\"/>\n"
            "  <graph id=\"G\" edgedefault=\"undirected\">\n");

    for (ix = 0; ix < g->node_count; ix++) {
        if (g->nodes[ix]->label == NULL) {
            fprintf(outfile, "    <node id=\"n%d\" />\n", g->nodes[ix]->id);
            continue;
        }
        fprintf(outfile, "    <node id=\"n%d\">\n", g->nodes[ix]->id);
        fprintf(outfile, "      <data key=\"d0\">\n");
        fprintf(outfile, "        <y:ShapeNode>\n");
        if (g->nodes[ix]->color != 0x1ffffff) {
            fprintf(outfile, "          <y:Fill color=\"#%X\" transparent=\"false\" />\n", g->nodes[ix]->color);
        }
        fprintf(outfile, "          <y:NodeLabel>%s</y:NodeLabel>\n", g->nodes[ix]->label);
        fprintf(outfile, "        </y:ShapeNode>\n");
        fprintf(outfile, "      </data>\n");
        fprintf(outfile, "    </node>\n");
    }
    for (ix = 0; ix < g->edge_count; ix++) {
        if (g->edges[ix]->label == NULL) {
            fprintf(outfile, "    <edge source=\"n%d\" target=\"n%d\" />\n", g->edges[ix]->src_node, g->edges[ix]->dst_node);
            continue;
        }
        fprintf(outfile, "    <edge source=\"n%d\" target=\"n%d\">\n", g->edges[ix]->src_node, g->edges[ix]->dst_node);
        fprintf(outfile, "      <data key=\"d2\">\n");
        fprintf(outfile, "        <y:PolyLineEdge>\n");
        fprintf(outfile, "          <y:EdgeLabel>%s</y:EdgeLabel>\n", g->edges[ix]->label);
        fprintf(outfile, "        </y:PolyLineEdge>\n");
        fprintf(outfile, "      </data>\n");
        fprintf(outfile, "    </edge>\n");
    }
    fprintf(outfile, 
            "  </graph>\n"
            "</graphml>\n");
}
