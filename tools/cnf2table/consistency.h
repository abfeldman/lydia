#ifndef SAFARI_CONSISTENCY_H
#define SAFARI_CONSISTENCY_H

#include "array.h"
#include "ltms.h"
#include "tv.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int safari_is_sat(const_tv_cnf, const_tv_clause_list, const_tv_term, const int);
extern int safari_enable_observation_assumptions(ltms, const_tv_term);
extern void safari_retract_observation_assumptions(ltms, const_tv_term);
extern int safari_enable_health_assumptions(ltms, const_tv_variables_cache, array);
extern void safari_retract_health_assumptions(ltms, const_tv_variables_cache, array);

#ifdef __cplusplus
}
#endif

#endif
