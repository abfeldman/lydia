#include "tv.h"
#include "io.h"
#include "obs.h"
#include "defs.h"
#include "stat.h"
#include "util.h"
#include "cones.h"
#include "types.h"
#include "safari.h"
#include "config.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef MPI
# include <mpi.h>
#endif

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define PROMPT "safari> "
#define MODULE "safari"

int proc_id = 0;
int proc_count = 1;
#ifdef MPI
int  processor_name_size;
char processor_name[MPI_MAX_PROCESSOR_NAME];
#endif

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
    if (!safari_diag(outfile, dp, alpha)) {
        lydia_error(outfile, "Error allocating memory.");
    } else {
        lydia_ok(outfile, parmv[0]);
    }
}

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
    if (!safari_conflicts(dp, alpha)) {
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
            "Stochastic MBD search.\n"
            "\n"
            "Mandatory arguments to long options are mandatory for short options too.\n"
            "      --runs=INT        specify number of runs\n"
            "  -g, --greediness=INT  specify greediness factor\n"
            "  -s, --sat=SOLVER      specify SAT solver (lydia, minisat or none)\n"
            "                        lydia is the default SAT solver\n");
    fprintf(fp,
            "      --trivial         start from the all faulty solution\n"
            "  -V, --verbose         be verbose\n"
            "  -c, --cones=FILE      specify a cones file\n"
            "      --full-model=FILE specify a CNF with the original model\n"
            "  -h, --help            display this help and exit\n"
            "  -v, --version         output version information and exit\n"
            "\n"
            "With no OUTPUT, or when OUTPUT is -, write to standard output.\n"
            "\n"
            "Report bugs to <lydia@fdir.org>.\n");
}

static command commandtab[] = {
    { "diag",           cmd_diag,        2,  2, "compute diagnosis" },
    { "conflicts",      cmd_conflicts,   2,  2, "compute conflicts" },
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

    char *p = NULL;

    serializable model = serializableNIL;
    tv_cnf_flat_kb fullmodel = tv_cnf_flat_kbNIL;
    tv_dnf_hierarchy dump = tv_dnf_hierarchyNIL;

    observations alphas = NULL;
    diagnostic_problem full_ds = NULL;
    diagnostic_problem ds = NULL;

    char *alpha_name = NULL;
    const_tv_term alpha = NULL;

    cones_context cones = NULL;

    int rc = EXIT_SUCCESS;

    register unsigned int ix;

    char *option_observation = NULL;
    unsigned int option_runs = 8;
    unsigned int option_greediness = 0;
    unsigned int option_seed = (unsigned int)-1;
    int option_batch = 0;
    int option_verbose = 0;
    int option_trivial = 0;
    int option_sat = TAGsat_lydia;

    int option_index = 0;
    int c;

    signed char map_cones_rc;

    struct option long_options[] = {
        {"runs", required_argument, 0, 2},
        {"trivial", no_argument, 0, 3},
        {"seed", required_argument, NULL, 4},
        {"cones", required_argument, NULL, 'c'},
        {"full-model", required_argument, NULL, 'f'},
        {"batch", optional_argument, NULL, 'b'},
        {"greediness", required_argument, 0, 'g'},
        {"sat", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"verbose", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };


    option_seed = time(NULL) ^ getpid() ^ proc_id;

#ifdef MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
    MPI_Get_processor_name(processor_name, &processor_name_size);
#endif

    while ((c = getopt_long(argc, argv, "Vhvg:s:b::", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 2: /* runs */
                option_runs = strtoul(optarg, (char **)&p, 10);
                if (NULL == p || *p != '\0') {
                    fprintf(stderr, MODULE ": invalid number of runs: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 3: /* trivial */
                option_trivial = 1;
                break;
            case 4:
                option_seed = strtoul(optarg, (char **)&p, 10);
                if (NULL == p || *p != '\0') {
                    fprintf(stderr, "Invalid seed `%s'.\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'b':
                option_observation = optarg;
                option_batch = 1;
                break;
            case 'g':
                option_greediness = strtoul(optarg, (char **)&p, 10);
                if (NULL == p || *p != '\0') {
                    fprintf(stderr, MODULE ": invalid greediness: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 's':
                if (0 == strcmp(optarg, "lydia")) {
                    option_sat = TAGsat_lydia;
                } else if (0 == strcmp(optarg, "minisat")) {
                    option_sat = TAGsat_minisat;
                } else if (0 == strcmp(optarg, "none")) {
                    option_sat = TAGsat_none;
                } else {
                    fprintf(stderr,
                            "Valid arguments are:\n"
                            "  - `lydia'\n"
                            "  - `minisat'\n"
                            "  - `none'\n");
                    help(stderr);
                    return EXIT_FAILURE;
                }
                break;
            case 'c':
                option_cones = optarg;
                break;
            case 'f':
                option_full_model = optarg;
                break;
            case 'V':
                option_verbose = 1;
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

    if (argc < 3) {
        fprintf(stderr, "Missing mandatory model and observations.\n");
        help(stderr);
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

    srand(option_seed);

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
    if (!fscan_tv_dnf_hierarchy(obsfile, &dump)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                obsfilename,
                errmsg,
                lineno);
        rc = EXIT_FAILURE;
        goto exit;
    }

    if (serializableNIL == model) {
        fprintf(stderr, "Nothing to diagnose.\n");
        rc = EXIT_FAILURE;
        goto exit;
    }

    ds = diagnostic_problem_new(model);

    safari_init(ds,
                option_runs,
                option_greediness,
                option_sat,
                option_trivial);
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
        if (map_cones_rc != MAP_CONES_SUCCESS) {
            fprintf(stderr, "Error parsing `%s'.\n", option_cones);
            rc = EXIT_FAILURE;
            goto exit;
        }

        safari_set_cones(cones);
    }

    if (observations_load(to_hierarchy(dump),
                          ds->u.tv_cnf_sd->variables,
                          &alphas)) {
        fprintf(stderr, "Error loading observations.\n");

        rc = EXIT_FAILURE;

        goto exit;
    }

    diagnostic_problem_add(ds->name->name, ds, alphas);
    if (cones != NULL) {
        diagnostic_problem_add("$nil", full_ds, NULL);
    }

    enable_statistics();
    enable_diagnosis();
    enable_listings();

    if (option_batch) {
        if (option_observation != NULL) {
            alpha_name = option_observation;
            if (NULL == (alpha = observation_get(ds->alphas, alpha_name))) {
                fprintf(stderr, "Error loading observation `%s'.\n", alpha_name);

                rc = EXIT_FAILURE;
                goto exit;
            }

            if (!safari_diag(outfile, ds, alpha)) {
                lydia_error(outfile, "Error allocating memory.");

                rc = EXIT_FAILURE;
                goto exit;
            }
            ds->fm(outfile,
                   option_cones != NULL ? full_ds : ds,
                   proc_id == 0 ? pp_fm : NULL);
            ds->cards(outfile,
                      option_cones != NULL ? full_ds : ds,
                      proc_id == 0 ? pp_card : NULL);
        } else {
            for (ix = 0; ix < ds->alphas->names->sz; ix++) {
                alpha_name = ((lydia_symbol)ds->alphas->names->arr[ix])->name;

                if (NULL == (alpha = observation_get(ds->alphas, alpha_name))) {
                    fprintf(stderr, "Internal error loading observation `%s'.\n", alpha_name);

                    rc = EXIT_FAILURE;
                    goto exit;
                }
                fprintf(outfile, "# alpha=`%s'\n", alpha_name);
                if (!safari_diag(outfile, ds, alpha)) {
                    lydia_error(outfile, "Error allocating memory.");

                    rc = EXIT_FAILURE;
                    goto exit;
                }
                ds->fm(outfile,
                       option_cones != NULL ? full_ds : ds,
                       proc_id == 0 ? pp_fm : NULL);
                ds->cards(outfile,
                          option_cones != NULL ? full_ds : ds,
                          proc_id == 0 ? pp_card : NULL);
            }
        }
    } else {
        cli_init(MODULE, VERSION_MAJOR, VERSION_MINOR);
        cli_loop(option_cones != NULL ? full_ds : ds, stdin, PROMPT, commandtab, outfile);
        cli_destroy();
    }

    if (option_verbose && proc_id == 0) {
        display_stat_items(stderr);
    }

    safari_destroy();

exit:
    diagnostic_problems_free();

    if (serializableNIL != model) {
        rfre_serializable(model);
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

#ifdef MPI
    MPI_Finalize();
#endif

    return rc;
}
