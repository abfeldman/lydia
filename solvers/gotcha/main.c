#include "tv.h"
#include "io.h"
#include "obs.h"
#include "defs.h"
#include "util.h"
#include "stat.h"
#include "cones.h"
#include "strdup.h"
#include "config.h"
#include "inline.h"
#include "gotcha.h"
#include "dnf_tree.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef WIN32
# include <sys/time.h>
#else
# include <Winsock2.h>
#endif
#include <signal.h>

#define VERSION_MAJOR 2
#define VERSION_MINOR 0
#define PROMPT "gotcha> "
#define MODULE "gotcha"

static void cmd_diag(const void *UNUSED(context),
                     FILE *outfile,
                     const char **parmv,
                     const unsigned int UNUSED(parmc))
{
    diagnostic_problem ds;
    tv_term alpha;

    if (NULL == (ds = diagnostic_problem_get(parmv[1]))) {
        lydia_error(outfile, "Can not find a model `%s'.", parmv[1]);
        return;
    }
    if (NULL == (alpha = observation_get(ds->alphas, parmv[2]))) {
        lydia_error(outfile, "Can not find an observation `%s'.", parmv[2]);
        return;
    }
    if (!gotcha_diag(ds, alpha)) {
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
            "cDNF diagnostic engine.\n"
            "\n"
            "  -c, --cones=FILE      specify a cones file\n"
            "      --full-model=FILE specify a CNF with the original model\n"
            "  -h, --help            display this help and exit\n"
            "  -v, --version         output version information and exit\n"
            "\n"
            "With no OUTPUT, or when OUTPUT is -, write to standard output.\n"
            "\n"
            "Report bugs to <lydia@fdir.org>.\n");
}

command commandtab[] = {
    { "diag",           cmd_diag,        2,  2, "compute diagnosis" },
    { NULL,             NULL,           -1, -1, NULL }  /* End of the table. */
};

int main(int argc, char **argv)
{
    FILE *fullmodelfile = NULL;
    FILE *modelfile = NULL;
    FILE *conesfile = NULL;
    FILE *obsfile = NULL;
    FILE *outfile = stdout;

    char *modelfilename = NULL;
    char *obsfilename = NULL;
    char *outfilename = NULL;

    char *option_cones = NULL;
    char *option_full_model = NULL;

    tv_dnf_hierarchy model = tv_dnf_hierarchyNIL;
    tv_cnf_flat_kb fullmodel = tv_cnf_flat_kbNIL;
    tv_dnf_hierarchy dump = tv_dnf_hierarchyNIL;
    node root;

    observations alphas = NULL;
    diagnostic_problem full_ds = NULL;
    diagnostic_problem ds = NULL;

    cones_context cones = NULL;

    int rc = EXIT_SUCCESS;

    int option_index = 0;
    int c;

    signed char map_cones_rc;

    struct option long_options[] = {
        {"cones", required_argument, NULL, 'c'},
        {"full-model", required_argument, NULL, 'f'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {0, 0, NULL, 0}
    };

    while ((c = getopt_long(argc, argv, "c:h?v", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 'c':
                option_cones = optarg;
                break;
            case 'f':
                option_full_model = optarg;
                break;
            case 'h':
                usage(stdout);
                return EXIT_SUCCESS;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                help(stdout);
                return EXIT_FAILURE;
        }
    }

    if (argc < 3) {
        fprintf(stderr, "Missing mandatory model and observations.\n");
        help(stdout);
        return EXIT_FAILURE;
    }

    if ((option_cones == NULL && option_full_model != NULL) ||
        (option_cones != NULL && option_full_model == NULL)) {
        fprintf(stderr, "A cones file needs a full model and vice versa.\n");
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

    if (!fscan_tv_dnf_hierarchy(modelfile, &model)) {
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

    if (nodeNIL == (root = find_root_node(to_hierarchy(model)))) {
        fprintf(stderr, "Nothing to diagnose.\n");
        rc = EXIT_FAILURE;
        goto exit;
    }

    ds = diagnostic_problem_new(to_serializable(model));

    gotcha_init(ds, model, root);
    if (option_cones != NULL) {
        assert(option_full_model != NULL);

        conesfile = ckfopen(option_cones, "r");
        fullmodelfile = ckfopen(option_full_model, "r");

        if (!fscan_tv_cnf_flat_kb(fullmodelfile, &fullmodel)) {
            fprintf(stderr,
                    "Error parsing `%s': %s in line %d.\n",
                    option_full_model,
                    errmsg,
                    lineno);
            rc = EXIT_FAILURE;
            goto exit;
        }

        full_ds = diagnostic_problem_new(to_serializable(fullmodel));
        cones = map_cones(conesfile, ds, full_ds, &map_cones_rc);
        /* @todo: error handling */

        gotcha_set_cones(cones);
    }

    if (observations_load(to_hierarchy(dump), ds->variables, &alphas)) {
        fprintf(stderr, "Error loading observations.\n");

        rc = EXIT_FAILURE;

        goto exit;
    }

    diagnostic_problem_add(root->type->name, ds, alphas);
    if (cones != NULL) {
        diagnostic_problem_add("$nil", full_ds, NULL);
    }

    enable_statistics(); 
    enable_diagnosis();
    enable_listings();

    cli_init(MODULE, VERSION_MAJOR, VERSION_MINOR);
    cli_loop(option_cones != NULL ? full_ds : ds, stdin, PROMPT, commandtab, outfile);
    cli_destroy();

    gotcha_destroy(ds);

exit:
    diagnostic_problems_free();

    if (tv_dnf_hierarchyNIL != model) {
        rfre_tv_dnf_hierarchy(model);
    }
    if (tv_cnf_flat_kbNIL != fullmodel) {
        rfre_tv_cnf_flat_kb(fullmodel);
    }
    if (tv_dnf_hierarchyNIL != dump) {
        rfre_tv_dnf_hierarchy(dump);
    }

    if (NULL != cones) {
        cones_context_free(cones);
    }

    destroy_symtbl();

    return rc;
}
