#include "serializable.h"
#include "hierarchy.h"
#include "graphml.h"
#include "config.h"
#include "graph.h"
#include "util.h"
#include "lcm.h"
#include "mv.h"
#include "tv.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

signed char process_variables(edge x,
                              edge y,
                              node node_x,
                              node node_y)
{
    register unsigned int ix;
    register unsigned int iy;

    for (ix = 0; ix < x->bindings->sz; ix++) {
        mapping mx = x->bindings->arr[ix];
        unsigned int pos_x;
        tv_wff wff_x = to_tv_wff(node_x->constraints);
        variable vx;
        if (!search_variable_list(wff_x->variables, mx->to, &pos_x)) {
            assert(0);
            abort();
        }
        vx = wff_x->variables->arr[pos_x];
        for (iy = 0; iy < y->bindings->sz; iy++) {
            mapping my = y->bindings->arr[iy];
            unsigned int pos_y;
            tv_wff wff_y = to_tv_wff(node_y->constraints);
            variable vy;
            if (!search_variable_list(wff_y->variables, my->to, &pos_y)) {
                assert(0);
                abort();
            }
            vy = wff_y->variables->arr[pos_y];
            if (isequal_identifier(mx->from, my->from) &&
                vx->name->name->name[0] == 'o' &&
                vy->name->name->name[0] == 'i') {
                return 1;
            }
        }
    }
    return 0;
}

static void version(FILE *fp)
{
    fprintf(fp, "hwff2dmfp v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: hwff2dmfp [OPTION] SOURCE DEST\n"
            "  or:  hwff2dmfp\n"
            "Wff to DIMACS maximum flow converter.\n"
            "\n"
            "  -h                   display this help and exit\n"
            "  -v                   output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <lydia-dev@falcon.pds.twi.tudelft.nl>.\n");
}

int main(int argc, char **argv)
{
    char *infilename = NULL;
    char *outfilename = NULL;

    FILE *infile;
    FILE *outfile;

    serializable input;

    tv_wff_hierarchy hwff;
    node root;

    g_graph g;

    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;

    int c;
    extern int optind;

    while ((c = getopt(argc, argv, "Vsh?v")) != -1) {
        switch (c) {
            case 0:
                break;
            case '?':
            case 'h':
                usage(stdout);
                return EXIT_SUCCESS;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Try `hwff2dmfp -h' for more information.\n");
                return EXIT_FAILURE;
        }
    }

    infile = stdin;
    outfile = stdout;

    if (optind < argc) {
        infilename = argv[optind++];
    }
    if (optind < argc) {
        outfilename = argv[optind++];
    }

    if (infilename != NULL && 0 != strcmp(infilename, "-")) {
        infile = ckfopen(infilename, "r");
    }
    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();
    if (!fscan_serializable(infile, &input)) {
        fprintf(stderr, "hwff2dmfp: %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    hwff = to_tv_wff_hierarchy(input);
    assert(hwff->nodes->sz > 0);
    root = hwff->nodes->arr[hwff->nodes->sz - 1];

    g = graph_new();

    graph_node_add(g, graph_node_new(1)); /* source */
    for (ix = 1; ix <= root->edges->sz; ix++) {
        graph_node_add(g, graph_node_new(ix + 1));
    }
    graph_node_add(g, graph_node_new(root->edges->sz + 2)); /* sink */

    iz = 0;
    for (ix = 0; ix < root->edges->sz; ix++) {
        edge edge_x = root->edges->arr[ix];
        unsigned int node_x;
        node nx;
        if (!search_node_list(hwff->nodes, edge_x->type, &node_x)) {
            assert(0);
            abort();
        }
        nx = hwff->nodes->arr[node_x];
        for (iy = 1; iy < edge_x->bindings->sz; iy++) {
            mapping my = edge_x->bindings->arr[iy];
            unsigned int pos_root;
            tv_wff wff_root = to_tv_wff(root->constraints);
            variable vroot;
            if (!search_variable_list(wff_root->variables,
                                      my->from,
                                      &pos_root)) {
                assert(0);
                abort();
            }
            vroot = wff_root->variables->arr[pos_root];
            if (is_input(vroot)) {
                g_edge e = graph_edge_new(iz++, 1, ix + 2);
                graph_edge_add(g, e);
                break;
            }
        }
    }

    for (ix = 0; ix < root->edges->sz; ix++) {
        edge edge_x = root->edges->arr[ix];
        unsigned int node_x;
        if (!search_node_list(hwff->nodes, edge_x->type, &node_x)) {
            assert(0);
            abort();
        }
        for (iy = 0; iy < root->edges->sz; iy++) {
            if (ix != iy) {
                edge edge_y = root->edges->arr[iy];
                unsigned int node_y;
                if (!search_node_list(hwff->nodes, edge_y->type, &node_y)) {
                    assert(0);
                    abort();
                }
                if (process_variables(edge_x,
                                      edge_y,
                                      hwff->nodes->arr[node_x],
                                      hwff->nodes->arr[node_y])) {
                    g_edge e = graph_edge_new(iz++, ix + 2, iy + 2);
                    graph_edge_add(g, e);
                }
            }
        }
    }


    for (ix = 0; ix < root->edges->sz; ix++) {
        edge edge_x = root->edges->arr[ix];
        unsigned int node_x;
        node nx;
        if (!search_node_list(hwff->nodes, edge_x->type, &node_x)) {
            assert(0);
            abort();
        }
        nx = hwff->nodes->arr[node_x];
        for (iy = 0; iy < 1/*edge_x->bindings->sz*/; iy++) {
            mapping my = edge_x->bindings->arr[iy];
            unsigned int pos_root;
            tv_wff wff_root = to_tv_wff(root->constraints);
            variable vroot;
            if (!search_variable_list(wff_root->variables,
                                      my->from,
                                      &pos_root)) {
                assert(0);
                abort();
            }
            vroot = wff_root->variables->arr[pos_root];
            if (is_output(vroot)) {
                g_edge e = graph_edge_new(iz++, ix + 2, root->edges->sz + 2);
                graph_edge_add(g, e);
                break;
            }
        }
    }

    fprintf(outfile, "p max %d %d\n", g->node_count, g->edge_count);
    fprintf(outfile, "n %d s\n", 1);
    fprintf(outfile, "n %d t\n", g->node_count);
    for (ix = 0; ix < g->edge_count; ix++) {
        fprintf(outfile,
                "a %d %d %d\n",
                g->edges[ix]->src_node,
                g->edges[ix]->dst_node,
                1);
    }

    graph_free(g);

    rfre_serializable(input);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
