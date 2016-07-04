#ifndef TYPES_H
#define TYPES_H

#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LYDIA_TRUE_STR  "t"
#define LYDIA_FALSE_STR "nil"

#define LYDIA_FALSE     0
#define LYDIA_TRUE      1

#define lydia_boolNIL   0
#define lydia_symbolNIL 0

typedef int lydia_bool;
typedef struct str_lydia_symbol *lydia_symbol;
typedef const struct str_lydia_symbol *const_lydia_symbol;

struct str_lydia_symbol
{
    char *name;
    void *data;
};

extern void init_symtbl();
extern void destroy_symtbl();
extern lydia_symbol add_lydia_symbol(const char *name);

#define rdup_lydia_bool(s) (s)
#define rfre_lydia_bool(s)
#define fre_lydia_bool(s)
#define cmp_lydia_bool(a, b) (((a) == (b)) ? 0 : 1)
#define isequal_lydia_bool(a, b) (a == b)
#define null_lydia_bool() lydia_boolNIL

#define rdup_lydia_symbol(s) (s)
#define rfre_lydia_symbol(s)
#define fre_lydia_symbol(s)
#define cmp_lydia_symbol(a, b) (((a) == (b)) ? 0 : strcmp((a)->name, (b)->name))
#define isequal_lydia_symbol(a, b) (a == b)
#define null_lydia_symbol() lydia_symbolNIL

/* 'int' functions */
#define rdup_int(i) (i)
#define fre_int(i)
#define rfre_int(i)
#define cmp_int(a,b) ((a) - (b))
#define isequal_int(a,b) ((a) == (b))
#define intNIL (0)
#define null_int() intNIL

/* 'double' functions */
#define rdup_double(d) (d)
#define fre_double(d)
#define rfre_double(d)
#define doubleNIL (0.0)
#define null_double() doubleNIL
extern int cmp_double(const double a, const double b);

extern int lineno;
extern char errmsg[];

#ifdef __cplusplus
}
#endif

#endif
