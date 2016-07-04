#ifndef __FREAD_H__
#define __FREAD_H__

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

extern int fread_double(FILE *f, double *t);
extern int fread_float(FILE *f, float *t);
extern int fread_int(FILE *f, int *t);
extern int fread_unsigned(FILE *f, unsigned int *t);
extern int fread_lydia_bool(FILE *f, lydia_bool *t);
extern int fread_lydia_symbol(FILE *f, lydia_symbol *t);

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
