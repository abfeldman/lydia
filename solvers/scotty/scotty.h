#ifndef SCOTTY_H
#define SCOTTY_H

#include "tv.h"
#include "diag.h"

extern int scotty_init(diagnostic_problem);
extern int scotty_diag(diagnostic_problem, const_tv_term);
extern int scotty_destroy();

#endif
