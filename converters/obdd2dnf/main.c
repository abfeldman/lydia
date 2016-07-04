#include "tv.h"
#include "defs.h"
#include "hash.h"
#include "util.h"
#include "obdd.h"
#include "config.h"
#include "fprint.h"
#include "strdup.h"
#include "obdd2dnf.h"
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
    fprintf(fp, "obdd2dnf v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: obdd2dnf [OPTION] SOURCE DEST\n"
            "  or:  obdd2dnf\n"
            "OBDD to DNF converter.\n"
            "\n"
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

    serializable input = serializableNIL;
    serializable output = serializableNIL;

    print_state st;

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
                fprintf(stderr, "Try `obdd2dnf -h' for more information.\n");
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
        fprintf(stderr, "obdd2dnf: %d: %s\n", lineno, errmsg);
        return EXIT_FAILURE;
    }

    st = set_print(outfile, 1, 75, 8);
    if (serializableNIL == input) {
        fprint_serializable(st, serializableNIL);
        destroy_symtbl();
        return EXIT_SUCCESS;
    }
    if (input->tag != TAGobdd_flat_kb && input->tag != TAGobdd_hierarchy) {
        fprintf(stderr, "obdd2dnf: input is not an OBDD\n");
        rfre_serializable(input);
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    obdd2dnf(input, &output);
    rfre_serializable(input);

    fprint_serializable(st, output);
    rfre_serializable(output);
    end_print(st);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
