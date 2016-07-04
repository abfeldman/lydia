#include "serializable.h"
#include "hierarchy.h"
#include "graphml.h"
#include "config.h"
#include "graph.h"
#include "neato.h"
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

static int mv_clause_has_variable(const_mv_clause cx, int var)
{
    register unsigned int ix;

    for (ix = 0; ix < cx->pos->sz; ix++) {
	if (cx->pos->arr[ix]->var == var) {
	    return 1;
	}
    }
    for (ix = 0; ix < cx->neg->sz; ix++) {
	if (cx->neg->arr[ix]->var == var) {
	    return 1;
	}
    }

    return 0;
}

static int mv_clause_shares_variables(const_mv_clause cx, const_mv_clause cy)
{
    register unsigned int ix;

    for (ix = 0; ix < cx->pos->sz; ix++) {
	if (mv_clause_has_variable(cy, cx->pos->arr[ix]->var)) {
	    return 1;
	}
    }
    for (ix = 0; ix < cx->neg->sz; ix++) {
	if (mv_clause_has_variable(cy, cx->neg->arr[ix]->var)) {
	    return 1;
	}
    }

    return 0;
}

static g_graph mv_cnf_to_graph(mv_cnf input)
{
    register unsigned int ix, iy, iz = 0;

    g_graph result = graph_new();

    for (ix = 0; ix < input->clauses->sz; ix++) {
	graph_node_add(result, graph_node_new(ix));
    }

    for (ix = 0; ix < input->clauses->sz; ix++) {
	for (iy = ix + 1; iy < input->clauses->sz; iy++) {
	    if (mv_clause_shares_variables(input->clauses->arr[ix], input->clauses->arr[iy]) > 0) {
		graph_edge_add(result, graph_edge_new(iz++, ix, iy));
	    }
	}
    }

    return result;
}

static int tv_clause_has_variable(const_tv_clause cx, int var)
{
    register unsigned int ix;

    for (ix = 0; ix < cx->pos->sz; ix++) {
	if (cx->pos->arr[ix] == var) {
	    return 1;
	}
    }
    for (ix = 0; ix < cx->neg->sz; ix++) {
	if (cx->neg->arr[ix] == var) {
	    return 1;
	}
    }

    return 0;
}

static int tv_clause_shares_variables(const_tv_clause cx, const_tv_clause cy)
{
    register unsigned int ix;

    for (ix = 0; ix < cx->pos->sz; ix++) {
	if (tv_clause_has_variable(cy, cx->pos->arr[ix])) {
	    return 1;
	}
    }
    for (ix = 0; ix < cx->neg->sz; ix++) {
	if (tv_clause_has_variable(cy, cx->neg->arr[ix])) {
	    return 1;
	}
    }

    return 0;
}

static g_graph tv_cnf_to_graph(tv_cnf input)
{
    register unsigned int ix, iy, iz = 0;

    g_graph result = graph_new();

    for (ix = 0; ix < input->clauses->sz; ix++) {
	graph_node_add(result, graph_node_new(ix));
    }

    for (ix = 0; ix < input->clauses->sz; ix++) {
	for (iy = ix + 1; iy < input->clauses->sz; iy++) {
	    if (tv_clause_shares_variables(input->clauses->arr[ix], input->clauses->arr[iy]) > 0) {
		graph_edge_add(result, graph_edge_new(iz++, ix, iy));
	    }
	}
    }

    return result;
}


static void version(FILE *fp)
{
    fprintf(fp, "cnf2graphml v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
	    "Usage: cnf2graphml [OPTION] SOURCE DEST\n"
	    "  or:  cnf2graphml\n"
	    "CNF to GraphML converter.\n"
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
		fprintf(stderr, "Try `cnf2graphml -h' for more information.\n");
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
    if (fscan_serializable(infile, &input)) {
	fprintf(stderr, "cnf2graphml: %d: %s\n", lineno, errmsg);
	return EXIT_FAILURE;
    }
    g_graph result = NULL;
    switch (input->tag) {
	case TAGtv_cnf:
	    result = tv_cnf_to_graph(to_tv_cnf(input));
	    break;
	case TAGmv_cnf:
	    result = mv_cnf_to_graph(to_mv_cnf(input));
	    break;
	default:
	    fprintf(stderr, "cnf2graphml: not implemented\n");
	    break;
    }
    if (NULL != result) {
	print_graphml(stdout, result);
	graph_free(result);
    }
    rfre_serializable(input);

    return EXIT_SUCCESS;
}

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
