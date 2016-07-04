#ifndef ATMS_H
#define ATMS_H

#include "tv.h"
#include "trie.h"
#include "array.h"

typedef struct str_atms *atms;
typedef struct str_atms_node *atms_node;
typedef struct str_atms_justification *atms_justification;

struct str_atms                                /* This is the ATMS engine context. */
{
    array nodes;
    array justifications;
};

struct str_atms_node
{
    signed int index;                          /* Variable name. */

    trie label;

    signed char assumption;
    signed char contradiction;

/*
 * An array of pointers to justifications which contain this node as
 * an antecedent.
 */
    array consequences;
};

struct str_atms_justification
{
    array antecedents;
    atms_node consequent;
};

/* Interface: */
extern atms atms_new();
extern void atms_free(atms);

extern int atms_add_node(atms, const signed char, const signed char);
extern int atms_add_justification(atms, const_material_implication);

extern array atms_get_labels(atms, int);
extern array atms_get_nogoods(atms);

#endif
