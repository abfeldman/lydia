#include "cnf2table.h"
#include "config.h"
#include "util.h"

#include <time.h>
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
#define MODULE "cnf2table"

static void version(FILE *fp)
{
    fprintf(fp, MODULE " v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void help(FILE *fp)
{
    fprintf(fp, "Try `" MODULE " -h' for more information.\n");
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: " MODULE " [OPTION] CNF [OUTPUT]\n"
            "  or:  " MODULE "\n"
            "Uses BCP to build a truth-table from a CNF formula.\n"
            "\n"
            "Mandatory arguments to long options are mandatory for short options too.\n"
            "  -h, --help            display this help and exit\n"
            "  -v, --version         output version information and exit\n"
            "\n"
            "With no OUTPUT, or when OUTPUT is -, write to standard output.\n"
            "\n"
            "Report bugs to <alex@llama.gs>.\n");
}

int main(int argc, char **argv)
{
    FILE *modelfile = NULL;
    FILE *outfile = NULL;

    char *modelfilename = NULL;
    char *outfilename = NULL;

    serializable model = serializableNIL;

    int rc = EXIT_SUCCESS;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
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
                help(stderr);
                return EXIT_FAILURE;
        }
    }

    modelfile = stdin;
    outfile = stdout;

    if (optind < argc) {
        modelfilename = argv[optind++];
    }

    if (optind < argc) {
        outfilename = argv[optind++];
    }

    if (modelfilename != NULL) {
        modelfile = ckfopen(modelfilename, "r");
    }
    if (outfilename != NULL && 0 != strcmp(outfilename, "-")) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();

    if (!fscan_serializable(modelfile, &model)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                modelfilename,
                errmsg,
                lineno);
        rc = EXIT_FAILURE;
        goto exit;
    }
    if (model->tag != TAGtv_cnf_flat_kb) {
        fprintf(stderr,
                "Unsupported file format: `%s'.\n",
                modelfilename);
        rc = EXIT_FAILURE;
        goto exit;
    }

    cnf2table(to_tv_cnf_flat_kb(model), outfile);

exit:
    if (serializableNIL != model) {
        rfre_serializable(model);
    }
    if (outfile != NULL) {
        fclose(outfile);
    }

    destroy_symtbl();

    return rc;
}
