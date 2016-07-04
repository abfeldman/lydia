#ifndef ENC_H
#define ENC_H

#include "variable.h"
#include "tv.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern int mvwff2tvwff(const_mv_wff_hierarchy, tv_wff_hierarchy *, const int);

extern tv_term encode_term(const_mv_term,
                           const_variable_list,
                           const_variable_list,
                           const_values_set_list,
                           const int);

#ifdef __cplusplus
}
#endif

#endif
