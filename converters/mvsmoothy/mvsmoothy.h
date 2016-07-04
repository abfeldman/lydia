#ifndef __MVSMOOTHY_H__
#define __MVSMOOTHY_H__

#include "serializable.h"
#include "hierarchy.h"

extern int smoothy(const_serializable input, int hier, unsigned int depth, const int verbose, const int simplify, serializable *output);
extern void mark_used(hierarchy input, node root, int_list result);
extern void smoothy_init();
extern void smoothy_destroy();

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

