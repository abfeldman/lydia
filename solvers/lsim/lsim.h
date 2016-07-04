#ifndef LSIM_H
#define LSIM_H

#include "dnf_tree.h"
#include "variable.h"
#include "diag.h"
#include "tv.h"

#include <stdlib.h>

#define UNSAT   0
#define SAT     1
#define UNKNOWN 2

extern int cnf_propagate(const_tv_cnf, const_tv_term, tv_term *);
extern int hdnf_propagate(diagnostic_problem,
                          const_dnf_tree,
                          const_tv_term,
                          const_variable_list,
                          tv_term *);

extern void lsim_init(diagnostic_problem, const_serializable);
extern int lsim(diagnostic_problem, const_tv_term, tv_term *);
extern void lsim_destroy(diagnostic_problem);

#endif
