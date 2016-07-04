#ifndef OBS_H
#define OBS_H

#include "hierarchy.h"
#include "variable.h"
#include "hash.h"

typedef struct str_observations *observations;

struct str_observations
{
    hash_table *observations;

    array names;
};

extern observations observations_new();
extern signed char observations_load(hierarchy,
                                     const_variable_list,
                                     observations *);
extern void *observation_get(observations, const char *);
extern void observations_free(observations);

#endif
