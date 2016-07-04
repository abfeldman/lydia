#ifndef CDAS_H
#define CDAS_H

#include "variable.h"
#include "diag.h"
#include "ltms.h"
#include "tv.h"

extern int cdas_init(diagnostic_problem, const int, const int, const int, const int);
extern int cdas_diag(diagnostic_problem, const_tv_term);
extern int cdas_obs(diagnostic_problem, const_tv_term);
extern void cdas_destroy();

extern ltms cdas_get_tms();

#endif
