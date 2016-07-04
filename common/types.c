/**
 *  \file types.c
 *  \brief Lydia built-in types and symbol table.
 */

#include "strdup.h"
#include "types.h"
#include "hash.h"

#include <stdlib.h>
#include <string.h>

static hash_table symtbl;

char errmsg[128] = "";

static void lydia_symbol_dtor(void *symbol)
{
    free((*(lydia_symbol *)symbol)->name);
    if (NULL != (*(lydia_symbol *)symbol)->data) {
	free((*(lydia_symbol *)symbol)->data);
    }
    free(*(lydia_symbol *)symbol);
}

void init_symtbl()
{
    hash_init(&symtbl, 2, NULL, lydia_symbol_dtor);
}

void destroy_symtbl()
{
    hash_destroy(&symtbl);
}

lydia_symbol add_lydia_symbol(const char *name)
{
    void *symbol = NULL;
    if (0 == hash_find(&symtbl,
		       name,
		       (unsigned int)strlen(name),
		       &symbol)) {
	return *(lydia_symbol *)symbol;
    }
    symbol = malloc(sizeof(struct str_lydia_symbol));
    if (NULL == symbol) {
	return NULL;
    }
    ((lydia_symbol)symbol)->name = strdup(name);
    ((lydia_symbol)symbol)->data = NULL;
    hash_add(&symtbl,
	     name,
	     (unsigned int)strlen(name),
	     &symbol,
	     sizeof(lydia_symbol),
	     NULL);
    return symbol;
}

int cmp_double(const double a, const double b)
{
     if (a > b) {
         return 1;
     }
     if (a < b) {
         return -1;
     }
     return 0;
}

/*
 * Local variables:
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
