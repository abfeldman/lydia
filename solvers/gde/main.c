#include "sorted_int_list.h"
#include "pp_variable.h"
#include "variable.h"
#include "config.h"
#include "fprint.h"
#include "strdup.h"
#include "util.h"
#include "defs.h"
#include "hash.h"
#include "obs.h"
#include "gde.h"
#include "io.h"

#include <ctype.h>
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
#define PROMPT "gde> "
#define MODULE "gde"

static void cmd_conflicts(const void *UNUSED(context),
                          FILE *outfile,
                          const char **parmv,
                          const unsigned int UNUSED(parmc))
{
    diagnostic_problem dp;
    tv_term alpha;

    if (NULL == (dp = diagnostic_problem_get(parmv[1]))) {
        lydia_error(outfile, "Can not find a model `%s'.", parmv[1]);
        return;
    }
    if (NULL == (alpha = observation_get(dp->alphas, parmv[2]))) {
        lydia_error(outfile, "Can not find an observation `%s'.", parmv[2]);
        return;
    }
    if (!gde_conflicts(outfile, dp, alpha)) {
        lydia_error(outfile, "Error allocating memory.");
    } else {
        lydia_ok(outfile, parmv[0]);
    }
}

static void cmd_diag(const void *UNUSED(context),
                     FILE *outfile,
                     const char **parmv,
                     const unsigned int UNUSED(parmc))
{
    diagnostic_problem dp;
    tv_term alpha;

    if (NULL == (dp = diagnostic_problem_get(parmv[1]))) {
        lydia_error(outfile, "Can not find a model `%s'.", parmv[1]);
        return;
    }
    if (NULL == (alpha = observation_get(dp->alphas, parmv[2]))) {
        lydia_error(outfile, "Can not find an observation `%s'.", parmv[2]);
        return;
    }
    if (!gde_diag(dp, alpha)) {
        lydia_error(outfile, "Error allocating memory.");
    } else {
        lydia_ok(outfile, parmv[0]);
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
            "General Diagnostic Solver.\n"
            "\n"
            "  -m, --mincard         generates only minimal cardinality diagnoses\n"
            "  -h, --help            display this help and exit\n"
            "  -v, --version         output version information and exit\n"
            "\n"
            "With no OUTPUT, or when OUTPUT is -, write to standard output.\n"
            "\n"
            "Report bugs to <lydia@fdir.org>.\n");
}

command commandtab[] = {
    { "diag",           cmd_diag,        2,  2, "compute diagnoses" },
    { "conflicts",      cmd_conflicts,   2,  2, "compute conflicts" },
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

    horn_flat_kb model = horn_flat_kbNIL;
    tv_dnf_hierarchy dump = tv_dnf_hierarchyNIL;

    observations alphas = NULL;
    diagnostic_problem ds = NULL;

    int rc = EXIT_SUCCESS;

    int option_index = 0;
    int option_mincard = 0;
    int c;

    struct option long_options[] = {
        {"mincard", 0, 0, 'm'},
		{"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "mh?v", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 'm':
                option_mincard = 1;
                break;
            case 'h':
                usage(stdout);
                return EXIT_SUCCESS;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                help(stdout);
                return EXIT_SUCCESS;
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
    if (obsfilename != NULL) {
        obsfile = ckfopen(obsfilename, "r");
    }
    if (outfilename != NULL && 0 != strcmp(outfilename, "-")) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();

    if (!fscan_horn_flat_kb(modelfile, &model)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                modelfilename,
                errmsg,
                lineno);
        rc = EXIT_FAILURE;
        goto exit;
    }
    if (!fscan_tv_dnf_hierarchy(obsfile, &dump)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                obsfilename,
                errmsg,
                lineno);
        rc = EXIT_FAILURE;
        goto exit;
    }

    if (horn_flat_kbNIL == model) {
        fprintf(stderr, "Nothing to diagnose.\n");
        rc = EXIT_FAILURE;
        goto exit;
    }
    if (material_implication_listNIL == to_horn(model->constraints)->clauses) {
        fprintf(stderr, "Trivially inconsistent model.\n");
        rc = EXIT_FAILURE;
        goto exit;
    }

    if (observations_load(to_hierarchy(dump),
                          model->constraints->variables,
                          &alphas)) {
        fprintf(stderr, "Error loading observations.\n");

        rc = EXIT_FAILURE;

        goto exit;
    }

    ds = diagnostic_problem_new(to_serializable(model));

    gde_init(ds, option_mincard);

    diagnostic_problem_add(model->name->name, ds, alphas);

    enable_statistics();
    enable_diagnosis();
    enable_listings();

    cli_init(MODULE, VERSION_MAJOR, VERSION_MINOR);
    cli_loop(ds, stdin, PROMPT, commandtab, outfile);
    cli_destroy();

    gde_destroy();

exit:
    diagnostic_problems_free();

    if (horn_flat_kbNIL != model) {
        rfre_horn_flat_kb(model);
    }
    if (tv_dnf_hierarchyNIL != dump) {
        rfre_tv_dnf_hierarchy(dump);
    }

    destroy_symtbl();

    return rc;
}
