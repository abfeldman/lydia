#include "hierarchy.h"
#include "strdup.h"
#include "config.h"
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
#define VERSION_MINOR 1

#define PROGRAM_NAME "kb2lisp"

static FILE *infile;
static FILE *outfile;

char *infilename = NULL;
char *outfilename = NULL;

static void version(FILE *fp)
{
    fprintf(fp, PROGRAM_NAME " v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: " PROGRAM_NAME " [OPTION] SOURCE DEST\n"
            "  or:  " PROGRAM_NAME "\n"
            "CNF LISP pretty printer.\n"
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
    serializable input;
    print_state st;
    int c;

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
    if (!fscan_serializable(infile, &input)) {
        fprintf(stderr, PROGRAM_NAME ": %d: %s\n", lineno, errmsg);
        destroy_symtbl();
        return EXIT_FAILURE;
    }
    st = set_print(outfile, 1, 300, 8);
    switch (input->tag) {
        case TAGtv_wff_flat_kb:
            pp_tv_wff(st, to_tv_wff(to_tv_wff_flat_kb(input)->constraints));
            break;
        case TAGtv_cnf_flat_kb:
            pp_tv_cnf(st, to_tv_cnf(to_tv_cnf_flat_kb(input)->constraints));
            break;
        case TAGhorn_flat_kb:
            pp_horn(st, to_horn(to_horn_flat_kb(input)->constraints));
            break;
        case TAGtv_dnf_flat_kb:
            pp_tv_dnf(st, to_tv_dnf(to_tv_wff_flat_kb(input)->constraints));
            break;
        case TAGmv_wff_flat_kb:
            pp_mv_wff(st, to_mv_wff(to_mv_wff_flat_kb(input)->constraints));
            break;
        case TAGmv_cnf_flat_kb:
            pp_mv_cnf(st, to_mv_cnf(to_mv_wff_flat_kb(input)->constraints));
            break;
        case TAGmv_dnf_flat_kb:
            pp_mv_dnf(st, to_mv_dnf(to_mv_wff_flat_kb(input)->constraints));
            break;
        case TAGtv_wff_hierarchy:
            pp_tv_wff_hierarchy(st, to_tv_wff_hierarchy(input));
            break;
        case TAGtv_cnf_hierarchy:
            pp_tv_cnf_hierarchy(st, to_tv_cnf_hierarchy(input));
            break;
        case TAGtv_dnf_hierarchy:
            pp_tv_dnf_hierarchy(st, to_tv_dnf_hierarchy(input));
            break;
        case TAGmv_wff_hierarchy:
            pp_mv_wff_hierarchy(st, to_mv_wff_hierarchy(input));
            break;
        case TAGmv_cnf_hierarchy:
            pp_mv_cnf_hierarchy(st, to_mv_cnf_hierarchy(input));
            break;
        case TAGmv_dnf_hierarchy:
            pp_mv_dnf_hierarchy(st, to_mv_dnf_hierarchy(input));
            break;
        default:
            fprintf(stderr, PROGRAM_NAME ": not implemented\n");
            break;
    }
    end_print(st);
    rfre_serializable(input);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
