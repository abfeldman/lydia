#include "tv.h"
#include "util.h"
#include "fprint.h"
#include "config.h"
#include "strdup.h"
#include "lcm2wff.h"

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
    fprintf(fp, "lcm2wff v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: lcm2wff [OPTION] SOURCE DEST\n"
            "  or:  lcm2wff\n"
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
    print_state st;
    tv_wff_hierarchy output;
    csp_hierarchy input = csp_hierarchyNIL;

    int c;
    extern int optind;

    while ((c = getopt(argc, argv, "h?v")) != -1) {
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
            default:
                fprintf(stderr, "Try `lcm2wff -h' for more information.\n");
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

    if (!fscan_csp_hierarchy(infile, &input)) {
        fprintf(stderr, "lcm2wff: %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        return EXIT_FAILURE;
    }
    if (!lcm2wff(input, &output)) {
        fprintf(stderr, "lcm2wff: Not a propositional model.\n");
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    st = set_print(outfile, 1, 75, 8);
    fprint_tv_wff_hierarchy(st, output);
    end_print(st);

    rfre_tv_wff_hierarchy(output);
    rfre_csp_hierarchy(input);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
