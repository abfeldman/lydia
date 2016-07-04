#include <assert.h>
#include <stdlib.h>

#include "tv.h"
#include "atms.h"
#include "trie.h"
#include "qsort.h"
#include "config.h"
#include "pp_variable.h"

static void atms_print_environment(FILE *outfile, array env, const_variable_list variables)
{
    register unsigned int ix;

    fprintf(outfile, "    {");
    for (ix = 0; ix < env->sz; ix++) {
        atms_node node = (atms_node)env->arr[ix];
        if (ix != 0) {
            fprintf(outfile, ", ");
        }
        if (node->index == 0) {
            fprintf(outfile, "F");
        } else {
            pp_variable_name(outfile, variables->arr[node->index - 1]->name);
        }
    }
    fprintf(outfile, "}\n");
}

static int cmp_environments(const void *a, const void *b)
{
    register unsigned int ix;

    const array *na = (const array *)a;
    const array *nb = (const array *)b;

    if ((*na)->sz != (*nb)->sz) {
        return (*na)->sz - (*nb)->sz;
    }
    for (ix = 0; ix < (*na)->sz; ix++) {
        atms_node a = (*na)->arr[ix];
        atms_node b = (*nb)->arr[ix];
        if (a->index != b->index) {
            return a->index - b->index;
        }
    }
    return 0;
}

void atms_print_environments(FILE *outfile,
                             trie L_,
                             const_variable_list variables)
{
    register unsigned int ix;

    array L = trie_get_all_values(L_);

    lydia_qsort(L->arr, L->sz, sizeof(L->arr[0]), cmp_environments);

    for (ix = 0; ix < L->sz; ix++) {
        atms_print_environment(outfile, L->arr[ix], variables);
    }

    array_free(L);
}

void atms_print_node(FILE *outfile,
                     atms_node node,
                     const_variable_list variables)
{
    fprintf(outfile, "{\n");
    fprintf(outfile, "  index: %d\n", node->index);
    if (node->index != 0) {
        fprintf(outfile, "  name: ");
        pp_variable_name(outfile, variables->arr[node->index - 1]->name);
        fprintf(outfile, "\n");
    }
    fprintf(outfile, "  assumption: %d\n", node->assumption);
    fprintf(outfile, "  contradiction: %d\n", node->contradiction);
    fprintf(outfile, "  consequences: %d\n", (int)node->consequences->sz);
    if (NULL != node->label) {
        fprintf(outfile, "  label:");
        fprintf(outfile, "\n  {\n");
        atms_print_environments(outfile, node->label, variables);
        fprintf(outfile, "  }");
        fprintf(outfile, "\n");
    }
    fprintf(outfile, "}\n");
}

void atms_print_justification(FILE *outfile,
                              atms_justification justification,
                              const_variable_list variables)
{
    register unsigned int ix;

    fprintf(outfile, "{\n");
    fprintf(outfile, "  ");
    for (ix = 0; ix < justification->antecedents->sz; ix++) {
        atms_node antecedent = (atms_node)justification->antecedents->arr[ix];
        if (ix != 0) {
            fprintf(outfile, " ^ ");
        }
        if (antecedent->index == 0) {
            fprintf(outfile, "F");
        } else {
            pp_variable_name(outfile, variables->arr[antecedent->index - 1]->name);
        }
    }
    fprintf(outfile, " => ");
    if (justification->consequent->index == 0) {
        fprintf(outfile, "F");
    } else {
        pp_variable_name(outfile, variables->arr[justification->consequent->index - 1]->name);
    }
    fprintf(outfile, "\n");
    fprintf(outfile, "}\n");
}

void atms_print(FILE *outfile, atms tms, const_variable_list variables)
{
    register unsigned int ix;

    fprintf(outfile, "nodes:\n");
    for (ix = 0; ix < tms->nodes->sz; ix++) {
        atms_node node = (atms_node)tms->nodes->arr[ix];
        atms_print_node(outfile, node, variables);
    }

    fprintf(outfile, "justifications:\n");
    for (ix = 0; ix < tms->justifications->sz; ix++) {
        atms_justification justification = (atms_justification)tms->justifications->arr[ix];
        atms_print_justification(outfile, justification, variables);
    }
    fprintf(outfile, "nogoods:\n");
    fprintf(outfile, "{\n");
    atms_print_environments(outfile, ((atms_node)tms->nodes->arr[0])->label, variables);
    fprintf(outfile, "}\n");
}
