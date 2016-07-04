#ifndef CNF2HORN_H
#define CNF2HORN_H

#include "tv.h"

extern int cnf2horn(const_serializable, serializable *);
extern void cnf2horn_init();
extern void cnf2horn_destroy();

#endif
