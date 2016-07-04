#ifndef CONES
#define CONES

#include "diag.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAP_CONES_SUCCESS           0
#define MAP_CONES_OUT_OF_MEMORY     1
#define MAP_CONES_UNKNOWN_CONE      2
#define MAP_CONES_UNKNOWN_VARIABLE  3

typedef struct str_cones_context *cones_context;
typedef const struct str_cones_context *const_cones_context;

struct str_cones_context
{
    diagnostic_problem ds;
    diagnostic_problem full_ds;
    array map;
};

extern void cones_context_free(cones_context);

extern cones_context map_cones(FILE *,
                               const diagnostic_problem,
                               const diagnostic_problem,
                               signed char *);

extern signed char expand_cone_exhaustive(const cones_context,
                                          const const_tv_term,
                                          const_faultmode);

extern signed char expand_cone_random(const cones_context,
                                      const const_tv_term,
                                      const_faultmode);

#ifdef __cplusplus
}
#endif

#endif
