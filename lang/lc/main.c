#include "lc.h"
#include "lcm.h"
#include "util.h"
#include "error.h"
#include "strdup.h"
#include "config.h"
#include "fprint.h"
#include "findbin.h"

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define VERSION_MAJOR 2
#define VERSION_MINOR 0
#define MODULE "lc"

static FILE *infile = NULL;
static FILE *outfile = NULL;

static char prefix[4096]; /* To Do: Fix this. */
static char **include_path = NULL;
static unsigned int include_path_size = 0;

static char *infilename = NULL;
static char *outfilename = NULL;

static void add_include_path(char *path)
{
    include_path = (char **)realloc(include_path, (include_path_size + 1) * sizeof(char *));
    include_path[include_path_size] = strdup(path);
    include_path_size += 1;
}

static void free_include_paths()
{
    register unsigned int ix;

    for (ix = 0; ix < include_path_size; ix++) {
        free(include_path[ix]);
    }
    free(include_path);
}

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
            "Usage: " MODULE " [OPTION] SOURCE DEST\n"
            "  or:  " MODULE "\n"
            "Compile the SOURCE LYDIA model and write the result to DEST, or standard input,\n"
            "to standard output.\n"
            "\n"
            "  -I<dir>              add <dir> to the directory search path\n"
            "      --unused         dump unused systems\n"
            "  -h, --help           display this help and exit\n"
            "  -v, --version        output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <alex@llama.gs>.\n");
}

int main(int argc, char **argv)
{
    csp_hierarchy lcm;
    print_state st;
    int binary = 0;

    int option_unused_systems = 0;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"unused", 0, 0, 1},
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {0, 0, 0, 0}
    };

    strncpy(prefix, findbin(argv[0], 1), sizeof(prefix));

    add_include_path(canonify(prefix, "components"));

    while ((c = getopt_long(argc, argv, "hvI:b", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 1:
                option_unused_systems = 1;
                break;
            case 'I':
                add_include_path(optarg);
                break;
            case 'h':
                usage(stdout);
                return EXIT_SUCCESS;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            case 'b':
                binary = 1;
                break;
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

    infilename = (infilename == NULL ? strdup("-") : infilename);

    if (0 != strcmp(infilename, "-")) {
        if (NULL == (infile = fopen(infilename, "r"))) {
            leh_error(COMPILER_OPEN_ERROR,
                      LEH_LOCATION_GLOBAL,
                      originNIL,
                      infilename,
                      strerror(errno));
            return EXIT_FAILURE;
        }
    }
    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();
    if (!lc(infile,
            infilename,
            include_path,
            include_path_size,
            option_unused_systems,
            &lcm)) {
        free(infilename);
        free(outfilename);
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    if (binary) {
        fwrite_csp_hierarchy(outfile, lcm);
    } else {
        st = set_print(outfile, 1, 75, 8);
        fprint_csp_hierarchy(st, lcm);
        end_print(st);
    }
    
    rfre_csp_hierarchy(lcm);

    free(infilename);
    free(outfilename);
    free_include_paths();

    destroy_symtbl();

    return EXIT_SUCCESS;
}
