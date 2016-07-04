#include "card_sort_terms.h"
#include "sorted_int_list.h"
#include "hierarchy.h"
#include "variable.h"
#include "dnf_tree.h"
#include "config.h"
#include "gotcha.h"
#include "qsort.h"
#include "util.h"
#include "mv.h"
#include "tv.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

dnf_tree dnf_tree_new(lydia_symbol name, tv_term_list terms, array kids)
{
    dnf_tree result;

    if (NULL == (result = (dnf_tree)malloc(sizeof(struct str_dnf_tree)))) {
        return result;
    }
    result->name = name;
    result->terms = terms;
    result->kids = kids;

    return result;
}

void dnf_tree_free(void *tree)
{
    if (tree != NULL) {
        rfre_tv_term_list(((const_dnf_tree)tree)->terms);
        array_free(((const_dnf_tree)tree)->kids);
        free(tree);
    }
}

void *dnf_tree_copy(const void *tree)
{
    if (tree == NULL) {
        return NULL;
    }
    return dnf_tree_new(((const_dnf_tree)tree)->name,
                        rdup_tv_term_list(((const_dnf_tree)tree)->terms),
                        array_copy(((const_dnf_tree)tree)->kids));
}

dnf_tree dnf_tree_sort(dnf_tree tree,
                       dnf_tree root,
                       tv_variables_cache tv_cache,
                       mv_variables_cache mv_cache,
                       int encoding)
{
    register unsigned int ix;

    tv_term_list terms = root->terms;
    for (ix = 0; ix < terms->sz; ix++) {
        sort_int_list(terms->arr[ix]->pos);
        sort_int_list(terms->arr[ix]->neg);
    }
    if (!cardinality_sort_terms(tv_cache, mv_cache, terms, encoding)) {
        return 0;
    }

    for (ix = 0; ix < root->kids->sz; ix++) {
        dnf_tree_sort(tree,
                      root->kids->arr[ix],
                      tv_cache,
                      mv_cache,
                      encoding);
    }
    return root;
}

dnf_tree dnf_tree_make(const_hierarchy input,
                       const_node root,
                       int_list_list variable_mappings,
                       unsigned int *current_map)
{
    register unsigned int ix;
    register unsigned int iy;

    tv_term_list terms;
    dnf_tree result;
    dnf_tree kid_tree;
    int_list pos;
    int_list neg;
    array kids;
    node kid;

    if (NULL == (terms = rdup_tv_term_list(to_tv_dnf(root->constraints)->terms))) {
        return NULL;
    }
    if (NULL == (kids = array_new(dnf_tree_free, dnf_tree_copy))) {
        rfre_tv_term_list(terms);
        return NULL;
    }
    if (NULL == (result = dnf_tree_new(root->type, terms, kids))) {
        rfre_tv_term_list(terms);
        array_free(kids);
        return NULL;
    }

    for (ix = 0; ix < result->terms->sz; ix++) {
        pos = result->terms->arr[ix]->pos;
        neg = result->terms->arr[ix]->neg;
        for (iy = 0; iy < pos->sz; iy++) {
            pos->arr[iy] = variable_mappings->arr[*current_map]->arr[pos->arr[iy]];
        }
        for (iy = 0; iy < neg->sz; iy++) {
            neg->arr[iy] = variable_mappings->arr[*current_map]->arr[neg->arr[iy]];
        }
        result->terms->arr[ix]->pos = sort_int_list(result->terms->arr[ix]->pos);
        result->terms->arr[ix]->neg = sort_int_list(result->terms->arr[ix]->neg);
    }

    *current_map += 1;

    for (ix = 0; ix < root->edges->sz; ix++) {
        if (nodeNIL == (kid = find_node(input, root->edges->arr[ix]->type))) {
            assert(0); /* To Do: To internal error. */
            abort();
            break;
        }
        kid_tree = dnf_tree_make(input, kid, variable_mappings, current_map);
        if (NULL == kid_tree) {
            dnf_tree_free(result);
            return NULL;
        }
        if (!array_append(result->kids, kid_tree)) {
            dnf_tree_free(result);
            dnf_tree_free(kid_tree);
            return NULL;
        }
    }

    return result;
}

static int terms_consistent(const_tv_term x, const_tv_term y)
{
    unsigned int ix, iy;

    for (ix = 0; ix < x->pos->sz; ix++) {
        for (iy = 0;iy < y->neg->sz; iy++) {
            if (x->pos->arr[ix] == y->neg->arr[iy]) {
                return 0;
            }
        }
    }
    for (ix = 0;ix < x->neg->sz; ix++) {
        for (iy = 0;iy < y->pos->sz; iy++) {
            if (x->neg->arr[ix] == y->pos->arr[iy]) {
                return 0;
            }
        }
    }
    return 1;
}

static tv_term_list reduce_alpha(tv_term_list tl, const_tv_term alpha)
{
    register unsigned int ix;

    tv_term_list result = new_tv_term_list();
    if (NULL == result) {
        return tv_term_listNIL;
    }

    for (ix = 0; ix < tl->sz; ix++) {
        if (terms_consistent(tl->arr[ix], alpha)) {
            if (!append_tv_term_list(result, rdup_tv_term(tl->arr[ix]))) {
                rfre_tv_term_list(result);
                return tv_term_listNIL;
            }
        }
    }

    return result;
}

dnf_tree dnf_tree_filter(dnf_tree root, const_tv_term alpha)
{
    register unsigned int ix;

    tv_term_list terms = reduce_alpha(root->terms, alpha);
    if (NULL == terms) {
        return NULL;
    }

    rfre_tv_term_list(root->terms);
    root->terms = terms;
    for (ix = 0; ix < root->kids->sz; ix++) {
        if (NULL == dnf_tree_filter(root->kids->arr[ix], alpha)) {
            return NULL;
        }
    }

    return root;
}
