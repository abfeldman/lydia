#ifndef LC_TYPES_H
#define LC_TYPES_H

#include "list.h"
#include "lcm.h"
#include "variable.h"

enum en_types {
    TYPEint,
    TYPEfloat,
    TYPEbool,
    TYPEenum
};

extern int infer_term_type(csp_term, variable_list, constant_list);
extern int terms_le(csp_term, csp_term, values_set_list, variable_list, constant_list, variable_assignment_list, int *);
extern int terms_ge(csp_term, csp_term, values_set_list, variable_list, constant_list, variable_assignment_list, int *);
extern int terms_lt(csp_term, csp_term, values_set_list, variable_list, constant_list, variable_assignment_list, int *);
extern int terms_gt(csp_term, csp_term, values_set_list, variable_list, constant_list, variable_assignment_list, int *);
extern int terms_equiv(csp_term, csp_term, values_set_list, variable_list, constant_list, variable_assignment_list, int *);

#endif
