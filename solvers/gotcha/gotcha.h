#ifndef GOTCHA_H
#define GOTCHA_H

#include "variable.h"
#include "dnf_tree.h"
#include "array.h"
#include "cones.h"
#include "diag.h"
#include "tv.h"

typedef struct str_gotcha_node *gotcha_node;
typedef const struct str_gotcha_node *const_gotcha_node;

struct str_gotcha_node
{
    unsigned short int *offsets;
    unsigned short int depth;
    unsigned short int cardinality;
};

extern void gotcha_init(diagnostic_problem,
                        const_tv_dnf_hierarchy,
                        const_node);
extern int gotcha_diag(diagnostic_problem, const_tv_term);
extern void gotcha_destroy(diagnostic_problem);

extern int gotcha_set_input(diagnostic_problem,
                            const_tv_dnf_hierarchy,
                            const_node);
extern void gotcha_set_cones(cones_context);
extern void gotcha_destroy_input(diagnostic_problem);

#endif
