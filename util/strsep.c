#include "config.h"

#include <stdio.h>
#include <string.h>

#ifndef HAVE_STRSEP
char *strsep(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
 
    if ((s = *stringp) == NULL) {
        return NULL;
    }
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0) {
                    s = NULL;
                } else {
                    s[-1] = 0;
                }
                *stringp = s;
                return tok;
            }
        } while (sc != 0);
    }
    /* Never reached. */
}
#endif
