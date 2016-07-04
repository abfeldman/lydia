#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern char *read_line(FILE *, size_t);

/* String utilities. */
extern void rfre_string_list(char **, unsigned int);
extern const char *scanword(const char *, char **);
extern char **chop_string(const char *, unsigned int *);
extern char *stripwhite(char *);

/* File utilities. */
extern FILE *ckfopen(const char *, const char *);

/* Numeric value parsing. */
extern int parse_int_value(const char *, int *);
extern int parse_float_value(const char *, double *);
extern int parse_bool_value(const char *, int *);

#ifdef __cplusplus
}
#endif

#endif
