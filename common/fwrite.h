#ifndef __FWRITE_H__
#define __FWRITE_H__

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

extern void init_hash(const void *header, const unsigned int length);
extern void save_hash(unsigned char *h);

extern void fwrite_double(FILE *f, const double t);
extern void fwrite_float(FILE *f, const float t);
extern void fwrite_int(FILE *f, const int t);
extern void fwrite_unsigned(FILE *f, const unsigned int t);
extern void fwrite_lydia_bool(FILE *f, const lydia_bool t);
extern void fwrite_lydia_symbol(FILE *f, const lydia_symbol t);

#endif

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
