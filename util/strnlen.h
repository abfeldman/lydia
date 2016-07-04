#ifndef __STRNLEN_H__
#define __STRNLEN_H__

#include "config.h"

#ifndef HAVE_STRNLEN
extern size_t strnlen(const char *string, size_t maxlen);
#endif

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
