#ifndef __CNF2DNF_H__
#define __CNF2DNF_H__

#include "tv.h"

extern int cnf2dnf(const_serializable input, serializable *output);
extern void cnf2dnf_init();
extern void cnf2dnf_destroy();

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
