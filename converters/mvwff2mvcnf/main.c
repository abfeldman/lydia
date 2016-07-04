/* filename:     main.c
 * description:  main interface for mvwff2mvcnf
 * author:       Tom Janssen (TU Delft)
 */


#include "mvwff2mvcnf.h"
#include "mv.h"
#include "util.h"
#include "strdup.h"
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif
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
    fprintf(fp, "mvwff2mvcnf v. %d.%d (TU Delft)\n",
            VERSION_MAJOR,
            VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: mvwff2mvcnf [OPTION] SOURCE DEST\n"
            "  or:  mvwff2mvcnf\n"
            "Multi-valued propositional WFF to CNF convertor.\n"
            "\n"
            "  -? or\n"
            "  -h               display this help and exit\n"
            "  -v               output version information and exit\n"
            "  -V               verbose output\n"
            "  -b               read and write in binary format\n"
            "\n"
            "Report bugs to <lydia-dev@falcon.pds.twi.tudelft.nl>.\n");
}

int main(int argc, char **argv)
{
    print_state st;
    serializable input = serializableNIL;
    serializable output = serializableNIL;

    int verbose = 0;
    int binary = 0;

    int c;
    extern int optind;

    while((c = getopt(argc, argv, "Vh?vb")) != -1) {
        switch(c) {
            case 0:
                break;
            case 'V':
                verbose = 1;
                break;
            case '?':
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
                fprintf(stderr, "Try 'mvwff2mvcnf -h' for more information.\n");
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

    if (infilename != NULL  &&  strcmp(infilename, "-")!=0) {
        infile = ckfopen(infilename, "r");
    }
    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();
    if ((binary ? fread_serializable(infile, &input) : !fscan_serializable(infile, &input))) {
        fprintf(stderr, "mvwff2mvcnf: %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        free(infilename);
        free(outfilename);
        return EXIT_FAILURE;
    }
    
    if (input == serializableNIL) {
        st = set_print(outfile, 1, 75, 8);
        fprint_serializable(st, serializableNIL);
        end_print(st);
        destroy_symtbl();
        free(infilename);
        free(outfilename);
        return EXIT_SUCCESS;
    }

    if (!(input->tag == TAGmv_wff_flat_kb ||
          input->tag == TAGmv_wff_hierarchy)) {
        fprintf(stderr, "mvwff2mvcnf: unsupported model\n");
        destroy_symtbl();
        free(infilename);
        free(outfilename);
        return EXIT_FAILURE;
    }

    if ((output = (serializable)mvwff2mvcnf(input)) == NULL) {
        fprintf(stderr, "WARNING: Inconsistent model.\n");
        destroy_symtbl();
        free(infilename);
        free(outfilename);
        return EXIT_FAILURE;
    }

    rfre_serializable(input);
    if (binary) {
        fwrite_serializable(outfile, output);
    } else {
        st = set_print(outfile, 1, 75, 8);
        fprint_serializable(st, output);
        end_print(st);
    }
    rfre_serializable(output);
    destroy_symtbl();

    free(infilename);
    free(outfilename);

    return EXIT_SUCCESS;
}
