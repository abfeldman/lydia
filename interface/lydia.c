#include "pp_variable.h"
#include "lcm2mvwff.h"
#include "wff2obdd.h"
#include "obdd2dnf.h"
#include "findbin.h"
#include "cnf2dnf.h"
#include "wff2cnf.h"
#include "lcm2wff.h"
#include "smoothy.h"
#include "inline.h"
#include "scotty.h"
#include "gotcha.h"
#include "config.h"
#include "strdup.h"
#include "safari.h"
#include "error.h"
#include "util.h"
#include "defs.h"
#include "cdas.h"
#include "obdd.h"
#include "lsim.h"
#include "hash.h"
#include "ast.h"
#include "lcm.h"
#include "dec.h"
#include "enc.h"
#include "obs.h"
#include "lc.h"
#include "io.h"
#include "oc.h"
#include "mv.h"
#include "tv.h"

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#ifdef MPI
# include <mpi.h>
#endif

#define VERSION_MAJOR 2
#define VERSION_MINOR 0
#define PROMPT "lydia> "
#define MODULE "lydia"

int proc_id = 0;
int proc_count = 1;
#ifdef MPI
int  processor_name_size;
char processor_name[MPI_MAX_PROCESSOR_NAME];
#endif

static char **include_path = NULL;
static unsigned int include_path_size = 0;

#define MODE_CDAS   1
#define MODE_SCOTTY 2
#define MODE_GOTCHA 3
#define MODE_LSIM   4
#define MODE_SAFARI 5

static int encoding = ENCODING_SPARSE;

static int is_multivalued(const_csp_hierarchy lcm)
{
    register unsigned int ix, iy;

    for (ix = 0; ix < lcm->nodes->sz; ix++) {
        for (iy = 0; iy < lcm->nodes->arr[ix]->constraints->variables->sz; iy++) {
            if (TAGenum_variable == lcm->nodes->arr[ix]->constraints->variables->arr[iy]->tag) {
                return 1;
            }
        }
    }

    return 0;
}

tv_wff_hierarchy lcm2tv_wff(const_csp_hierarchy lcm,
                            const int option_multivalued,
                            const int option_encoding)
{
    mv_wff_hierarchy hmvwff = mv_wff_hierarchyNIL;
    tv_wff_hierarchy htvwff = tv_wff_hierarchyNIL;

    if (option_multivalued) {
        if (!lcm2mvwff(lcm, &hmvwff)) {
            return NULL;
        }
        mvwff2tvwff(hmvwff, &htvwff, option_encoding);
        rfre_mv_wff_hierarchy(hmvwff);
    } else {
        if (!lcm2wff(lcm, &htvwff)) {
            return NULL;
        }
    }

    return htvwff;
}

tv_cnf_flat_kb lcm2tv_cnf(const_csp_hierarchy lcm,
                          const int option_multivalued,
                          const int option_encoding,
                          const int option_verbose,
                          const int option_simplify)
{
    tv_wff_hierarchy htvwff = tv_wff_hierarchyNIL;
    tv_cnf_hierarchy htvcnf = tv_cnf_hierarchyNIL;
    tv_cnf_flat_kb tvcnf = tv_cnf_flat_kbNIL;

    if (NULL == (htvwff = lcm2tv_wff(lcm,
                                     option_multivalued,
                                     option_encoding))) {
        return NULL;
    }
    if (!wff2cnf((const_serializable)htvwff,
                 (void *) /* (serializable *) */ &htvcnf)) {
        return NULL;
    }

    rfre_tv_wff_hierarchy(htvwff);

    smoothy_init();
    if (!smoothy((const_serializable)htvcnf,
                 0, /* hierarchical flattening */
                 1, /* depth for hierarchical flattening */
                 option_verbose,
                 option_simplify,
                 (void *) /* (serializable *) */ &tvcnf)) {
        return NULL;
    }

    rfre_tv_cnf_hierarchy(htvcnf);

    return tvcnf;
}

tv_dnf_flat_kb lcm2tv_dnf(const_csp_hierarchy lcm,
                          const int option_multivalued,
                          const int option_encoding,
                          const int option_verbose,
                          const int option_simplify)
{
    tv_wff_hierarchy htvwff = tv_wff_hierarchyNIL;
    tv_wff_flat_kb tvwff = tv_wff_flat_kbNIL;
    obdd_flat_kb obdd = obdd_flat_kbNIL;
    tv_dnf_flat_kb tvdnf = tv_dnf_flat_kbNIL;

    if (NULL == (htvwff = lcm2tv_wff(lcm,
                                     option_multivalued,
                                     option_encoding))) {
        return NULL;
    }

    smoothy_init();
    if (!smoothy((const_serializable)htvwff,
                 0, /* hierarchical flattening */
                 1, /* depth for hierarchical flattening */
                 option_verbose,
                 option_simplify,
                 (void *) /* (serializable *) */ &tvwff)) {
        return NULL;
    }

    rfre_tv_wff_hierarchy(htvwff);

    wff2obdd_init();
    wff2obdd((const_serializable)tvwff,
             (void *) /* (serializable *) */ &obdd,
             0 /* brute force */);
    wff2obdd_destroy();

    rfre_tv_wff_flat_kb(tvwff);

    obdd2dnf((const_serializable)obdd,
             (void *) /* (serializable *) */ &tvdnf);

    rfre_obdd_flat_kb(obdd);

    return tvdnf;
}

tv_dnf_hierarchy lcm2tv_hdnf(const_csp_hierarchy lcm,
                             const int option_multivalued,
                             const int option_encoding)
{
    tv_wff_hierarchy htvwff = tv_wff_hierarchyNIL;
    obdd_hierarchy hobdd = obdd_hierarchyNIL;
    tv_dnf_hierarchy htvdnf = tv_dnf_hierarchyNIL;

    if (NULL == (htvwff = lcm2tv_wff(lcm, option_multivalued, option_encoding))) {
        return NULL;
    }

    wff2obdd_init();
    wff2obdd((const_serializable)htvwff,
             (void *) /* (serializable *) */ &hobdd,
             0 /* brute force */);
    wff2obdd_destroy();

    rfre_tv_wff_hierarchy(htvwff);

    obdd2dnf((const_serializable)hobdd,
             (void *) /* (serializable *) */ &htvdnf);

    rfre_obdd_hierarchy(hobdd);

    return htvdnf;
}

static void cmd_lsim_sim(const void *UNUSED(context),
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

static void cmd_gotcha_diag(const void *UNUSED(context),
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
    if (!gotcha_diag(dp, alpha)) {
        lydia_error(outfile, "Error allocating memory.");
    } else {
        lydia_ok(outfile, parmv[0]);
    }
}

static void cmd_scotty_diag(const void *UNUSED(context),
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
    if (!scotty_diag(dp, alpha)) {
        lydia_error(outfile, "Error allocating memory.");
    } else {
        lydia_ok(outfile, parmv[0]);
    }
}

static void cmd_cdas_diag(const void *UNUSED(input),
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
    if (!cdas_diag(dp, alpha)) {
        lydia_error(outfile, "Error allocating memory.");
    } else {
        lydia_ok(outfile, parmv[0]);
    }
}

static void cmd_safari_diag(const void *UNUSED(input),
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

command gotcha_commands[] = {
    { "diag",           cmd_gotcha_diag,         2,  2, "compute diagnosis" },
    { NULL,             NULL,                   -1, -1, NULL }  /* End of the table. */
};

command scotty_commands[] = {
    { "diag",           cmd_scotty_diag,         2,  2, "compute diagnosis" },
    { NULL,             NULL,                   -1, -1, NULL }  /* End of the table. */
};

command cdas_commands[] = {
    { "diag",           cmd_cdas_diag,           2,  2, "compute diagnosis" },
    { NULL,             NULL,                   -1, -1, NULL }  /* End of the table. */
};

command safari_commands[] = {
    { "diag",           cmd_safari_diag,         2,  2, "compute diagnosis" },
    { NULL,             NULL,                   -1, -1, NULL }  /* End of the table. */
};

command lsim_commands[] = {
    { "sim",            cmd_lsim_sim,            2,  2, "simulate the system" },
    { NULL,             NULL,                   -1, -1, NULL }  /* End of the table. */
};

static int add_include_path(char *path)
{
    char **p = (char **)realloc(include_path, (include_path_size + 1) * sizeof(char *));
    if (NULL == p) {
        return 0;
    }
    include_path = p;
    include_path[include_path_size] = strdup(path);
    include_path_size += 1;

    return 1;
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
    fprintf(fp, "lydia v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void help(FILE *fp)
{
    fprintf(fp, "Try `" MODULE " -h' for more information.\n");
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: lydia [OPTION] MODEL OBSERVATIONS\n"
            "  or:  lydia\n"
            "Invoke the LYDIA command line interface.\n"
            "\n"
            "  -t, --gotcha               use tDNF search\n"
            "  -s, --scotty               perform full DNF conversion\n"
            "  -f, --safari               use stochastic search\n"
            "  -l, --simulation           run Lydia in simulation mode\n"
            "  -d, --dense                use dense instead of sparse encodings\n"
            "      --skip-simplification  skip Boolean CNF/DNF simplification\n");
    fprintf(fp,
            "  -I<dir>                    add <dir> to the components search path\n"
            "  -V, --verbose              increase verbosity\n"
            "      --tries=INT            specify number of tries\n"
            "  -g, --greediness=INT       specify greediness factor\n"
            "  -h, --help                 display this help and exit\n"
            "  -v, --version              output version information and exit\n"
            "\n"
            "Report bugs to <alex@fdir.org>.\n");
}

int main(int argc, char **argv)
{
    FILE *modelfile = NULL;
    FILE *obsfile = NULL;
    FILE *outfile = stdout;

    char *modelfilename = NULL;
    char *obsfilename = NULL;
    char *outfilename = NULL;

    char *prefix = NULL;
    char *p = NULL;

    node root;

    values_set_list domains = values_set_listNIL;
    variable_list variables = variable_listNIL;
    variable_list encoded_variables = variable_listNIL;

    int_list_list variable_mappings;

    csp_hierarchy lcm = csp_hierarchyNIL;
    csp_hierarchy lco = csp_hierarchyNIL;
    tv_dnf_hierarchy hdnf = tv_dnf_hierarchyNIL;
    tv_dnf_hierarchy dump = tv_dnf_hierarchyNIL;
    tv_dnf_flat_kb dnf = tv_dnf_flat_kbNIL;
    tv_cnf_flat_kb cnf = tv_cnf_flat_kbNIL;
    node hdnf_root = nodeNIL;

    observations alphas = NULL;
    diagnostic_problem ds = NULL;

    int mode = MODE_CDAS;

    int multivalued = 0;

    unsigned int option_tries = 8;
    unsigned int option_greediness = 0;
    int option_sat = TAGsat_lydia;
    int option_verbose = 0;
    int option_simplify = 1;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"bcp", 0, 0, 'b'},
        {"gotcha", 0, 0, 't'},
        {"scotty", 0, 0, 's'},
        {"safari", 0, 0, 'f'},
        {"simulation", 0, 0, 'l'},
        {"dense", 0, 0, 'd'},
        {"skip-simplification", 0, 0, 1},
        {"tries", 1, 0, 2},
        {"greediness", 1, 0, 'g'},
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc,
                            argv,
                            "fbtsldI:Vh?v",
                            long_options,
                            &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 1:
                option_simplify = 0;
                break;
            case 2: /* tries */
                option_tries = strtoul(optarg, (char **)&p, 10);
                if (NULL == p || *p != '\0') {
                    fprintf(stderr, MODULE ": invalid number of tries: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'g':
                option_greediness = strtoul(optarg, (char **)&p, 10);
                if (NULL == p || *p != '\0') {
                    fprintf(stderr, MODULE ": invalid greediness: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'b':
                option_sat = TAGsat_none;
                break;
            case 'd':
                encoding = ENCODING_DENSE;
                break;
            case 'l':
                mode = MODE_LSIM;
                break;
            case 's':
                mode = MODE_SCOTTY;
                break;
            case 't':
                mode = MODE_GOTCHA;
                break;
            case 'f':
                mode = MODE_SAFARI;
                break;
            case 'I':
                add_include_path(optarg);
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
                fprintf(stderr, "Try `lydia -h' for more information.\n");
                return EXIT_FAILURE;
        }
    }

    if (argc < 3) {
        fprintf(stderr, "Missing mandatory model and observations.\n");
        help(stdout);
        return EXIT_FAILURE;
    }

    if (NULL == (prefix = strdup(findbin(argv[0], 1)))) {
/* To Do: To normal error. */
        assert(0);
        abort();
    }

    if (!add_include_path(canonify(prefix, "components"))) {
/* To Do: To normal error. */
        assert(0);
        abort();
    }
    if (!add_include_path(canonify(prefix, "share/lydia/components"))) {
/* To Do: To normal error. */
        assert(0);
        abort();
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

    if (NULL == (modelfile = fopen(modelfilename, "r"))) {
        leh_error(COMPILER_OPEN_ERROR,
                  LEH_LOCATION_GLOBAL,
                  originNIL,
                  modelfilename,
                  strerror(errno));
        return EXIT_FAILURE;
    }
    if (NULL == (obsfile = fopen(obsfilename, "r"))) {
        leh_error(COMPILER_OPEN_ERROR,
                  LEH_LOCATION_GLOBAL,
                  originNIL,
                  obsfilename,
                  strerror(errno));
        return EXIT_FAILURE;
    }

    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    enable_statistics();
    enable_diagnosis();
    enable_listings();

    init_symtbl();

    if (!lc(modelfile,
            modelfilename,
            include_path,
            include_path_size,
            0,
            &lcm)) {
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    if (int_list_listNIL == (variable_mappings = new_int_list_list())) {
/* To Do: Error handling. */
        return EXIT_FAILURE;
    }

    if (values_set_listNIL == (domains = new_values_set_list())) {
        rfre_int_list_list(variable_mappings);
    }
    if (variable_listNIL == (variables = new_variable_list())) {
        rfre_int_list_list(variable_mappings);
        rfre_values_set_list(domains);
    }
    if (variable_listNIL == (encoded_variables = new_variable_list())) {
        rfre_int_list_list(variable_mappings);
        rfre_values_set_list(domains);
        rfre_variable_list(encoded_variables);
    }

    if (nodeNIL == (root = find_root_node(to_hierarchy(lcm)))) {
/* To Do: Convert this to an error message. */
        assert(0);
        abort();
    }

    inline_variables(to_hierarchy(lcm),
                     root,
                     mapping_listNIL,
                     domains,
                     variables,
                     encoded_variables,
                     constant_listNIL,
                     variable_mappings,
                     int_list_listNIL,
                     NULL);

    rfre_int_list_list(variable_mappings);

    if (root->constraints->encoding != ENCODING_NONE) {
        rfre_variable_list(variables);
        variables = encoded_variables;
    }

    if (!oc(obsfile,
            obsfilename,
            include_path,
            include_path_size,
            domains,
            variables,
            &lco)) {
        free_include_paths();
        free(prefix);

        destroy_symtbl();

        fclose(modelfile);
        fclose(obsfile);
        fclose(outfile);

        return EXIT_FAILURE;
    }

    free(prefix);

    free_include_paths();

    rfre_values_set_list(domains);
    rfre_variable_list(encoded_variables);
    rfre_variable_list(variables);

    multivalued = is_multivalued(lcm);

/* Observations. */
    if (NULL == (dump = lcm2tv_hdnf(lco, multivalued, encoding))) {
/* To Do: Normal error handling. */
        assert(0);
        abort();

        return EXIT_FAILURE;
    }
    if (csp_hierarchyNIL != lco) {
        rfre_csp_hierarchy(lco);
    }

/* Model. */
    if (mode == MODE_GOTCHA) {
        if (NULL == (hdnf = lcm2tv_hdnf(lcm, multivalued, encoding))) {
            /* @todo: Free resources. */

            return EXIT_FAILURE;
        }

        hdnf_root = find_root_node(to_hierarchy(hdnf));
        assert(hdnf_root != nodeNIL);

        ds = diagnostic_problem_new((const_serializable)hdnf);
        gotcha_init(ds, hdnf, hdnf_root);

        if (observations_load(to_hierarchy(dump), ds->variables, &alphas)) {
            fprintf(stderr, "Error loading observations.\n");

            /* @todo: Free resources. */

            return EXIT_FAILURE;
        }
    }

    if (mode == MODE_SCOTTY) {
        if (NULL == (dnf = lcm2tv_dnf(lcm,
                                      multivalued,
                                      encoding,
                                      option_verbose,
                                      option_simplify))) {

            /* @todo: Free resources. */

            return EXIT_FAILURE;
        }

        if (observations_load(to_hierarchy(dump),
                              to_tv_cnf_flat_kb(dnf)->constraints->variables,
                              &alphas)) {
            fprintf(stderr, "Error loading observations.\n");

            /* @todo: Free resources. */

            return EXIT_FAILURE;
        }
    }

    if (mode == MODE_CDAS || mode == MODE_LSIM || mode == MODE_SAFARI) {
        if (NULL == (cnf = lcm2tv_cnf(lcm,
                                      multivalued,
                                      encoding,
                                      option_verbose,
                                      option_simplify))) {
            /* @todo: Free resources. */

            return EXIT_FAILURE;
        }

        if (observations_load(to_hierarchy(dump),
                              to_tv_cnf_flat_kb(cnf)->constraints->variables,
                              &alphas)) {
            fprintf(stderr, "Error loading observations.\n");

            /* @todo: Free resources. */

            return EXIT_FAILURE;
        }
    }

    if (tv_dnf_hierarchyNIL != dump) {
        rfre_tv_dnf_hierarchy(dump);
    }
    if (csp_hierarchyNIL != lcm) {
        rfre_csp_hierarchy(lcm);
    }

    cli_init(MODULE, VERSION_MAJOR, VERSION_MINOR);
    switch (mode) {
        case MODE_SCOTTY:
            ds = diagnostic_problem_new((const_serializable)dnf);
            scotty_init(ds);

            diagnostic_problem_add(dnf->name->name, ds, alphas);

            cli_loop(ds, stdin, PROMPT, scotty_commands, outfile);
            scotty_destroy();

            rfre_tv_dnf_flat_kb(dnf);
            break;
        case MODE_CDAS:
            ds = diagnostic_problem_new((const_serializable)cnf);
            cdas_init(ds, 1, 0, 1, 0);

            diagnostic_problem_add(cnf->name->name, ds, alphas);

            cli_loop(ds, stdin, PROMPT, cdas_commands, outfile);
            cdas_destroy();

            rfre_tv_cnf_flat_kb(cnf);
            break;
        case MODE_SAFARI:
            ds = diagnostic_problem_new((const_serializable)cnf);
            safari_init(ds,
                        option_tries,
                        option_greediness,
                        option_sat,
                        0);

            diagnostic_problem_add(cnf->name->name, ds, alphas);

            cli_loop(ds, stdin, PROMPT, safari_commands, outfile);
            safari_destroy();

            rfre_tv_cnf_flat_kb(cnf);
            break;
        case MODE_LSIM:
            ds = diagnostic_problem_new((const_serializable)cnf);
            lsim_init(ds, (const_serializable)cnf);

            diagnostic_problem_add(cnf->name->name, ds, alphas);

            cli_loop(cnf, stdin, PROMPT, lsim_commands, outfile);
            lsim_destroy(ds);

            rfre_tv_cnf_flat_kb(cnf);
            break;
        case MODE_GOTCHA:
            diagnostic_problem_add(hdnf_root->type->name, ds, alphas);

            cli_loop(ds, stdin, PROMPT, gotcha_commands, outfile);
            gotcha_destroy(ds);

            rfre_tv_dnf_hierarchy(hdnf);
            break;
    }
    cli_destroy();

    diagnostic_problems_free();

    destroy_symtbl();

    return EXIT_SUCCESS;
}
