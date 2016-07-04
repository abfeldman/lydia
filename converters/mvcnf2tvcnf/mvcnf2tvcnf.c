#include "hierarchy.h"
#include "variable.h"
#include "wff2cnf.h"
#include "strdup.h"
#include "config.h"
#include "fprint.h"
#include "sparse.h"
#include "dense.h"
#include "util.h"
#include "enc.h"
#include "mv.h"
#include "tv.h"

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

static mv_wff_expr mv_clause_to_mv_wff_expr(const_mv_clause clause)
{
    mv_wff_expr result;

    register unsigned int ix;

    result = mv_wff_exprNIL;
    for (ix = 0; ix < clause->neg->sz; ix++) {
        mv_literal literal = clause->neg->arr[ix];
        mv_wff_e_not expr = new_mv_wff_e_not(to_mv_wff_expr(new_mv_wff_e_var(literal->var, literal->val)));
        if (result == mv_wff_exprNIL) {
            result = to_mv_wff_expr(expr);
            continue;
        }
        result = to_mv_wff_expr(new_mv_wff_e_or(result, to_mv_wff_expr(expr)));
    }
    for (ix = 0; ix < clause->pos->sz; ix++) {
        mv_literal literal = clause->pos->arr[ix];
        mv_wff_e_var expr = new_mv_wff_e_var(literal->var, literal->val);
        if (result == mv_wff_exprNIL) {
            result = to_mv_wff_expr(expr);
            continue;
        }
        result = to_mv_wff_expr(new_mv_wff_e_or(result, to_mv_wff_expr(expr)));
    }

    return result;
}

static mv_wff mv_cnf_to_mv_wff(const_mv_cnf cnf)
{
    mv_wff result;
    mv_clause clause;
    mv_wff_expr expr;

    register unsigned int ix;

    result = new_mv_wff(rdup_values_set_list(cnf->domains),
                        rdup_variable_list(cnf->variables),
                        rdup_variable_list(cnf->encoded_variables),
                        rdup_constant_list(cnf->constants),
                        cnf->encoding,
                        new_mv_wff_expr_list());

    for (ix = 0; ix < cnf->clauses->sz; ix++) {
        clause = cnf->clauses->arr[ix];
        expr = mv_clause_to_mv_wff_expr(clause);
        append_mv_wff_expr_list(result->e, expr);
    }

    return result;
}

tv_cnf sparse_encode_cnf(const_mv_cnf mv_cnf)
{
    mv_wff mv_wff;
    tv_wff tv_wff;
    tv_cnf tv_cnf;

    mv_wff = mv_cnf_to_mv_wff(mv_cnf);
    tv_wff = sparse_encode_wff(mv_wff);
    tv_cnf = tv_wff_to_tv_cnf(tv_wff);

    rfre_tv_wff(tv_wff);
    rfre_mv_wff(mv_wff);

    return tv_cnf;
}

tv_cnf dense_encode_cnf(const_mv_cnf mv_cnf)
{
    mv_wff mv_wff;
    tv_wff tv_wff;
    tv_cnf tv_cnf;

    mv_wff = mv_cnf_to_mv_wff(mv_cnf);
    tv_wff = dense_encode_wff(mv_wff);
    tv_cnf = tv_wff_to_tv_cnf(tv_wff);

    rfre_tv_wff(tv_wff);
    rfre_mv_wff(mv_wff);

    return tv_cnf;
}

int mvcnf2tvcnf(const_mv_cnf_hierarchy input,
                tv_cnf_hierarchy *output,
                const int encoding)
{
    register unsigned int ix;

    mv_cnf node_input;
    tv_cnf node_output;
    edge_list edges_input;
    edge_list edges_output;

    node result;

    *output = new_tv_cnf_hierarchy(new_node_list());

    for (ix = 0; ix < input->nodes->sz; ix++) {
        node_input = to_mv_cnf(input->nodes->arr[ix]->constraints);
        edges_input = input->nodes->arr[ix]->edges;

        if (ENCODING_SPARSE == encoding) {
            node_output = sparse_encode_cnf(node_input);
            edges_output = sparse_encode_edges(edges_input,
                                               node_input->variables,
                                               node_input->domains);
        } else if (ENCODING_DENSE == encoding) {
            node_output = dense_encode_cnf(node_input);
            edges_output = dense_encode_edges(edges_input,
                                              node_input->variables,
                                              node_input->domains);
        } else {
            assert(0);
            abort();
        }
        result = new_node(input->nodes->arr[ix]->type,
                          edges_output,
                          to_kb(node_output));
        append_node_list((*output)->nodes, result);
    }

    return 1;
}

static void version(FILE *fp)
{
    fprintf(fp, "mvcnf2tvcnf v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void help(FILE *fp)
{
    fprintf(fp, "Try `mvcnf2tvcnf -h' for more information.\n");
}

static void usage(FILE *fp)
{
    fprintf(fp,
            "Usage: mvcnf2tvcnf [OPTION] SOURCE DEST\n"
            "  or:  mvcnf2tvcnf\n"
            "Propositional Multi-Valued CNF to Two-Valued CNF converter.\n"
            "\n"
            "  -d, --dense          use dense instead of sparse encodings\n"
            "  -h                   display this help and exit\n"
            "  -v                   output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <lydia-dev@falcon.pds.twi.tudelft.nl>.\n");
}

int main(int argc, char **argv)
{
    FILE *infile;
    FILE *outfile;

    char *infilename = NULL;
    char *outfilename = NULL;

    mv_cnf_hierarchy input = mv_cnf_hierarchyNIL;
    tv_cnf_hierarchy output = tv_cnf_hierarchyNIL;

    print_state st;

    int encoding = ENCODING_SPARSE;

    int option_index = 0;
    int c;

    struct option long_options[] = {
        {"dense", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "h?vd", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
                break;
            case 'd':
                encoding = ENCODING_DENSE;
                break;
            case 'h':
                usage(stdout);
                return EXIT_FAILURE;
            case 'v':
                version(stdout);
                return EXIT_SUCCESS;
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

    if (infilename != NULL && 0 != strcmp(infilename, "-")) {
        infile = ckfopen(infilename, "r");
    }
    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();
    if (!fscan_mv_cnf_hierarchy(infile, &input)) {
        fprintf(stderr,
                "Error parsing `%s': %s in line %d.\n",
                infilename,
                errmsg,
                lineno);
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    mvcnf2tvcnf(input, &output, encoding);

    st = set_print(outfile, 1, 75, 8);
    fprint_tv_cnf_hierarchy(st, output);
    end_print(st);

    rfre_tv_cnf_hierarchy(output);
    rfre_mv_cnf_hierarchy(input);

    free(infilename);
    free(outfilename);

    destroy_symtbl();

    return EXIT_SUCCESS;
}
