#include "findbin.h"
#include "flat_kb.h"
#include "fprint.h"
#include "config.h"
#include "strdup.h"
#include "inline.h"
#include "error.h"
#include "util.h"
#include "oc.h"

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
#define MODULE "oc"

static char **include_path = NULL;
static unsigned int include_path_size = 0;

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
            "Compile the SOURCE LYDIA observation and write the result to DEST, or standard\n"
            "input, to standard output.\n"
            "\n"
            "  -I<dir>              add <dir> to the directory search path\n"
            "  -h, --help           display this help and exit\n"
            "  -v, --version        output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <lydia@fdir.org>.\n");
}

int main(int argc, char **argv)
{
    FILE *modelfile = NULL;
    FILE *obsfile = NULL;
    FILE *outfile = NULL;

    char *prefix = NULL;

    char *modelfilename = "-";
    char *obsfilename = "-";
    char *outfilename = "-";

    csp_hierarchy output;
    serializable model;
    print_state st;

    node root;

    values_set_list domains = values_set_listNIL;
    variable_list variables = variable_listNIL;
    variable_list encoded_variables = variable_listNIL;

    int_list_list variable_mappings;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"version", 0, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "hvI:", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 'I':
                add_include_path(optarg);
                break;
            case '?':
            case 'h':
                usage(stdout);
                return EXIT_FAILURE;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
            default:
                help(stdout);
                return EXIT_SUCCESS;
        }
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

    obsfile = stdin;
    outfile = stdout;

    if (optind < argc) {
        modelfilename = argv[optind++];
    }
    if (optind < argc) {
        obsfilename = argv[optind++];
    }
    if (optind < argc) {
        outfilename = argv[optind++];
    }

    if (0 == strcmp(modelfilename, "-") && 0 == strcmp(obsfilename, "-")) {
/* To Do: To normal error. */
        assert(0);
        abort();
    }

    if (NULL == (modelfile = fopen(modelfilename, "r"))) {
        loeh_error(COMPILER_OPEN_ERROR,
                   LOEH_LOCATION_GLOBAL,
                   obs_origNIL,
                   modelfilename,
                   strerror(errno));
        return EXIT_FAILURE;
    }
    if (0 != strcmp(obsfilename, "-")) {
        if (NULL == (obsfile = fopen(obsfilename, "r"))) {
            loeh_error(COMPILER_OPEN_ERROR,
                       LOEH_LOCATION_GLOBAL,
                       obs_origNIL,
                       obsfilename,
                       strerror(errno));
            return EXIT_FAILURE;
        }
    }
    if (0 != strcmp(outfilename, "-")) {
        if (NULL == (outfile = fopen(outfilename, "w"))) {
            loeh_error(COMPILER_OPEN_ERROR,
                       LOEH_LOCATION_GLOBAL,
                       obs_origNIL,
                       outfilename,
                       strerror(errno));
            return EXIT_FAILURE;
        }
    }

    init_symtbl();

    if (!fscan_serializable(modelfile, &model)) {
        fprintf(stderr, "%s:%d: %s\n", modelfilename, lineno, errmsg);

        free_include_paths();
        free(prefix);

        destroy_symtbl();

        fclose(modelfile);
        fclose(obsfile);
        fclose(outfile);

        return EXIT_FAILURE;
    }
    switch (model->tag) {
        case TAGcsp_hierarchy:
        case TAGmv_wff_hierarchy:
        case TAGmv_cnf_hierarchy:
        case TAGmv_dnf_hierarchy:
        case TAGtv_wff_hierarchy:
        case TAGtv_nnf_hierarchy:
        case TAGtv_cnf_hierarchy:
        case TAGtv_dnf_hierarchy:
        case TAGhorn_hierarchy:
        case TAGobdd_hierarchy:
        case TAGmdd_hierarchy:
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

            if (nodeNIL == (root = find_root_node(to_hierarchy(model)))) {
/* To Do: Convert this to an error message. */
                assert(0);
                abort();
            }

            inline_variables(to_hierarchy(model),
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
            break;
        case TAGcsp_flat_kb:
        case TAGhorn_flat_kb:
        case TAGmv_wff_flat_kb:
        case TAGmv_cnf_flat_kb:
        case TAGmv_dnf_flat_kb:
        case TAGtv_wff_flat_kb:
        case TAGtv_nnf_flat_kb:
        case TAGtv_cnf_flat_kb:
        case TAGtv_dnf_flat_kb:
        case TAGobdd_flat_kb:
        case TAGmdd_flat_kb:
            domains = to_flat_kb(model)->constraints->domains;
            variables = to_flat_kb(model)->constraints->encoding == ENCODING_NONE ?
                        to_flat_kb(model)->constraints->variables :
                        to_flat_kb(model)->constraints->encoded_variables;
            break;
    }

    if (!oc(obsfile,
            obsfilename,
            include_path,
            include_path_size,
            domains,
            variables,
            &output)) {
        free_include_paths();
        free(prefix);

        destroy_symtbl();

        fclose(modelfile);
        fclose(obsfile);
        fclose(outfile);

        return EXIT_FAILURE;
    }

    st = set_print(outfile, 1, 75, 8);
    fprint_csp_hierarchy(st, output);
    end_print(st);

    switch (model->tag) {
        case TAGcsp_hierarchy:
        case TAGmv_wff_hierarchy:
        case TAGmv_cnf_hierarchy:
        case TAGmv_dnf_hierarchy:
        case TAGtv_wff_hierarchy:
        case TAGtv_nnf_hierarchy:
        case TAGtv_cnf_hierarchy:
        case TAGtv_dnf_hierarchy:
        case TAGobdd_hierarchy:
        case TAGmdd_hierarchy:
            rfre_values_set_list(domains);
            if (encoded_variables != variables) {
                rfre_variable_list(encoded_variables);
            }
            rfre_variable_list(variables);
            break;
        default:
            break;
    }

    rfre_serializable(model);
    rfre_csp_hierarchy(output);

    free_include_paths();
    free(prefix);

    destroy_symtbl();

    fclose(modelfile);
    fclose(obsfile);
    fclose(outfile);

    return EXIT_SUCCESS;
}
