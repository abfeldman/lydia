#ifndef DNF_TREE_H
#define DNF_TREE_H

#include "array.h"
#include "tv.h"

typedef struct str_dnf_tree *dnf_tree;
typedef const struct str_dnf_tree *const_dnf_tree;

struct str_dnf_tree
{
    lydia_symbol name;
    tv_term_list terms;
    array kids;
};

extern dnf_tree dnf_tree_new(lydia_symbol, tv_term_list, array);
extern void dnf_tree_free(void *);
extern void *dnf_tree_copy(const void *);

extern dnf_tree dnf_tree_sort(dnf_tree,
                              dnf_tree,
                              tv_variables_cache,
                              mv_variables_cache,
                              int);

extern dnf_tree dnf_tree_make(const_hierarchy,
                              const_node,
                              int_list_list,
                              unsigned int *);
extern dnf_tree dnf_tree_filter(dnf_tree, const_tv_term);

#endif
