#include "pp_variable.h"
#include "variable.h"
#include "config.h"
#include "strdup.h"
#include "util.h"
#include "lsim.h"
#include "defs.h"
#include "diag.h"
#include "dec.h"
#include "io.h"
#include "tv.h"

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#ifndef WIN32
# include <sys/time.h>
#endif
#include <signal.h>

#define VERSION_MAJOR 2
#define VERSION_MINOR 1
#define PROMPT "lsim> "
#define MODULE "lsim"

/* Forward declarations. */
extern command commandtab[];

static void cmd_sim(const void *UNUSED(context),
                    FILE *outfile,
                    const char **parmv,
                    const unsigned int UNUSED(parmc))
{
    diagnostic_problem dp;
    tv_term alpha;
    tv_term result = NULL;
    int status;

    if (NULL == (dp = diagnostic_problem_get(parmv[1]))) {
        lydia_error(outfile, "Can not find a model `%s'.", parmv[1]);
        return;
    }
    if (NULL == (alpha = observation_get(dp->alphas, parmv[2]))) {
        lydia_error(outfile, "Can not find an observation `%s'.", parmv[2]);
        return;
    }

    status = lsim(dp, alpha, &result);
    if (SAT == status) {
        lydia_start_output(outfile, parmv[0]);
        if (dp->encoding == ENCODING_NONE) {
            pp_tv_term(outfile, dp->variables, result);
        } else {
            mv_term decoded_term = decode_term(result,
                                               dp->variables,
                                               dp->encoded_variables,
                                               dp->domains,
                                               dp->encoding);
            pp_mv_term(outfile,
                       dp->encoded_variables,
                       dp->domains,
                       decoded_term);
            rfre_mv_term(decoded_term);
        }
        fprintf(outfile, "\n");
        lydia_stop_output(outfile, parmv[0]);
    } else if (UNSAT == status) {
        lydia_error(outfile, "BCP found a contradiction.");
    } else if (UNKNOWN == status) {
        lydia_error(outfile, "Can not solve by using BCP only. Did you instantiate all inputs?");
    } else {
        /* To Do: Intenral error. */
        assert(0);
        abort();
    }
    if (NULL != result) {
        rfre_tv_term(result);
    }
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
            "Usage: " MODULE " [OPTION] MODEL OBSERVATIONS [OUTPUT]\n"
            "  or:  " MODULE "\n"
            "LYDIA discrete simulation engine.\n"
            "\n"
            "  -n, --nominal         generate a nominal observation\n"
            "  -h, --help            display this help and exit\n"
            "  -v, --version         output version information and exit\n"
            "\n"
            "With no OUTPUT, or when OUTPUT is -, write to standard output.\n"
            "\n"
            "Report bugs to <alex@llama.gs>.\n");
}

command commandtab[] = {
    { "sim",            cmd_sim,         2,  2, "Simulate the system" },
    { NULL,             NULL,           -1, -1, NULL }  /* End of the table. */
};

int main(int argc, char **argv)
{
    FILE *modelfile = NULL;
    FILE *obsfile = NULL;
    FILE *outfile = stdout;

    char *modelfilename = NULL;
    char *obsfilename = NULL;
    char *outfilename = NULL;

    serializable model = serializableNIL;
    tv_dnf_hierarchy dump = tv_dnf_hierarchyNIL;

    observations alphas = NULL;
    diagnostic_problem ds = NULL;

    int rc = EXIT_SUCCESS;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "h?vn", long_options, &option_index)) != -1) {
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
                fprintf(stderr, "Try `lsim -h' for more information.\n");
                return EXIT_FAILURE;
        }
    }

    if (argc < 3) {
        fprintf(stderr, "Missing mandatory model and observations.\n");
        help(stdout);
        return EXIT_FAILURE;
    }

    if (optind < argc) {
        modelfilename = argv[optind++];
    }
    if (optind < argc) {
        obsfilename = argv[optind++];
    }
    if (optind < argc) {
        outfilename = argv[optind++];
    }

    if (modelfilename != NULL) {
        modelfile = ckfopen(modelfilename, "r");
    }
    if (NULL != obsfilename) {
        obsfile = ckfopen(obsfilename, "r");
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
    if (NULL != obsfile && !fscan_tv_dnf_hierarchy(obsfile, &dump)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                obsfilename,
                errmsg,
                lineno);
        rc = EXIT_FAILURE;
        goto exit;
    }

    if (serializableNIL == model) {
        fprintf(stderr, "Nothing to simulate.\n");
        rc = EXIT_FAILURE;
        goto exit;
    }

    ds = diagnostic_problem_new(to_serializable(model));

    lsim_init(ds, model);

    if (observations_load(to_hierarchy(dump), ds->variables, &alphas)) {
        fprintf(stderr, "Error loading observations.\n");

        rc = EXIT_FAILURE;

        goto exit;
    }

    diagnostic_problem_add(ds->name->name, ds, alphas);

    enable_listings();

    cli_init(MODULE, VERSION_MAJOR, VERSION_MINOR);
    cli_loop(model, stdin, PROMPT, commandtab, outfile);
    cli_destroy();

    lsim_destroy(ds);

exit:
    diagnostic_problems_free();

    if (serializableNIL != model) {
        rfre_serializable(model);
    }
    if (tv_dnf_hierarchyNIL != dump) {
        rfre_tv_dnf_hierarchy(dump);
    }

    destroy_symtbl();

    return rc;
}
