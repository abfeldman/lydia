#include "tv.h"
#include "util.h"
#include "strdup.h"
#include "config.h"
#include "fprint.h"
#include "wff2cnf.h"

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
    fprintf(fp, "wff2cnf v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: wff2cnf [OPTION] SOURCE DEST\n"
            "  or:  wff2cnf\n"
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
    serializable input = serializableNIL;
    serializable output = serializableNIL;

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
                fprintf(stderr, "Try `wff2cnf -h' for more information.\n");
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
    input = serializableNIL;
    if (!fscan_serializable(infile, &input)) {
        fprintf(stderr, "wff2cnf: %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        free(infilename);
        free(outfilename);
        return EXIT_FAILURE;
    }
    st = set_print(outfile, 1, 75, 8);
    if (serializableNIL == input) {
        fprint_serializable(st, serializableNIL);
        destroy_symtbl();
        free(infilename);
        free(outfilename);
        return EXIT_SUCCESS;
    }

    if (!wff2cnf(input, &output)) {
        fprintf(stderr, "WARNING: Inconsistent model.\n");
        destroy_symtbl();
        free(infilename);
        free(outfilename);
        return EXIT_FAILURE;
    }

    rfre_serializable(input);
    fprint_serializable(st, output);
    end_print(st);
    rfre_serializable(output);
    destroy_symtbl();

    free(infilename);
    free(outfilename);

    return EXIT_SUCCESS;
}
