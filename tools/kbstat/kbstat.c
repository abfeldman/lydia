#include "hierarchy.h"
#include "variable.h"
#include "config.h"
#include "kbstat.h"
#include "strdup.h"
#include "obdd.h"
#include "util.h"
#include "lcm.h"
#include "tv.h"
#include "mv.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

static FILE *infile;

static char *infilename = NULL;

int has_attribute(const_variable var, char *attribute)
{
    const_variable_attribute attr = find_attribute(var->attributes, add_lydia_symbol(attribute));
    if (attr == variable_attributeNIL) {
        return 0;
    }
    assert(attr->tag == TAGbool_variable_attribute);
    return 1;
}

static void stat_node(node root)
{
    register unsigned int ix;

    unsigned int variables;

    unsigned int pi = 0;
    unsigned int po = 0;
    unsigned int input = 0;
    unsigned int output = 0;

    variables = root->constraints->variables->sz;
    for (ix = 0; ix < variables; ix++) {
        if (has_attribute(root->constraints->variables->arr[ix], "pi")) {
            pi += 1;
        }
        if (has_attribute(root->constraints->variables->arr[ix], "po")) {
            po += 1;
        }
        if (has_attribute(root->constraints->variables->arr[ix], "input")) {
            input += 1;
        }
        if (has_attribute(root->constraints->variables->arr[ix], "output")) {
            output += 1;
        }
    }
    printf("  variables: %d\n", variables);
    printf("  pi: %d\n", pi);
    printf("  po: %d\n", po);
    printf("  input: %d\n", input);
    printf("  output: %d\n", output);
}

static void walk_hierarchy(hierarchy hier, node root, unsigned int *nodes, unsigned int *maxedge, unsigned int *sumedges)
{
    register unsigned int ix;

    printf("node type: %s\n", root->type->name);

    stat_node(root);

    *nodes += 1;

    for (ix = 0; ix < root->edges->sz; ix++) {
	node kid;

	*sumedges += root->edges->arr[ix]->bindings->sz;
	if (root->edges->arr[ix]->bindings->sz > *maxedge) {
	    *maxedge = root->edges->arr[ix]->bindings->sz;
	}
	if (nodeNIL == (kid = find_node(hier, root->edges->arr[ix]->type))) {
	    assert(0);
	    break;
	}
	walk_hierarchy(hier, kid, nodes, maxedge, sumedges);
    }
}

static void stat_hierarchy_recursive(hierarchy h)
{
    unsigned int nodes = 0, maxedge = 0, sumedges = 0;

    walk_hierarchy(h, find_root_node(h), &nodes, &maxedge, &sumedges);

    printf("tree nodes: %d\n", nodes);
    printf("max. edge variables: %d\n", maxedge);
    printf("avg. edge variables: %g\n", (double)sumedges / ((double)nodes - 1));
}

static void walk_dnf_hierarchy(hierarchy hier, node root, unsigned int *terms, unsigned int *max_terms)
{
    register unsigned int ix;

    *terms += to_tv_dnf(root->constraints)->terms->sz;
    if (to_tv_dnf(root->constraints)->terms->sz > *max_terms) {
	*max_terms = to_tv_dnf(root->constraints)->terms->sz;
    }

    for (ix = 0; ix < root->edges->sz; ix++) {
	node kid;
	if (nodeNIL == (kid = find_node(hier, root->edges->arr[ix]->type))) {
	    assert(0);
	    break;
	}
	walk_dnf_hierarchy(hier, kid, terms, max_terms);
    }
}

static void walk_cnf_hierarchy(hierarchy hier, node root, unsigned int *clauses, unsigned int *max_clauses)
{
    register unsigned int ix;

    *clauses += to_tv_cnf(root->constraints)->clauses->sz;
    if (to_tv_cnf(root->constraints)->clauses->sz > *max_clauses) {
	*max_clauses = to_tv_cnf(root->constraints)->clauses->sz;
    }

    for (ix = 0; ix < root->edges->sz; ix++) {
	node kid;
	if (nodeNIL == (kid = find_node(hier, root->edges->arr[ix]->type))) {
	    assert(0);
	    break;
	}
	walk_cnf_hierarchy(hier, kid, clauses, max_clauses);
    }
}

static void stat_dnf_hierarchy_recursive(hierarchy h)
{
    unsigned int terms = 0;
    unsigned int max_terms = 0;

    walk_dnf_hierarchy(h, find_root_node(h), &terms, &max_terms);
    printf("tree terms: %d\n", terms);
    printf("maximal node terms: %d\n", max_terms);
}

static void stat_cnf_hierarchy_recursive(hierarchy h)
{
    unsigned int clauses = 0;
    unsigned int max_clauses = 0;

    walk_cnf_hierarchy(h, find_root_node(h), &clauses, &max_clauses);
    printf("tree clauses: %d\n", clauses);
    printf("maximal node clauses: %d\n", max_clauses);
}

static void stat_hierarchy_flat(hierarchy h)
{
    unsigned int i, nodes = 0, edges = 0;

    for (i = 0; i < h->nodes->sz; i++) {
	nodes += 1;
	edges += h->nodes->arr[i]->edges->sz;
    }

    printf("nodes: %d\n", nodes);
    printf("edges: %d\n", edges);
}

static void stat_tv_wff(tv_wff input)
{
    register unsigned int ix;

    unsigned int variables;
    unsigned int expressions;
    unsigned int health_variables = 0;
    unsigned int observation_variables = 0;

    variables = to_tv_wff(input)->variables->sz;
    expressions = to_tv_wff(input)->e->sz;

    for (ix = 0; ix < to_tv_wff(input)->variables->sz; ix++) {
	if (is_health(to_tv_wff(input)->variables->arr[ix])) {
	    health_variables += 1;
	}
	if (is_observable(to_tv_wff(input)->variables->arr[ix])) {
	    observation_variables += 1;
	}
    }
    printf("health variables: %d\n", health_variables);
    printf("variables: %d\n", variables);
    printf("expressions: %d\n", expressions);
    printf("observation variables: %d\n", observation_variables);
}

static void stat_tv_cnf(tv_cnf input)
{
    register unsigned int ix;

    unsigned int variables;
    unsigned int clauses;
    unsigned int health_variables = 0;
    unsigned int observation_variables = 0;

    variables = to_tv_cnf(input)->variables->sz;
    clauses = to_tv_cnf(input)->clauses->sz;

    for (ix = 0; ix < to_tv_cnf(input)->variables->sz; ix++) {
	if (is_health(to_tv_cnf(input)->variables->arr[ix])) {
	    health_variables += 1;
	}
	if (is_observable(to_tv_cnf(input)->variables->arr[ix])) {
	    observation_variables += 1;
	}
    }
    printf("health variables: %d\n", health_variables);
    printf("variables: %d\n", variables);
    printf("clauses: %d\n", clauses);
    printf("observation variables: %d\n", observation_variables);
}

static void stat_tv_dnf(tv_dnf input)
{
    register unsigned int ix;

    unsigned int variables;
    unsigned int terms;
    unsigned int health_variables = 0;
    unsigned int observation_variables = 0;

    variables = to_tv_dnf(input)->variables->sz;
    terms = to_tv_dnf(input)->terms->sz;

    for (ix = 0; ix < to_tv_dnf(input)->variables->sz; ix++) {
	if (is_health(to_tv_dnf(input)->variables->arr[ix])) {
	    health_variables += 1;
	}
	if (is_observable(to_tv_dnf(input)->variables->arr[ix])) {
	    observation_variables += 1;
	}
    }
    printf("variables: %d\n", variables);
    printf("health variables: %d\n", health_variables);
    printf("observation variables: %d\n", observation_variables);
    printf("terms: %d\n", terms);
}

static void stat_mv_cnf(mv_cnf input)
{
    register unsigned int ix;

    unsigned int variables;
    unsigned int clauses;
    unsigned int health_variables = 0;
    unsigned int observation_variables = 0;

    variables = to_mv_cnf(input)->variables->sz;
    clauses = to_mv_cnf(input)->clauses->sz;

    for (ix = 0; ix < to_mv_cnf(input)->variables->sz; ix++) {
	if (is_health(to_mv_cnf(input)->variables->arr[ix])) {
	    health_variables += 1;
	}
	if (is_observable(to_mv_cnf(input)->variables->arr[ix])) {
	    observation_variables += 1;
	}
    }
    printf("variables: %d\n", variables);
    printf("health variables: %d\n", health_variables);
    printf("observation variables: %d\n", observation_variables);
    printf("clauses: %d\n", clauses);
}

static void stat_mv_dnf(mv_dnf input)
{
    register unsigned int ix;

    unsigned int variables;
    unsigned int terms;
    unsigned int health_variables = 0;
    unsigned int observation_variables = 0;

    variables = to_mv_dnf(input)->variables->sz;
    terms = to_mv_dnf(input)->terms->sz;

    for (ix = 0; ix < to_mv_dnf(input)->variables->sz; ix++) {
	if (is_health(to_mv_dnf(input)->variables->arr[ix])) {
	    health_variables += 1;
	}
	if (is_observable(to_mv_dnf(input)->variables->arr[ix])) {
	    observation_variables += 1;
	}
    }
    printf("variables: %d\n", variables);
    printf("health variables: %d\n", health_variables);
    printf("observation variables: %d\n", observation_variables);
    printf("terms: %d\n", terms);
}

static void stat_obdd(obdd input)
{
    register unsigned int ix;

    unsigned int variables;
    unsigned int nodes;
    unsigned int health_variables = 0;
    unsigned int observation_variables = 0;

    variables = to_obdd(input)->variables->sz;
    nodes = to_obdd(input)->nodes->sz;

    for (ix = 0; ix < to_obdd(input)->variables->sz; ix++) {
	if (is_health(to_obdd(input)->variables->arr[ix])) {
	    health_variables += 1;
	}
	if (is_observable(to_tv_dnf(input)->variables->arr[ix])) {
	    observation_variables += 1;
	}
    }
    printf("variables: %d\n", variables);
    printf("health variables: %d\n", health_variables);
    printf("observation variables: %d\n", observation_variables);
    printf("nodes: %d\n", nodes);
}

static void version(FILE *fp)
{
    fprintf(fp, "kbstat v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
	    "Usage: kbstat [OPTION] SOURCE\n"
	    "  or:  kbstat\n"
	    "Lydia KB Statistics.\n"
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
    serializable input;

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
		fprintf(stderr, "Try `kbstat -h' for more information.\n");
		return EXIT_FAILURE;
	}
    }

    infile = stdin;

    if (optind < argc) {
	infilename = strdup(argv[optind++]);
    }

    if (infilename != NULL && 0 != strcmp(infilename, "-")) {
 	infile = ckfopen(infilename, "r");
    }

    init_symtbl();
    if (!fscan_serializable(infile, &input)) {
	fprintf(stderr, "kbstat: %d: %s\n", lineno, errmsg);
	destroy_symtbl();

	free(infilename);

	return EXIT_FAILURE;
    }

    switch (input->tag)
        {
	case TAGcsp_hierarchy:
	    stat_hierarchy_flat(to_hierarchy(input));
	    stat_hierarchy_recursive(to_hierarchy(input));
	    break;
	case TAGtv_cnf_hierarchy:
	    stat_hierarchy_flat(to_hierarchy(input));
	    stat_hierarchy_recursive(to_hierarchy(input));
	    stat_cnf_hierarchy_recursive(to_hierarchy(input));
	    break;
	case TAGtv_dnf_hierarchy:
	    stat_hierarchy_flat(to_hierarchy(input));
	    stat_hierarchy_recursive(to_hierarchy(input));
	    stat_dnf_hierarchy_recursive(to_hierarchy(input));
	    break;
        case TAGtv_wff_flat_kb:
            stat_tv_wff(to_tv_wff(to_tv_wff_flat_kb(input)->constraints));
            break;
        case TAGtv_cnf_flat_kb:
            stat_tv_cnf(to_tv_cnf(to_tv_cnf_flat_kb(input)->constraints));
            break;
        case TAGtv_dnf_flat_kb:
            stat_tv_dnf(to_tv_dnf(to_tv_dnf_flat_kb(input)->constraints));
            break;
        case TAGobdd_flat_kb:
            stat_obdd(to_obdd(to_obdd_flat_kb(input)->constraints));
            break;
        case TAGmv_cnf_flat_kb:
            stat_mv_cnf(to_mv_cnf(to_mv_cnf_flat_kb(input)->constraints));
            break;
        case TAGmv_dnf_flat_kb:
            stat_mv_dnf(to_mv_dnf(to_mv_dnf_flat_kb(input)->constraints));
            break;
	default:
	    fprintf(stderr, "kbstat: unsupported format\n");
    }
    rfre_serializable(input);
    destroy_symtbl();

    free(infilename);

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
