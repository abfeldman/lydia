#ifndef DUMP_H
#define DUMP_H

#include "ast.h"
#include "lcm.h"

extern csp_hierarchy dump_model(const_model,
                                const_user_type_entry_list,
                                const_attribute_entry_list,
                                const int,
                                int *);

#endif
