#ifndef PI_H
#define PI_H

#include "tv.h"

extern tv_nf pi_tison(const_tv_nf);
extern tv_nf pi_tison_trie(const_tv_nf);
extern tv_nf pi_brute_force(const_tv_nf);
extern int is_subsumed(const_tv_literal_set, const_tv_literal_set);

#endif
