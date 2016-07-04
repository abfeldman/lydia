#include "config.h"

#include <stdio.h>
#include <string.h>

#ifndef HAVE_STRNLEN
size_t strnlen(const char *string, size_t maxlen)
{
    const char *end = memchr(string, '\0', maxlen);
    return end ? (size_t) (end - string) : maxlen;
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

