#include "tv.h"
#include "util.h"
#include "tvnnf.h"
#include "config.h"
#include "fprint.h"
#include "strdup.h"
#include "variable.h"
#include "hierarchy.h"

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
static FILE *outfile;

char *infilename = NULL;
char *outfilename = NULL;

static void version(FILE *fp)
{
    fprintf(fp, "wff2nnf v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
	    "Usage: wff2nnf [OPTION] SOURCE DEST\n"
	    "  or:  wff2nnf\n"
	    "Propositional Wff to NNF converter.\n"
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
    print_state st;
    tv_wff_hierarchy input = tv_wff_hierarchyNIL;
    tv_nnf_hierarchy output;
    unsigned int i, j;

    int c;
    extern int optind;

    while ((c = getopt(argc, argv, "h?v")) != -1) {
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
		fprintf(stderr, "Try `wff2nnf -h' for more information.\n");
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

    st = set_print(outfile, 1, 75, 8);
    init_symtbl();
    if (!fscan_tv_wff_hierarchy(infile, &input)) {
	fprintf(stderr, "wff2nnf: %d: %s\n", lineno, errmsg);
	return EXIT_FAILURE;
    }

    output = new_tv_nnf_hierarchy(new_node_list());

    for (i = 0; i < input->nodes->sz; i++) {
	tv_wff node_input = to_tv_wff(input->nodes->arr[i]->constraints);
	tv_nnf node_output = new_tv_nnf(rdup_values_set_list(node_input->domains),
					rdup_variable_list(node_input->variables),
					rdup_variable_list(node_input->encoded_variables),
					rdup_constant_list(node_input->constants),
					node_input->encoding,
					new_tv_nnf_expr_list());
	node result;
	for (j = 0; j < node_input->e->sz; j++) {
	    tv_nnf_expr nnf = convert_nnf(&(node_input->e->arr[j]));
	    append_tv_nnf_expr_list(node_output->e, nnf);
	}
	result = new_node(rdup_lydia_symbol(input->nodes->arr[i]->type),
			  rdup_edge_list(input->nodes->arr[i]->edges),
			  to_kb(node_output));
	output->nodes = append_node_list(output->nodes, result);
    }

    fprint_tv_nnf_hierarchy(st, output);
    rfre_tv_nnf_hierarchy(output);

    rfre_tv_wff_hierarchy(input);

    end_print(st);

    destroy_symtbl();

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
