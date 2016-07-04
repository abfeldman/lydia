#include "sorted_int_list.h"
#include "pp_variable.h"
#include "lreadline.h"
#include "variable.h"
#include "dnf_tree.h"
#include "config.h"
#include "inline.h"
#include "gotcha.h"
#include "util.h"
#include "lsim.h"
#include "defs.h"
#include "bcp.h"
#include "enc.h"
#include "dec.h"
#include "io.h"
#include "tv.h"

#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

void assign_health_variables(variable_list variables, tv_term assignments)
{
    register unsigned int ix;

    for (ix = 0; ix < variables->sz; ix++) {
        if (!is_health(variables->arr[ix])) {
            continue;
        }
        switch (variables->arr[ix]->tag) {
            case TAGbool_variable:
                if (get_probability_bool(variables->arr[ix], 0) > 0.5) {
                    append_int_list(assignments->neg, ix);
                } else {
                    append_int_list(assignments->pos, ix);
                }
                break;
            case TAGenum_variable:
                assert(0);
                abort();
                break;
            case TAGint_variable:
            case TAGfloat_variable:
                assert(0);
                abort();
                break;
            default:
                assert(0);
        }
    }
}

int cnf_propagate(const_tv_cnf input,
                  const_tv_term assignments,
                  tv_term *solution)
{
    tv_clause_list initial_unit_clauses;
    tv_clause_list kb;
    int result;

    initial_unit_clauses = term_to_tv_clause_list(assignments);
    kb = concat_tv_clause_list(rdup_tv_clause_list(input->clauses),
                               rdup_tv_clause_list(initial_unit_clauses));

    result = bcp_get_solution(kb, input->variables->sz, solution);

    rfre_tv_clause_list(kb);
    rfre_tv_clause_list(initial_unit_clauses);

    return result;
}

static int is_consistent(tv_term term, signed char *inferred)
{
    register unsigned int ix;

    for (ix = 0; ix < term->pos->sz; ix++) {
        if (-1 == inferred[term->pos->arr[ix]]) {
            return 0;
        }
    }
    for (ix = 0; ix < term->neg->sz; ix++) {
        if (1 == inferred[term->neg->arr[ix]]) {
            return 0;
        }
    }
    return 1;
}

static int filter_terms(tv_term_list terms, signed char *inferred, int *fg)
{
    register unsigned int ix;

    for (ix = terms->sz - 1; ix < terms->sz; ix--) {
        if (!is_consistent(terms->arr[ix], inferred)) {
            delete_tv_term_list(terms, ix);
            *fg = 1;
        }
    }
    if (terms->sz == 1) {
        for (ix = 0; ix < terms->arr[0]->pos->sz; ix++) {
            inferred[terms->arr[0]->pos->arr[ix]] = 1;
        }
        for (ix = 0; ix < terms->arr[0]->neg->sz; ix++) {
            inferred[terms->arr[0]->neg->arr[ix]] = -1;
        }
    }
    if (terms->sz == 0) {
        return 0;
    }
    return 1;
}

static int hdnf_propagate_once(const_variable_list variables,
                               dnf_tree tree,
                               signed char *inferred,
                               int *fg)
{
    register unsigned int ix;

    if (!filter_terms(tree->terms, inferred, fg)) {
        return UNSAT;
    }

    for (ix = 0; ix < tree->kids->sz; ix++) {
        if (UNSAT == hdnf_propagate_once(variables,
                                         tree->kids->arr[ix],
                                         inferred,
                                         fg)) {
            return UNSAT;
        }
    }

    return UNKNOWN;
}

static int hdnf_is_sat(dnf_tree tree)
{
    register unsigned int ix;

    if (tree->terms->sz != 1) {
        return UNKNOWN;
    }

    for (ix = 0; ix < tree->kids->sz; ix++) {
        if (UNKNOWN == hdnf_is_sat(tree->kids->arr[ix])) {
            return UNKNOWN;
        }
    }

    return SAT;
}

int hdnf_propagate(diagnostic_problem problem,
                   const_dnf_tree sd,
                   const_tv_term assignment,
                   const_variable_list variables,
                   tv_term *solution)
{
    unsigned int ix;
    int fg;

    int result;
    dnf_tree consistent_input = dnf_tree_filter(dnf_tree_copy(sd), assignment);

    signed char *inferred = (signed char *)malloc(variables->sz * sizeof(signed char));
    if (NULL == inferred) {
        assert(0);
        abort();
    }
    memset(inferred, 0, problem->variables->sz * sizeof(char));

    for (ix = 0; ix < assignment->neg->sz; ix++) {
        inferred[assignment->neg->arr[ix]] = -1;
    }
    for (ix = 0; ix < assignment->pos->sz; ix++) {
        inferred[assignment->pos->arr[ix]] = 1;
    }

    do {
        fg = 0;
        result = hdnf_propagate_once(variables,
                                     consistent_input,
                                     inferred,
                                     &fg);
    } while (fg);

    if (result != UNSAT) {
        result = hdnf_is_sat(consistent_input);
    }
    dnf_tree_free(consistent_input);
    if (result != SAT) {
        free(inferred);
        return result;
    }
    *solution = new_tv_term(new_int_list(), new_int_list());
    for (ix = 0; ix < variables->sz; ix++) {
        if (inferred[ix] == -1) {
            append_int_list((*solution)->neg, ix);
        }
        if (inferred[ix] == 1) {
            append_int_list((*solution)->pos, ix);
        }
    }

    free(inferred);

    return result;
}

void lsim_init(diagnostic_problem problem,
               const_serializable input)
{
    srand((unsigned int)time(NULL) * getpid());

    if (TAGtv_dnf_hierarchy == input->tag) {
        node root = NULL;

        if (NULL == (root = find_root_node(to_hierarchy(input)))) {
            assert(0);
            abort();
        }

        gotcha_set_input(problem, (tv_dnf_hierarchy)input, root);
    }
}

void lsim_destroy(diagnostic_problem problem)
{
    if (TAGtv_dnf_hierarchy == problem->model->tag) {
        gotcha_destroy_input(problem);
    }
}

int lsim(diagnostic_problem dp, const_tv_term alpha, tv_term *solution)
{
    int result;
    tv_term instantiation = rdup_tv_term(alpha);

    assign_health_variables(dp->encoding == ENCODING_NONE ? dp->variables : dp->encoded_variables, instantiation);

    if (TAGtv_cnf_flat_kb == dp->model->tag) {
        const_tv_cnf cnf = to_tv_cnf(to_tv_cnf_flat_kb(dp->model)->constraints);

        result = cnf_propagate(cnf, instantiation, solution);
    } else if (TAGtv_dnf_hierarchy == dp->model->tag) {
        result = hdnf_propagate(dp,
                                dp->u.tv_tdnf_sd.model,
                                instantiation,
                                dp->variables,
                                solution);
    } else {
        assert(0);
        abort();
    }

    rfre_tv_term(instantiation);

    return result;
}

