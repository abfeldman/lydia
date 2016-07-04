#include "defs.h"
#include "fread.h"
#include "config.h"

int fread_double(FILE *f, double *t)
{
    return 1 != fread(t, sizeof(double), 1, f);
}

int fread_float(FILE *f, float *t)
{
    return 1 != fread(t, sizeof(float), 1, f);
}

int fread_int(FILE *f, int *t)
{
    int result = (1 != fread(t, sizeof(int), 1, f));

#if !WORDS_BIGENDIAN
    *t = swapint(*t);
#endif

    return result;
}

int fread_unsigned(FILE *f, unsigned int *t)
{
    int result = (1 != fread(t, sizeof(unsigned int), 1, f));

#if !WORDS_BIGENDIAN
    *t = swapint(*t);
#endif

    return result;
}

int fread_lydia_bool(FILE *f, lydia_bool *t)
{
    int result = (1 != fread(t, sizeof(lydia_bool), 1, f));

#if !WORDS_BIGENDIAN
    *t = swapint(*t);
#endif

    return result;
}

int fread_lydia_symbol(FILE *f, lydia_symbol *t)
{
    unsigned int length;
    char *sym;
    if (fread_unsigned(f, &length)) {
        return 1;
    }
    if (-1 == (int)length) {
        *t = lydia_symbolNIL;
        return 0;
    }
    sym = (char *)malloc(length + 1);
    if (NULL == sym) {
        return 1;
    }
    if (length != fread(sym, sizeof(char), length, f)) {
        return 1;
    }
    sym[length] = '\0';
    *t = add_lydia_symbol(sym);
    free(sym);
    return 0;
}
