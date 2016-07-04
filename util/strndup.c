#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strnlen.h"

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n)
{
    size_t len = strnlen(s, n);
    char *new = malloc(len + 1);

    if (new == NULL) {
	return NULL;
    }

    new[len] = '\0';
    return (char *)memcpy (new, s, len);
}
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

