#ifndef DEC_H
#define DEC_H

#include "variable.h"
#include "mv.h"
#include "tv.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern mv_term decode_term(const_tv_term,
                           const_variable_list,
                           const_variable_list,
                           const_values_set_list,
                           int);

#ifdef __cplusplus
}
#endif

#endif
