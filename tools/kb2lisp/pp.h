#ifndef PP_H
#define PP_H

#include "mv.h"
#include "tv.h"

extern void pp_tv_wff(print_state, const_tv_wff);
extern void pp_tv_cnf(print_state, const_tv_cnf);
extern void pp_horn(print_state, const_horn);
extern void pp_tv_dnf(print_state, const_tv_dnf);
extern void pp_tv_wff_hierarchy(print_state, const_tv_wff_hierarchy);
extern void pp_tv_cnf_hierarchy(print_state, const_tv_cnf_hierarchy);
extern void pp_tv_dnf_hierarchy(print_state, const_tv_dnf_hierarchy);
extern void pp_mv_wff(print_state, const_mv_wff);
extern void pp_mv_cnf(print_state, const_mv_cnf);
extern void pp_mv_dnf(print_state, const_mv_dnf);
extern void pp_mv_wff_hierarchy(print_state, const_mv_wff_hierarchy);
extern void pp_mv_cnf_hierarchy(print_state, const_mv_cnf_hierarchy);
extern void pp_mv_dnf_hierarchy(print_state, const_mv_dnf_hierarchy);

#endif
