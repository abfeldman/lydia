#include "config.h"

#include "normalize.h"
#include "walk.h"
#include "fold.h"
#include "mv.h"

int lcm2mvwff(const_csp_hierarchy input, mv_wff_hierarchy *output)
{
    register unsigned int i;

    *output = new_mv_wff_hierarchy(new_node_list());

    for (i = 0; i < input->nodes->sz; i++) {
        node node_input = input->nodes->arr[i];
        node node_output = normalize_mv_model(node_input);
        fold_bool_constants(node_output);
        node_output = convert_mv_model(node_output);
        if (node_output == nodeNIL) {
            return 0;
        }
        append_node_list((*output)->nodes, node_output);
    }

    return 1;
}
