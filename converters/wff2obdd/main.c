#include "tv.h"
#include "defs.h"
#include "stat.h"
#include "hash.h"
#include "util.h"
#include "obdd.h"
#include "strdup.h"
#include "config.h"
#include "fprint.h"
#include "wff2obdd.h"
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

static char *infilename = NULL;
static char *outfilename = NULL;

static void version(FILE *fp)
{
    fprintf(fp, "wff2obdd v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: wff2obdd [OPTION] SOURCE DEST\n"
            "  or:  wff2obdd\n"
            "Propositional Wff to OBDD converter.\n"
            "\n"
            "  -f                   use naive method\n"
            "  -V                   increase verbosity\n"
            "  -h                   display this help and exit\n"
            "  -v                   output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <lydia-bugs@fdir.org>.\n");
}

int main(int argc, char **argv)
{
    int c;
    extern int optind;

    int brute_force = 0;
    int verbose = 0;

    serializable input = serializableNIL;
    serializable output = serializableNIL;

    print_state st;

    while ((c = getopt(argc, argv, "Vfh?v")) != -1) {
        switch (c) {
            case 0:
                break;
            case 'V':
                verbose = 1;
                break;
            case 'f':
                brute_force = 1;
                break;
            case '?':
            case 'h':
                usage(stdout);
                return EXIT_SUCCESS;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Try `wff2obdd -h' for more information.\n");
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
        fprintf(stderr, "wff2obdd: %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        return EXIT_FAILURE;
    }
    st = set_print(outfile, 1, 75, 8);
    if (serializableNIL == input) {
        fprint_serializable(st, serializableNIL);
        destroy_symtbl();
        return EXIT_SUCCESS;
    }
    if (input->tag != TAGtv_wff_flat_kb && input->tag != TAGtv_wff_hierarchy) {
        fprintf(stderr, "wff2obdd: input is not in propositional Wff\n");
        rfre_serializable(input);
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    wff2obdd_init();
    wff2obdd(input, &output, brute_force);

    if (verbose) {
        display_stat_items(stderr);
    }
    wff2obdd_destroy();

    rfre_serializable(input);

    fprint_serializable(st, output);
    rfre_serializable(output);
    end_print(st);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
