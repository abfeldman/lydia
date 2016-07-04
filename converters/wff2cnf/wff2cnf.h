#ifndef WFF2CNF_H
#define WFF2CNF_H

#include "tv.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern tv_cnf tv_wff_to_tv_cnf(tv_wff);

extern int wff2cnf(const_serializable, serializable *);

#ifdef __cplusplus
}
#endif

#endif
