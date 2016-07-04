#include "hierarchy.h"
#include "variable.h"
#include "strdup.h"
#include "config.h"
#include "fprint.h"
#include "util.h"
#include "enc.h"
#include "mv.h"
#include "tv.h"

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

static void version(FILE *fp)
{
    fprintf(fp, "mvwff2tvwff v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void help(FILE *fp)
{
    fprintf(fp, "Try `mvwff2tvwff -h' for more information.\n");
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: mvwff2tvwff [OPTION] SOURCE DEST\n"
            "  or:  mvwff2tvwff\n"
            "Propositional Multi-Valued Wff to Two-Valued Wff converter.\n"
            "\n"
            "  -d, --dense          use dense instead of sparse encodings\n"
            "  -h                   display this help and exit\n"
            "  -v                   output version information and exit\n"
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

    mv_wff_hierarchy input = mv_wff_hierarchyNIL;
    tv_wff_hierarchy output = tv_wff_hierarchyNIL;

    print_state st;

    int encoding = ENCODING_SPARSE;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"dense", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "h?vd", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 'd':
                encoding = ENCODING_DENSE;
                break;
            case 'h':
                usage(stdout);
                return EXIT_FAILURE;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                help(stderr);
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
    if (!fscan_mv_wff_hierarchy(infile, &input)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                infilename,
                errmsg,
                lineno);
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    mvwff2tvwff(input, &output, encoding);

    st = set_print(outfile, 1, 75, 8);
    fprint_tv_wff_hierarchy(st, output);
    end_print(st);

    rfre_tv_wff_hierarchy(output);
    rfre_mv_wff_hierarchy(input);

    free(infilename);
    free(outfilename);

    destroy_symtbl();

    return EXIT_SUCCESS;
}
