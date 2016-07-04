#ifndef SAFARI_H
#define SAFARI_H

#include "tv.h"
#include "ltms.h"
#include "diag.h"
#include "cones.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum en_tags_sat {
    TAGsat_lydia,
    TAGsat_minisat,
    TAGsat_none
} tags_sat;

extern int safari_init(diagnostic_problem,
                       const unsigned int,
                       const unsigned int,
                       const int,
                       const int);
extern int safari_diag(FILE *, diagnostic_problem, const_tv_term);
extern int safari_conflicts(diagnostic_problem, const_tv_term);
extern int safari_pdf(diagnostic_problem, const_tv_term);
extern void safari_destroy();
extern void safari_set_cones(cones_context);

#ifdef __cplusplus
}
#endif

#endif
