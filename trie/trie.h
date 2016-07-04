/**
 *  \file trie.h
 *  \brief Trie interface.
 */

#ifndef TRIE_H
#define TRIE_H

#include "array.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (* trie_node_destroy_func_t)(void *);
typedef void *(* trie_node_clone_func_t)(const void *);

typedef void (* trie_node_visit_func_t)(void *, void *);
typedef signed char (* trie_node_remove_func_t)(void *, void *);

typedef void * (* trie_terminal_operator_func_t)(void *, void *);

typedef struct str_trie *trie;
typedef struct str_trie_node *trie_node;

struct str_trie
{
    trie_node root;

    unsigned int node_count;
    unsigned int deleted_nodes;

    trie_node_destroy_func_t destroy;
    trie_node_clone_func_t clone;
};

struct str_trie_node
{
    array edges;
    array kids;
/**
 * @todo "is_terminal" and "is_deleted" should be combined in a
 * single bitfield named "flags".
 */
    signed char is_terminal;
    signed char is_deleted;
/** @todo The trie pointer should be removed. */
    trie trie_;
    void *value;
};

extern trie trie_new(trie_node_destroy_func_t, trie_node_clone_func_t);
extern trie trie_copy(trie);
extern void trie_free(trie);

extern int trie_contains_key(trie, array);
extern int trie_element_subsumed_by_key(trie, array);
extern signed char trie_is_empty(trie);
extern signed char trie_add(trie, array, void *);
extern signed char trie_add_trie(trie, trie);
extern signed char trie_multiply(trie, array, void *);
extern signed char trie_multiply_trie(trie, trie, trie_terminal_operator_func_t);
extern signed char trie_is_subsumed(trie, array);
extern signed char trie_is_subsumed_fixed(trie, array);
extern void trie_remove_subsumed(trie, array);
extern void trie_remove_subsumed_trie(trie, trie);
extern void trie_remove_not_subsumed_trie(trie, trie);

extern array trie_get_all_values(trie);
extern void trie_nodes_visit(trie, trie_node_visit_func_t, void *);
extern void trie_gc(trie);

extern void pp_trie_values_int_array(FILE* , trie );

#ifdef __cplusplus
}
#endif

#endif
