#include "serializable.h"
#include "hierarchy.h"
#include "squeezy.h"
#include "config.h"
#include "fprint.h"
#include "strdup.h"
#include "trie.h"
#include "util.h"

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

int verbose = 0;
int greedy = 0;
int pionly = 0;
int tison = 1;
int tries = 1;


static void version(FILE *fp)
{
    fprintf(fp, "squeezy v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: squeezy [OPTION] SOURCE DEST\n"
            "  or:  squeezy\n"
            "Minimize the SOURCE CNF/DNF and write the result to DEST, or standard input,\n"
            "to standard output.\n"
            "\n"
            "  -p METHOD            specify what method for prime implicates\n"
            "                       to use. METHOD may be `tison' or `brute'\n"
            "  -P                   compute the prime implicates/implicants only\n"
            "  -T                   do not use tries\n");
    fprintf(fp,
            "  -g                   use a greedy minimal overlap algorithm\n"
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
    FILE *infile = stdin;
    FILE *outfile = stdout;

    char *infilename = NULL;
    char *outfilename = NULL;

    serializable output = serializableNIL;
    serializable input = serializableNIL;
    tv_nf nf;

    print_state st;

    int rc = EXIT_SUCCESS;

    int c;
    extern char *optarg;
    extern int optind;

    register unsigned int ix;

    while ((c = getopt(argc, argv, "dgPTh?vVp:")) != -1) {
        switch (c) {
            case 0:
                break;
            case 'p':
                if (NULL == optarg) {
                    fprintf(stderr, "No prime implcates method specified. Try `squeezy -h' for more information.\n");
                    return 1;
                }
                if (0 == strcmp(optarg, "brute")) {
                    tison = 0;
                } else if (0 == strcmp(optarg, "tison")) {
                    /* NOOP */
                } else {
                    fprintf(stderr, "Bad prime implcates method. Try `squeezy -h' for more information.\n");
                    return 1;
                }
                break;
            case 'g':
                greedy = 1;
                break;
            case 'P':
                pionly = 1;
                break;
            case 'T':
                tries = 0;
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
            default:
                fprintf(stderr, "Try `squeezy -h' for more information.\n");
                return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        infilename = argv[optind++];
    }
    if (optind < argc) {
        outfilename = argv[optind++];
    }

    if (infilename != NULL && 0 != strcmp(infilename, "-")) {
        infile = ckfopen(infilename, "r");
    }
    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();

    if (!fscan_serializable(infile, &input)) {
        fprintf(stderr, "squeezy: %d: %s\n", lineno, errmsg);
        rc = EXIT_FAILURE;
        goto exit;
    }
    if (serializableNIL == input) {
        assert(0);
        abort();
    }

    if (input->tag == TAGtv_cnf_flat_kb) {
        assert(TAGtv_cnf == to_tv_cnf_flat_kb(input)->constraints->tag);

        squeeze_nf(to_tv_nf(to_tv_cnf_flat_kb(input)->constraints),
                   &nf,
                   tison,
                   tries,
                   pionly,
                   greedy,
                   verbose);                   

        output = to_serializable(new_tv_cnf_flat_kb(to_tv_cnf_flat_kb(input)->name, to_kb(nf)));
    } else if (input->tag == TAGtv_dnf_flat_kb) {
        assert(TAGtv_dnf == to_tv_cnf_flat_kb(input)->constraints->tag);

        squeeze_nf(to_tv_nf(to_tv_dnf_flat_kb(input)->constraints),
                   &nf,
                   tison,
                   tries,
                   pionly,
                   greedy,
                   verbose);

        output = to_serializable(new_tv_dnf_flat_kb(to_tv_cnf_flat_kb(input)->name, to_kb(nf)));
    } else if (input->tag == TAGtv_cnf_hierarchy || input->tag == TAGtv_dnf_hierarchy) {
        output = rdup_serializable(input);
        if (TAGtv_cnf_hierarchy == input->tag) {
            for (ix = 0; ix < to_tv_cnf_hierarchy(input)->nodes->sz; ix++) {
                if (verbose) {
                    fprintf(stderr, "compressing '%s'\n", to_tv_cnf_hierarchy(input)->nodes->arr[ix]->type->name);
                }
                rfre_kb(to_hierarchy(output)->nodes->arr[ix]->constraints);
                squeeze_nf(to_tv_nf(to_hierarchy(input)->nodes->arr[ix]->constraints),
                           (void *)&(to_hierarchy(output)->nodes->arr[ix]->constraints),
                           tison,
                           tries,
                           pionly,
                           greedy,
                           verbose);
            }
        } else if (TAGtv_dnf_hierarchy == input->tag) {
            for (ix = 0; ix < to_tv_dnf_hierarchy(input)->nodes->sz; ix++) {
                if (verbose) {
                    fprintf(stderr, "compressing '%s'\n", to_tv_dnf_hierarchy(input)->nodes->arr[ix]->type->name);
                }
                rfre_kb(to_hierarchy(output)->nodes->arr[ix]->constraints);
                squeeze_nf(to_tv_nf(to_hierarchy(input)->nodes->arr[ix]->constraints), 
                           (void *)&(to_hierarchy(output)->nodes->arr[ix]->constraints),
                           tison,
                           tries,
                           pionly,
                           greedy,
                           verbose);
            }
        } else {
            assert(0);
            abort();
        }
    } else {
        assert(0);
        abort();
    }

    st = set_print(outfile, 1, 75, 8);
    fprint_serializable(st, output);
    end_print(st);

exit:
    if (serializableNIL != output) {
        rfre_serializable(output);
    }
    if (serializableNIL != input) {
        rfre_serializable(input);
    }

    destroy_symtbl();

    return rc;
}
