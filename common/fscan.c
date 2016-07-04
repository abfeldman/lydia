#include <ctype.h>

#include "defs.h"
#include "fscan.h"
#include "config.h"

int lineno = 1;

int is_nil_symbol(FILE *fp)
{
    int d, n, i, l;

    if ('$' != (d = fgetc(fp))) {
        ungetc(d, fp);
        return 0;
    }
    if ('n' != (n = fgetc(fp))) {
        ungetc(n, fp);
        ungetc(d, fp);
        return 0;
    }
    if ('i' != (i = fgetc(fp))) {
        ungetc(i, fp);
        ungetc(n, fp);
        ungetc(d, fp);
        return 0;
    }
    if ('l' != (l = fgetc(fp))) {
        ungetc(l, fp);
        ungetc(i, fp);
        ungetc(n, fp);
        ungetc(d, fp);
        return 0;
    }
    return 1;
}

static int is_nil(FILE *fp)
{
    int n, i, l;

    if ('n' != (n = fgetc(fp))) {
        ungetc(n, fp);
        return 0;
    }
    if ('i' != (i = fgetc(fp))) {
        ungetc(i, fp);
        ungetc(n, fp);
        return 0;
    }
    if ('l' != (l = fgetc(fp))) {
        ungetc(l, fp);
        ungetc(i, fp);
        ungetc(n, fp);
        return 0;
    }
    return 1;
}

static int is_t(FILE *fp)
{
    int t;

    if ('t' != (t = fgetc(fp))) {
        ungetc(t, fp);
        return 0;
    }
    return 1;
}

void skip_spaces(FILE *fp)
{
    int c = fgetc(fp);
    if (c == '\n') {
        lineno += 1;
    }
    while (isspace(c)) {
        c = fgetc(fp);
        if (c == '\n') {
            lineno += 1;
        }
    }
    ungetc(c, fp);
}

int fscan_colon(FILE *fp)
{
    int c = fgetc(fp);
    if (c == ':') {
        return 1;
    }
    ungetc(c, fp);
    return 0;
}

int fscan_lbracket(FILE *fp)
{
    int c = fgetc(fp);
    if (c == '(') {
        return 1;
    }
    ungetc(c, fp);
    return 0;
}

int fscan_rbracket(FILE *fp)
{
    int c = fgetc(fp);
    if (c == ')') {
        return 1;
    }
    ungetc(c, fp);
    return 0;
}

int fscan_string(FILE *fp, char **ptr)
{
    register unsigned int ix = 0, iy = 16;
    char *buf, *nbuf;

    if (NULL == (buf = (char *)malloc(iy * sizeof(char)))) {
        return 0;
    }
    while (1) {
        int c = fgetc(fp);
        if (!isalnum(c) && c != '_' && c != '$' && c != '#' && c != '~') {
            ungetc(c, fp);
            break;
        }
        if (ix + 2 > iy) {
            iy += iy + 2;
            nbuf = (char *)realloc(buf, iy * sizeof(char));
            if (NULL == nbuf) {
                return 0;
            }
            buf = nbuf;
        }
        buf[ix++] = c;
    }
    buf[ix] = '\0';
    *ptr = buf;

    return 1;
}

int fscan_double(FILE *fp, double *t)
{
    skip_spaces(fp);
    *t = 0.0;
    if (fscanf(fp, "%lg", t) != 1) {
        strcpy(errmsg, "double expected");
        return 0;
    }
    return 1;
}

int fscan_float(FILE *fp, float *t)
{
    skip_spaces(fp);
    *t = 0.0;
    if (fscanf(fp, "%g", t) != 1) {
        strcpy(errmsg, "float expected");
        return 0;
    }
    return 1;
}

int fscan_int(FILE *fp, int *t)
{
    skip_spaces(fp);
    *t = 0;
    if (fscanf(fp, "%d", t) != 1) {
        strcpy(errmsg, "int expected");
        return 0;
    }
    return 1;
}

int fscan_unsigned(FILE *fp, unsigned int *t)
{
    skip_spaces(fp);
    *t = 0;
    if (fscanf(fp, "%u", t) != 1) {
        strcpy(errmsg, "int expected");
        return 0;
    }
    return 1;
}

int fscan_lydia_bool(FILE *fp, lydia_bool *t)
{
    skip_spaces(fp);
    if (is_t(fp)) {
        *t = LYDIA_TRUE;
        return 1;
    } else if (is_nil(fp)) {
        *t = LYDIA_FALSE;
        return 1;
    }
    return 0;
}

int fscan_lydia_symbol(FILE *fp, lydia_symbol *t)
{
    char *symstr = NULL;

    skip_spaces(fp);
    if (!fscan_string(fp, &symstr)) {
        free(symstr);
        return 0;
    }
    if (0 == strcmp("$nil", symstr)) {
        *t = lydia_symbolNIL;
        free(symstr);
        return 1;
    }
    *t = add_lydia_symbol(symstr);
    free(symstr);

    return 1;
}
