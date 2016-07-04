#ifndef __FSCAN_H__
#define __FSCAN_H__

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

extern int is_nil_symbol(FILE *);
extern void skip_spaces(FILE *);
extern int fscan_colon(FILE *);
extern int fscan_lbracket(FILE *);
extern int fscan_rbracket(FILE *);
extern int fscan_string(FILE *, char **);

extern int fscan_double(FILE *, double *);
extern int fscan_float(FILE *, float *);
extern int fscan_int(FILE *, int *);
extern int fscan_unsigned(FILE *, unsigned int *);
extern int fscan_lydia_bool(FILE *, lydia_bool *);
extern int fscan_lydia_symbol(FILE *, lydia_symbol *);

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
