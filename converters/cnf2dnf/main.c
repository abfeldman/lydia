#include "util.h"
#include "stat.h"
#include "strdup.h"
#include "config.h"
#include "fprint.h"
#include "cnf2dnf.h"

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
    fprintf(fp, "cnf2dnf v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
	    "Usage: cnf2dnf [OPTION] SOURCE DEST\n"
	    "  or:  cnf2dnf\n"
	    "CNF to DNF converter.\n"
	    "\n"
	    "  -V                   increase verbosity\n"
	    "  -h                   display this help and exit\n"
	    "  -v                   output version information and exit\n"
	    "\n"
	    "With no SOURCE, or when SOURCE is -, read standard input.\n"
	    "\n"
	    "Report bugs to <lydia-dev@falcon.pds.twi.tudelft.nl>.\n");
}

int verbose = 0;

int main(int argc, char **argv)
{
    print_state st;

    serializable input = serializableNIL;
    serializable output = serializableNIL;

    int c;
    extern int optind;

    while ((c = getopt(argc, argv, "Vh?v")) != -1) {
	switch (c) {
	    case 0:
		break;
	    case 'V':
		verbose = 1;
		break;
	    case '?':
 	    case 'h':
		usage(stdout);
		return EXIT_SUCCESS;
 	    case 'v':
		version(stdout);
		return EXIT_SUCCESS;
	    default:
		fprintf(stderr, "Try `cnf2dnf -h' for more information.\n");
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
    if (!fscan_serializable(infile, &input)) {
	fprintf(stderr, "cnf2dnf: %d: %s\n", lineno, errmsg);
	destroy_symtbl();
	return EXIT_FAILURE;
    }
    st = set_print(outfile, 1, 75, 8);
    if (serializableNIL == input) {
	fprint_serializable(st, serializableNIL);

	end_print(st);
	destroy_symtbl();
	return EXIT_SUCCESS;
    }
    if (input->tag != TAGtv_cnf_flat_kb && input->tag != TAGtv_cnf_hierarchy) {
	fprintf(stderr, "cnf2dnf: input is not in CNF\n");
	rfre_serializable(input);
	destroy_symtbl();
	return EXIT_FAILURE;
    }

    cnf2dnf_init();
    cnf2dnf(input, &output);

    if (verbose) {
	display_stat_items(stderr);
    }
    cnf2dnf_destroy();
    rfre_serializable(input);

    fprint_serializable(st, output);
    rfre_serializable(output);

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
