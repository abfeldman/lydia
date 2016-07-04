#ifndef FPRINT_H
#define FPRINT_H

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "array.h"

#define PRINT_ITEM_CONS       1
#define PRINT_ITEM_WORD       2
#define PRINT_ITEM_LIST       3
#define PRINT_ITEM_EMPTY_LIST 4
#define PRINT_ITEM_SYMBOL     5
#define PRINT_ITEM_INT        6
#define PRINT_ITEM_UNSIGNED   7

typedef struct str_print_state *print_state;
typedef struct str_print_item *print_item;

struct str_print_state
{
    FILE *file;
    print_item root;
    print_item current;

    size_t iwidth;
    size_t tabsize;
    size_t wrap;
    size_t pos;
    unsigned int level;
};

struct str_print_item
{
    unsigned char type;
    union
    {
        char *word;
        lydia_symbol symbol;
        array list;
        int ival;
        unsigned int uval;
    } u;
    size_t len;
    print_item parent;
};

extern print_state set_print(FILE *, const int, const int, const int);
extern void end_print(print_state);

extern void open_cons(print_state, const char *);
extern void close_cons(print_state);
extern void open_list(print_state);
extern void close_list(print_state);

extern void fprint_double(print_state, double);
extern void fprint_float(print_state, float);
extern void fprint_int(print_state, int);
extern void fprint_unsigned(print_state, unsigned int);
extern void fprint_lydia_bool(print_state, lydia_bool);
extern void fprint_lydia_symbol(print_state, lydia_symbol);

#endif
