#ifndef __WALK_H__
#define __WALK_H__

#include "tv.h"
#include "lcm.h"

extern tv_wff_expr walk_tv_sentence(csp_sentence sentence, variable_list variables, constant_list constants);

#endif

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
