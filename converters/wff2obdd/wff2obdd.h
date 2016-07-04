#ifndef WFF2OBDD_H
#define WFF2OBDD_H

#include "tv.h"

extern int wff2obdd(const_serializable, serializable *, const int);
extern void wff2obdd_init();
extern void wff2obdd_destroy();

#endif
