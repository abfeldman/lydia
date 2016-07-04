#ifndef GDE_H
#define GDE_H

#include <stdlib.h>

#include "tv.h"
#include "diag.h"

extern int gde_init(diagnostic_problem,const int mincard);
extern int gde_diag(diagnostic_problem, const_tv_term);
extern int gde_conflicts(FILE *, diagnostic_problem, const_tv_term);
extern void gde_destroy();

#endif
