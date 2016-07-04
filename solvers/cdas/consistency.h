#ifndef CONSISTENCY_H
#define CONSISTENCY_H

#include "array.h"
#include "ltms.h"
#include "tv.h"

extern int cdas_is_sat(const_tv_cnf, const_tv_clause_list, const_tv_term);
extern int cdas_enable_observation(ltms, const_tv_term);
extern void cdas_retract_observation(ltms, const_tv_term);
extern int cdas_assume_health(ltms, const_tv_variables_cache, array);
extern void cdas_retract_health(ltms, const_tv_variables_cache, array);

#endif
