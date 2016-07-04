#include "smoothy.h"
#include "config.h"
#include "strdup.h"
#include "util.h"
#include "stat.h"

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
    fprintf(fp, "smoothy v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: smoothy [OPTION] SOURCE DEST\n"
            "  or:  smoothy\n"
            "Produce a flat DNF from a hierarchical one.\n"
            "\n"
            "  -s <depth>           partially inline CNF/DNF\n"
            "  -S                   skip Boolean CNF/DNF simplification\n"
            "  -V                   increase verbosity\n"
            "  -h                   display this help and exit\n"
            "  -v                   output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <alex@llama.gs>.\n");
}

int main(int argc, char **argv)
{
    extern int optind;
    extern char *optarg;

    print_state st;

    serializable input, output;

    char *p = NULL;

    int simplify = 1;
    int verbose = 0;
    int hier = 0;
    int c;

    unsigned int depth = 1;

    while ((c = getopt(argc, argv, "s:h?vVS")) != -1) {
        switch (c) {
            case 0:
                break;
            case 's':
                hier = 1;
                if (NULL == optarg) {
                    fprintf(stderr, "Missing inlining depth. Try `smoothy -h' for more information.\n");
                }
                depth = strtol(optarg, &p, 10);
                if (p == NULL || *p != '\0') {
                    fprintf(stderr, "Bad inlining depth. Try `smoothy -h' for more information.\n");
                }
                break;
            case 'S':
                simplify = 0;
                break;
            case 'V':
                verbose = 1;
                break;
            case '?':
            case 'h':
                usage(stdout);
                return EXIT_FAILURE;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Try `smoothy -h' for more information.\n");
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
    input = serializableNIL;
    if (!fscan_serializable(infile, &input)) {
        fprintf(stderr, "smoothy: %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        return EXIT_FAILURE;
    }
    output = serializableNIL;

    smoothy_init();
    if (!smoothy(input, hier, depth, verbose, simplify, &output)) {
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    if (verbose) {
        display_stat_items(stderr);
    }
    smoothy_destroy();

    rfre_serializable(input);
    fprint_serializable(st, output);
    rfre_serializable(output);
    end_print(st);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
