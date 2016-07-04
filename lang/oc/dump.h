#ifndef DUMP_H
#define DUMP_H

#include "oc.h"
#include "lcm.h"
#include "variable.h"

extern identifier make_identifier(obs_variable_identifier);
extern csp_hierarchy dump(obs_dump, values_set_list);

#endif
