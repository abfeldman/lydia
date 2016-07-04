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

static FILE *infile;
static FILE *outfile;

char *infilename = NULL;
char *outfilename = NULL;

static unsigned int num_variables;
static unsigned int *varsx = NULL;
static unsigned int *varsy = NULL;

static void walk_tv_wff_expr(const_tv_wff_expr e, unsigned int *result)
{
    switch (e->tag) {
        case TAGtv_wff_e_or:
            walk_tv_wff_expr(to_tv_wff_e_or(e)->lhs, result);
            walk_tv_wff_expr(to_tv_wff_e_or(e)->rhs, result);
            break;
        case TAGtv_wff_e_not:
            walk_tv_wff_expr(to_tv_wff_e_not(e)->n, result);
            break;
        case TAGtv_wff_e_and:
            walk_tv_wff_expr(to_tv_wff_e_and(e)->lhs, result);
            walk_tv_wff_expr(to_tv_wff_e_and(e)->rhs, result);
            break;
        case TAGtv_wff_e_equiv:
            walk_tv_wff_expr(to_tv_wff_e_equiv(e)->lhs, result);
            walk_tv_wff_expr(to_tv_wff_e_equiv(e)->rhs, result);
            break;
        case TAGtv_wff_e_impl:
            walk_tv_wff_expr(to_tv_wff_e_impl(e)->lhs, result);
            walk_tv_wff_expr(to_tv_wff_e_impl(e)->rhs, result);
            break;
        case TAGtv_wff_e_var:
            result[to_tv_wff_e_var(e)->v] = 1;
            break;
        case TAGtv_wff_e_const:
/* Noop. */
            break;
    }
}
static unsigned int tv_wff_expr_share_variables(const_tv_wff_expr ex,
                                                const_tv_wff_expr ey)
{
    register unsigned int ix;
    register unsigned int result = 0;

    bzero(varsx, sizeof(unsigned int) * num_variables);
    bzero(varsy, sizeof(unsigned int) * num_variables);
    walk_tv_wff_expr(ex, varsx);
    walk_tv_wff_expr(ey, varsy);

    for (ix = 0; ix < num_variables; ix++) {
        if (varsx[ix] > 0 && varsy[ix] > 0) {
            result += 1;
        }
    }
    return result;
}

static g_graph tv_wff_to_graph(tv_wff input)
{
    register unsigned int iw, ix, iy, iz = 0;

    g_graph result = graph_new();

    for (ix = 0; ix < input->e->sz; ix++) {
        graph_node_add(result, graph_node_new(ix));
    }

    for (ix = 0; ix < input->e->sz; ix++) {
        for (iy = ix + 1; iy < input->e->sz; iy++) {
            if (0 != (iw = tv_wff_expr_share_variables(input->e->arr[ix],
                                                       input->e->arr[iy]))) {
                g_edge e = graph_edge_new(iz++, ix, iy);
                e->weight = iw;
                graph_edge_add(result, e);
            }
        }
    }

    return result;
}

static void version(FILE *fp)
{
    fprintf(fp, "wff2graphml v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: wff2graphml [OPTION] SOURCE DEST\n"
            "  or:  wff2graphml\n"
            "Wff to GraphML converter.\n"
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
                fprintf(stderr, "Try `wff2graphml -h' for more information.\n");
                return EXIT_FAILURE;
        }
    }

    infile = stdin;
    outfile = stdout;

    if (optind < argc) {
        infilename = strdup(argv[optind++]);
    }
    if (optind < argc) {
        outfilename = strdup(argv[optind++]);
    }

    if (infilename != NULL && 0 != strcmp(infilename, "-")) {
        infile = ckfopen(infilename, "r");
    }
    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    serializable input;
    g_graph result = NULL;
    tv_wff wff;

    init_symtbl();
    if (!fscan_serializable(infile, &input)) {
        fprintf(stderr, "wff2graphml: %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        return EXIT_FAILURE;
    }
    switch (input->tag) {
        case TAGtv_wff_flat_kb:
            wff = to_tv_wff(to_tv_wff_flat_kb(input)->constraints);

            num_variables = to_tv_wff(wff)->variables->sz;
            varsx = (unsigned int *)malloc(sizeof(unsigned int) * num_variables);
            varsy = (unsigned int *)malloc(sizeof(unsigned int) * num_variables);

            result = tv_wff_to_graph(to_tv_wff(wff));

            free(varsx);
            free(varsy);
            break;
        default:
            fprintf(stderr, "wff2graphml: not implemented\n");
            break;
    }
    if (NULL != result) {
        print_graphml(stdout, result);
        graph_free(result);
    }
    rfre_serializable(input);

    destroy_symtbl();

    return EXIT_SUCCESS;
}
