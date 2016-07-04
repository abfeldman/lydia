#include "cnf2table.h"
#include "defs.h"
#include "pp_variable.h"
#include "bcp.h"

#include <assert.h>

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

static int is_pi(const_variable var)
{
    const_variable_attribute attr = find_attribute(var->attributes, add_lydia_symbol("pi"));
    if (attr == variable_attributeNIL) {
        return 0;
    }

    assert(attr->tag == TAGbool_variable_attribute);

    return 1;
}

static int is_po(const_variable var)
{
    const_variable_attribute attr = find_attribute(var->attributes, add_lydia_symbol("po"));
    if (attr == variable_attributeNIL) {
        return 0;
    }

    assert(attr->tag == TAGbool_variable_attribute);

    return 1;
}

void cnf2table(tv_cnf_flat_kb model, FILE *outfile)
{
    register unsigned int ix;
    register unsigned int iy;

    tv_cnf cnf = to_tv_cnf(model->constraints);

    array pi = array_new(NULL, NULL);
    array po = array_new(NULL, NULL);

    signed char first = 1;

    unsigned int inputs;

    for (ix = 0; ix < cnf->variables->sz; ix++) {
        variable var = cnf->variables->arr[ix];
        if (is_pi(var)) {
            array_append(pi, ui2p(ix));
        }
        if (is_po(var)) {
            array_append(po, ui2p(ix));
        }
    }

    for (ix = 0; ix < pi->sz; ix++) {
        variable var = cnf->variables->arr[p2ui(pi->arr[ix])];

        if (first) {
            first = 0;
        } else {
            fprintf(outfile, "\t");
        }

        pp_variable_name(outfile, var->name);
    }
    for (ix = 0; ix < po->sz; ix++) {
        variable var = cnf->variables->arr[p2ui(po->arr[ix])];

        if (first) {
            first = 0;
        } else {
            fprintf(outfile, "\t");
        }

        pp_variable_name(outfile, var->name);
    }
    fprintf(outfile, "\n");

    /* F T */
    first = 1;
    for (ix = 0; ix < pi->sz; ix++) {
        if (first) {
            first = 0;
        } else {
            fprintf(outfile, "\t");
        }

        fprintf(outfile, "F T");
    }
    for (ix = 0; ix < po->sz; ix++) {
        if (first) {
            first = 0;
        } else {
            fprintf(outfile, "\t");
        }

        fprintf(outfile, "F T");
    }
    fprintf(outfile, "\n");

    /* class */
    first = 1;
    for (ix = 0; ix < pi->sz; ix++) {
        if (first) {
            first = 0;
        } else {
            fprintf(outfile, "\t");
        }
    }
    for (ix = 0; ix < po->sz; ix++) {
        if (first) {
            first = 0;
        } else {
            fprintf(outfile, "\t");
        }
        fprintf(outfile, "class");
    }
    fprintf(outfile, "\n");

    fprintf(outfile, "\n");

    inputs = 1 << pi->sz;

    for (ix = 0; ix < inputs; ix++) {
        tv_term input = new_tv_term(new_int_list(), new_int_list());
        tv_term output;
        signed char val;
        int result;

        first = 1;
        for (iy = 0; iy < pi->sz; iy++) {
            val = (ix >> iy) & 1;

            if (first) {
                first = 0;
            } else {
                fprintf(outfile, "\t");
            }
            fprintf(outfile, "%c", val ? 'T' : 'F');

            append_int_list(val ? input->pos : input->neg, p2ui(pi->arr[iy]));
        }

        result = cnf_propagate(cnf, input, &output);

        assert(result == SAT);
        assert(output->neg->sz + output->pos->sz == cnf->variables->sz);

        for (iy = 0; iy < po->sz; iy++) {
            if (member_int_list(output->neg, p2ui(po->arr[iy]))) {
                val = 0;
            } else if (member_int_list(output->pos, p2ui(po->arr[iy]))) {
                val = 1;
            } else {
                assert(0);
            }

            if (first) {
                first = 0;
            } else {
                fprintf(outfile, "\t");
            }
            fprintf(outfile, "%c", val ? 'T' : 'F');
        }

        fprintf(outfile, "\n");
        rfre_tv_term(input);
    }

    array_free(pi);
    array_free(po);
}
