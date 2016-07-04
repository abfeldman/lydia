#include "config.h"
#include "strdup.h"
#include "util.h"
#include "tv.h"
#include "pp.h"

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

#define PROGRAM_NAME "cnf2dimacs"

static void version(FILE *fp)
{
    fprintf(fp, PROGRAM_NAME " v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: " PROGRAM_NAME " [OPTION] SOURCE DEST\n"
            "  or:  " PROGRAM_NAME "\n"
            "Lydia CNF to DIMACS converter.\n"
            "\n"
            "  -h, --help            display this help and exit\n"
            "  -v, --version         output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <lydia-dev@falcon.pds.twi.tudelft.nl>.\n");
}

int main(int argc, char **argv)
{
    FILE *infile;
    FILE *outfile;

    char *infilename = NULL;
    char *outfilename = NULL;

    tv_cnf_flat_kb input = tv_cnf_flat_kbNIL;

    int c, option_index = 0;

    struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "hv", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 'h':
                usage(stdout);
                return EXIT_SUCCESS;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Try `" PROGRAM_NAME " -h' for more information.\n");
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
    if (!fscan_tv_cnf_flat_kb(infile, &input)) {
        fprintf(stderr, PROGRAM_NAME ": %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        return EXIT_FAILURE;
    }
    pp_cnf(outfile, to_tv_cnf(input->constraints));
    rfre_tv_cnf_flat_kb(input);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
