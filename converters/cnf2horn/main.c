#include "util.h"
#include "stat.h"
#include "strdup.h"
#include "config.h"
#include "fprint.h"
#include "cnf2horn.h"

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
    fprintf(fp, "cnf2horn v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void help(FILE *fp)
{
    fprintf(fp, "Try `cnf2horn -h' for more information.\n");
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: cnf2horn [OPTION] SOURCE DEST\n"
            "  or:  cnf2horn\n"
            "CNF to Horn converter.\n"
            "\n"
            "  -h                   display this help and exit\n"
            "  -v                   output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <lydia@fdir.org>.\n");
}

int main(int argc, char **argv)
{

    FILE *infile;
    FILE *outfile;

    char *infilename = NULL;
    char *outfilename = NULL;

    serializable input = serializableNIL;
    serializable output = serializableNIL;

    print_state st;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "h?v", long_options, &option_index)) != -1) {
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
    if (!fscan_serializable(infile, &input)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                infilename,
                errmsg,
                lineno);
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
        fprintf(stderr, "Input is not in CNF.\n");
        rfre_serializable(input);
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    cnf2horn_init();
    cnf2horn(input, &output);
    cnf2horn_destroy();
    rfre_serializable(input);

    fprint_serializable(st, output);
    rfre_serializable(output);

    end_print(st);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
