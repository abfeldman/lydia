#ifndef PP_VARIABLE_H
#define PP_VARIABLE_H

#include "tv.h"
#include "mv.h"
#include "variable.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern void pp_variable_name(FILE *, const_identifier);
extern void pp_faultmode(FILE *, const_faultmode, const_variable_list, const_values_set_list);

extern void pp_variable(FILE *, const_variable, const_values_set_list);
extern void pp_variable_list(FILE *, const_variable_list, const_values_set_list, const char *);

extern void pp_variable_assignment(FILE *, const_variable_assignment, const_variable_list, const_values_set_list);
extern void pp_variable_assignment_list(FILE *, const_variable_assignment_list, const_variable_list, const_values_set_list);

extern unsigned int pp_tv_term_short(FILE *, const_variable_list, const_tv_term);
extern unsigned int pp_tv_term(FILE *, const_variable_list, const_tv_term);
extern unsigned int pp_mv_term(FILE *, const_variable_list, const_values_set_list, const_mv_term /* sorted */);

extern unsigned int pp_tv_clause_short(FILE *, const_variable_list, const_tv_clause);
extern unsigned int pp_tv_clause(FILE *, const_variable_list, const_tv_clause);
extern unsigned int pp_mv_clause(FILE *, const_variable_list, const_values_set_list, const_mv_clause /* sorted */);

#ifdef __cplusplus
}
#endif

#endif
