#ifndef CARD_SORT_TERMS
#define CARD_SORT_TERMS

#include "tv.h"
#include "mv.h"

extern tv_term_list cardinality_sort_terms(tv_variables_cache,
                                           mv_variables_cache,
                                           tv_term_list,
                                           int);

#endif
