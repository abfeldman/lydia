#include <assert.h>
#include <string.h>
#include <math.h>

#include "tv.h"
#include "mv.h"
#include "pp.h"

static void pp_tv_wff_expr(print_state st,
                           const_tv_wff_expr e,
                           const_variable_list variables)
{
    char *name;

    switch (e->tag) {
        case TAGtv_wff_e_not:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":NOT"));
            pp_tv_wff_expr(st, to_tv_wff_e_not(e)->n, variables);
            close_list(st);
            break;
        case TAGtv_wff_e_and:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":AND"));
            pp_tv_wff_expr(st, to_tv_wff_e_and(e)->lhs, variables);
            pp_tv_wff_expr(st, to_tv_wff_e_and(e)->rhs, variables);
            close_list(st);
            break;
        case TAGtv_wff_e_or:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":OR"));
            pp_tv_wff_expr(st, to_tv_wff_e_or(e)->lhs, variables);
            pp_tv_wff_expr(st, to_tv_wff_e_or(e)->rhs, variables);
            close_list(st);
            break;
        case TAGtv_wff_e_impl:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":IMPLY"));
            pp_tv_wff_expr(st, to_tv_wff_e_impl(e)->lhs, variables);
            pp_tv_wff_expr(st, to_tv_wff_e_impl(e)->rhs, variables);
            close_list(st);
            break;
        case TAGtv_wff_e_equiv:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":EQUIV"));
            pp_tv_wff_expr(st, to_tv_wff_e_equiv(e)->lhs, variables);
            pp_tv_wff_expr(st, to_tv_wff_e_equiv(e)->rhs, variables);
            close_list(st);
            break;
        case TAGtv_wff_e_var:
            if (NULL == (name = get_variable_name(variables->arr[to_tv_wff_e_var(e)->v]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);

            break;
        case TAGtv_wff_e_const:
            fprint_lydia_symbol(st, add_lydia_symbol(to_tv_wff_e_const(e)->c ? "t" : "nil"));
            break;
    }
}

static void pp_mv_wff_expr(print_state st,
                           const_mv_wff_expr e,
                           const_variable_list variables,
                           const_values_set_list domains)
{
    char *name;

    register unsigned int ix, iy;

    switch (e->tag) {
        case TAGmv_wff_e_not:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":NOT"));
            pp_mv_wff_expr(st, to_mv_wff_e_not(e)->n, variables, domains);
            close_list(st);
            break;
        case TAGmv_wff_e_and:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":AND"));
            pp_mv_wff_expr(st, to_mv_wff_e_and(e)->lhs, variables, domains);
            pp_mv_wff_expr(st, to_mv_wff_e_and(e)->rhs, variables, domains);
            close_list(st);
            break;
        case TAGmv_wff_e_or:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":OR"));
            pp_mv_wff_expr(st, to_mv_wff_e_or(e)->lhs, variables, domains);
            pp_mv_wff_expr(st, to_mv_wff_e_or(e)->rhs, variables, domains);
            close_list(st);
            break;
        case TAGmv_wff_e_impl:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":IMPLY"));
            pp_mv_wff_expr(st, to_mv_wff_e_impl(e)->lhs, variables, domains);
            pp_mv_wff_expr(st, to_mv_wff_e_impl(e)->rhs, variables, domains);
            close_list(st);
            break;
        case TAGmv_wff_e_equiv:
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol(":EQUIV"));
            pp_mv_wff_expr(st, to_mv_wff_e_equiv(e)->lhs, variables, domains);
            pp_mv_wff_expr(st, to_mv_wff_e_equiv(e)->rhs, variables, domains);
            close_list(st);
            break;
        case TAGmv_wff_e_var:
            open_list(st);
            if (NULL == (name = get_variable_name(variables->arr[to_mv_wff_e_var(e)->var]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
            ix = to_enum_variable(variables->arr[to_mv_wff_e_var(e)->var])->values_set;
            iy = to_mv_wff_e_var(e)->val;
            fprint_lydia_symbol(st, domains->arr[ix]->entries->arr[iy]);
            close_list(st);
            break;
        case TAGmv_wff_e_const:
            fprint_lydia_symbol(st, add_lydia_symbol(to_tv_wff_e_const(e)->c ? "t" : "nil"));
            break;
    }
}

static void pp_tv_clause(print_state st,
                         const_tv_clause clause,
                         const_variable_list variables)
{
    char *name, *nname;

    unsigned int ix;

    for (ix = 0; ix < variables->sz; ix++) {
        if (member_int_list(clause->pos, ix)) {
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
        }
        if (member_int_list(clause->neg, ix)) {
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            if (NULL == (nname = realloc(name, strlen(name) + 2))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            memmove(nname + 1, nname, strlen(nname) + 1);
            nname[0] = '~';
            fprint_lydia_symbol(st, add_lydia_symbol(nname));
            free(nname);
        }
    }
}

static void pp_material_implication(print_state st,
                                    const_material_implication clause,
                                    const_variable_list variables,
                                    unsigned int num)
{
    char *name;

    unsigned int ix;

    fprint_lydia_symbol(st, add_lydia_symbol("justify-node"));

    {
        char buf[128];
        sprintf(buf, "\"R%d\"", num);
        fprint_lydia_symbol(st, add_lydia_symbol(buf));
    }

    if (clause->consequent == -1) {
        fprint_lydia_symbol(st, add_lydia_symbol("contradiction"));
    } else {
        if (NULL == (name = get_variable_name(variables->arr[clause->consequent]->name))) {
            assert(0); /* To Do: Normal error handling. */
            abort();
        }
        fprint_lydia_symbol(st, add_lydia_symbol(name));
        free(name);
    }

    open_list(st);
    fprint_lydia_symbol(st, add_lydia_symbol("list"));
    for (ix = 0; ix < variables->sz; ix++) {
        if (member_int_list(clause->antecedents, ix)) {
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
        }
    }
    close_list(st);
}

static void pp_tv_term(print_state st,
                       const_tv_term term,
                       const_variable_list variables)
{
    char *name, *nname;

    unsigned int ix;

    for (ix = 0; ix < variables->sz; ix++) {
        if (member_int_list(term->pos, ix)) {
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
        }
        if (member_int_list(term->neg, ix)) {
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            if (NULL == (nname = realloc(name, strlen(name) + 2))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            memmove(nname + 1, nname, strlen(nname) + 1);
            nname[0] = '~';
            fprint_lydia_symbol(st, add_lydia_symbol(nname));
            free(nname);
        }
    }
}

static void pp_mv_clause(print_state st,
                         const_mv_clause clause,
                         const_variable_list variables,
                         const_values_set_list domains)
{
    char *name;

    register unsigned int ix;
    unsigned int pos;

    for (ix = 0; ix < variables->sz; ix++) {
        if (search_mv_literal_list(clause->pos, ix, &pos)) {
            open_list(st);
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
            fprint_lydia_symbol(st, domains->arr[to_enum_variable(variables->arr[ix])->values_set]->entries->arr[clause->pos->arr[pos]->val]);
            close_list(st);
        }
        if (search_mv_literal_list(clause->neg, ix, &pos)) {
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol("~"));
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
            fprint_lydia_symbol(st, domains->arr[to_enum_variable(variables->arr[ix])->values_set]->entries->arr[clause->neg->arr[pos]->val]);
            close_list(st);
        }
    }
}

static void pp_mv_term(print_state st,
                       const_mv_term term,
                       const_variable_list variables,
                       const_values_set_list domains)
{
    char *name;

    register unsigned int ix;
    unsigned int pos;

    for (ix = 0; ix < variables->sz; ix++) {
        if (search_mv_literal_list(term->pos, ix, &pos)) {
            open_list(st);
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
            fprint_lydia_symbol(st, domains->arr[to_enum_variable(variables->arr[ix])->values_set]->entries->arr[term->pos->arr[pos]->val]);
            close_list(st);
        }
        if (search_mv_literal_list(term->neg, ix, &pos)) {
            open_list(st);
            fprint_lydia_symbol(st, add_lydia_symbol("~"));
            if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
            fprint_lydia_symbol(st, domains->arr[to_enum_variable(variables->arr[ix])->values_set]->entries->arr[term->neg->arr[pos]->val]);
            close_list(st);
        }
    }
}

static void pp_tv_clause_list(print_state st,
                              const_tv_clause_list clauses,
                              const_variable_list variables)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < clauses->sz; ix++) {
        open_list(st);
        pp_tv_clause(st, clauses->arr[ix], variables);
        close_list(st);
    }
    close_list(st);
}

static void pp_material_implication_list(print_state st,
                                         const_material_implication_list clauses,
                                         const_variable_list variables)
{
    register unsigned int ix;

    char *name;

    open_list(st);
    for (ix = 0; ix < variables->sz; ix++) {
        open_list(st);
        if (NULL == (name = get_variable_name(variables->arr[ix]->name))) {
            assert(0); /* To Do: Normal error handling. */
            abort();
        }
        fprint_lydia_symbol(st, add_lydia_symbol("setq"));
        fprint_lydia_symbol(st, add_lydia_symbol(name));
        open_list(st);
        fprint_lydia_symbol(st, add_lydia_symbol("tms-create-node"));
        fprint_lydia_symbol(st, add_lydia_symbol("*atms*"));
        {
            char buf[256];
            sprintf(buf, "\"%s\"", name);
            fprint_lydia_symbol(st, add_lydia_symbol(buf));
        }
        if (is_health(variables->arr[ix])) {
            fprint_lydia_symbol(st, add_lydia_symbol(":ASSUMPTIONP"));
            fprint_lydia_symbol(st, add_lydia_symbol("t"));
        }
        close_list(st);
        close_list(st);
        free(name);
    }
    open_list(st);
    fprint_lydia_symbol(st, add_lydia_symbol("setq"));
    fprint_lydia_symbol(st, add_lydia_symbol("contradiction"));
    open_list(st);
    fprint_lydia_symbol(st, add_lydia_symbol("tms-create-node"));
    fprint_lydia_symbol(st, add_lydia_symbol("*atms*"));
    fprint_lydia_symbol(st, add_lydia_symbol("'CONTRADICTION"));
    fprint_lydia_symbol(st, add_lydia_symbol(":CONTRADICTORYP"));
    fprint_lydia_symbol(st, add_lydia_symbol("t"));
    close_list(st);
    close_list(st);

    for (ix = 0; ix < clauses->sz; ix++) {
        open_list(st);
        pp_material_implication(st, clauses->arr[ix], variables, ix + 1);
        close_list(st);
    }
    close_list(st);
}

static void pp_tv_wff_expr_list(print_state st,
                                const_tv_wff_expr_list l,
                                const_variable_list variables)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < l->sz; ix++) {
        pp_tv_wff_expr(st, l->arr[ix], variables);
    }
    close_list(st);
}

static void pp_mv_wff_expr_list(print_state st,
                                const_mv_wff_expr_list l,
                                const_variable_list variables,
                                const_values_set_list domains)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < l->sz; ix++) {
        pp_mv_wff_expr(st, l->arr[ix], variables, domains);
    }
    close_list(st);
}

static void pp_tv_term_list(print_state st,
                            const_tv_term_list terms,
                            const_variable_list variables)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < terms->sz; ix++) {
        open_list(st);
        pp_tv_term(st, terms->arr[ix], variables);
        close_list(st);
    }
    close_list(st);
}

static void pp_mv_clause_list(print_state st,
                              const_mv_clause_list clauses,
                              const_variable_list variables,
                              const_values_set_list domains)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < clauses->sz; ix++) {
        open_list(st);
        pp_mv_clause(st, clauses->arr[ix], variables, domains);
        close_list(st);
    }
    close_list(st);
}

static void pp_mv_term_list(print_state st,
                            const_mv_term_list terms,
                            const_variable_list variables,
                            const_values_set_list domains)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < terms->sz; ix++) {
        open_list(st);
        pp_mv_term(st, terms->arr[ix], variables, domains);
        close_list(st);
    }
    close_list(st);
}

void pp_tv_wff(print_state st, const_tv_wff wff)
{
    pp_tv_wff_expr_list(st, wff->e, wff->variables);
}

void pp_mv_wff(print_state st, const_mv_wff wff)
{
    pp_mv_wff_expr_list(st, wff->e, wff->variables, wff->domains);
}

void pp_tv_cnf(print_state st, const_tv_cnf cnf)
{
    pp_tv_clause_list(st, cnf->clauses, cnf->variables);
}

void pp_horn(print_state st, const_horn theory)
{
    pp_material_implication_list(st, theory->clauses, theory->variables);
}

void pp_tv_dnf(print_state st, const_tv_dnf dnf)
{
    pp_tv_term_list(st, dnf->terms, dnf->variables);
}

void pp_mv_cnf(print_state st, const_mv_cnf cnf)
{
    pp_mv_clause_list(st, cnf->clauses, cnf->variables, cnf->domains);
}

void pp_mv_dnf(print_state st, const_mv_dnf dnf)
{
    pp_mv_term_list(st, dnf->terms, dnf->variables, dnf->domains);
}

char *get_edge_name(const_lydia_symbol name, const_int_list indices)
{
    size_t length = 0;
    unsigned int ix;
    char *result;

    length += strlen(name->name);
    if (int_listNIL != indices) {
        for (ix = 0; ix < indices->sz; ix++) {
            length += (size_t)log10(indices->arr[ix] + 10) + 2;
        }
    }
    result = malloc(length + 2);
    result[0] = '\0';
    sprintf(result + strlen(result), "%s", name->name);
    if (int_listNIL != indices) {
        for (ix = 0; ix < indices->sz; ix++) {
            sprintf(result + strlen(result), "[%d]", indices->arr[ix]);
        }
    }

    return result;
}

static void pp_edge_list(print_state st, const_edge_list edges)
{
    register unsigned int iy, iz;

    char *name;

    for (iy = 0; iy < edges->sz; iy++) {
        open_list(st);
        fprint_lydia_symbol(st, edges->arr[iy]->type);
        if (NULL == (name = get_edge_name(edges->arr[iy]->name,
                                          edges->arr[iy]->indices))) {
            assert(0); /* To Do: Normal error handling. */
            abort();
        }
        fprint_lydia_symbol(st, add_lydia_symbol(name));
        free(name);

        for (iz = 0; iz < edges->arr[iy]->bindings->sz; iz++) {
            open_list(st);
            if (NULL == (name = get_variable_name(edges->arr[iy]->bindings->arr[iz]->from))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
            if (NULL == (name = get_variable_name(edges->arr[iy]->bindings->arr[iz]->to))) {
                assert(0); /* To Do: Normal error handling. */
                abort();
            }
            fprint_lydia_symbol(st, add_lydia_symbol(name));
            free(name);
            close_list(st);
        }
        close_list(st);
    }
}

void pp_mv_wff_hierarchy(print_state st, const_mv_wff_hierarchy wff_hierarchy)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < wff_hierarchy->nodes->sz; ix++) {
        open_list(st);

        fprint_lydia_symbol(st, wff_hierarchy->nodes->arr[ix]->type);

        pp_mv_wff(st, to_mv_wff(wff_hierarchy->nodes->arr[ix]->constraints));
        pp_edge_list(st, wff_hierarchy->nodes->arr[ix]->edges);
        close_list(st);
    }
    close_list(st);
}

void pp_mv_cnf_hierarchy(print_state st, const_mv_cnf_hierarchy cnf_hierarchy)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < cnf_hierarchy->nodes->sz; ix++) {
        open_list(st);

        fprint_lydia_symbol(st, cnf_hierarchy->nodes->arr[ix]->type);

        pp_mv_cnf(st, to_mv_cnf(cnf_hierarchy->nodes->arr[ix]->constraints));
        pp_edge_list(st, cnf_hierarchy->nodes->arr[ix]->edges);
        close_list(st);
    }
    close_list(st);
}

void pp_mv_dnf_hierarchy(print_state st, const_mv_dnf_hierarchy dnf_hierarchy)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < dnf_hierarchy->nodes->sz; ix++) {
        open_list(st);

        fprint_lydia_symbol(st, dnf_hierarchy->nodes->arr[ix]->type);

        pp_mv_dnf(st, to_mv_dnf(dnf_hierarchy->nodes->arr[ix]->constraints));
        pp_edge_list(st, dnf_hierarchy->nodes->arr[ix]->edges);
        close_list(st);
    }
    close_list(st);
}

void pp_tv_cnf_hierarchy(print_state st, const_tv_cnf_hierarchy cnf_hierarchy)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < cnf_hierarchy->nodes->sz; ix++) {
        open_list(st);

        fprint_lydia_symbol(st, cnf_hierarchy->nodes->arr[ix]->type);

        pp_tv_cnf(st, to_tv_cnf(cnf_hierarchy->nodes->arr[ix]->constraints));
        pp_edge_list(st, cnf_hierarchy->nodes->arr[ix]->edges);
        close_list(st);
    }
    close_list(st);
}

void pp_tv_dnf_hierarchy(print_state st, const_tv_dnf_hierarchy dnf_hierarchy)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < dnf_hierarchy->nodes->sz; ix++) {
        open_list(st);

        fprint_lydia_symbol(st, dnf_hierarchy->nodes->arr[ix]->type);

        pp_tv_dnf(st, to_tv_dnf(dnf_hierarchy->nodes->arr[ix]->constraints));
        pp_edge_list(st, dnf_hierarchy->nodes->arr[ix]->edges);
        close_list(st);
    }
    close_list(st);
}

void pp_tv_wff_hierarchy(print_state st, const_tv_wff_hierarchy wff_hierarchy)
{
    register unsigned int ix;

    open_list(st);
    for (ix = 0; ix < wff_hierarchy->nodes->sz; ix++) {
        open_list(st);

        fprint_lydia_symbol(st, wff_hierarchy->nodes->arr[ix]->type);

        pp_tv_wff(st, to_tv_wff(wff_hierarchy->nodes->arr[ix]->constraints));
        pp_edge_list(st, wff_hierarchy->nodes->arr[ix]->edges);
        close_list(st);
    }
    close_list(st);
}
