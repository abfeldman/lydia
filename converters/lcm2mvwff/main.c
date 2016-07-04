#include "mv.h"
#include "lcm.h"
#include "util.h"
#include "strdup.h"
#include "config.h"
#include "lcm2mvwff.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define VERSION_MAJOR 2
#define VERSION_MINOR 0

static FILE *infile;
static FILE *outfile;

static char *infilename = NULL;
static char *outfilename = NULL;

static void version(FILE *fp)
{
    fprintf(fp, "lcm2mvwff v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
	    "Usage: lcm2mvwff [OPTION] SOURCE DEST\n"
	    "  or:  lcm2mvwff\n"
	    "Propositional Wff to CNF converter.\n"
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
    csp_hierarchy input = csp_hierarchyNIL;
    mv_wff_hierarchy output;
    print_state st;
    int binary = 0;

    int c;
    extern int optind;

    while ((c = getopt(argc, argv, "h?vb")) != -1) {
	switch (c) {
	    case 0:
		break;
	    case '?':
 	    case 'h':
		usage(stdout);
		return EXIT_FAILURE;
 	    case 'v':
		version(stdout);
		return EXIT_SUCCESS;
	    case 'b':
		binary = 1;
		break;
	    default:
		fprintf(stderr, "Try `lcm2mvwff -h' for more information.\n");
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

    init_symtbl();
    if (binary?fread_csp_hierarchy(infile, &input):
	       !fscan_csp_hierarchy(infile, &input)) {
	fprintf(stderr, "lcm2mvwff: %d: %s\n", lineno, errmsg);
	free(infilename);
	free(outfilename);
	destroy_symtbl();
	return EXIT_FAILURE;
    }
    if (!lcm2mvwff(input, &output)) {
	free(infilename);
	free(outfilename);
	destroy_symtbl();
	return EXIT_FAILURE;
    }

    if(binary) {
	fwrite_mv_wff_hierarchy(outfile, output);
    } else {
    	st = set_print(outfile, 1, 75, 8);
    	fprint_mv_wff_hierarchy(st, output);
    	end_print(st);
    }

    rfre_mv_wff_hierarchy(output);
    rfre_csp_hierarchy(input);

    free(infilename);
    free(outfilename);

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
