#ifndef QSORT_H
#define QSORT_H

#include <stdio.h>

typedef int (* lydia_compare_func_t)(const void *, const void *);

extern void lydia_qsort(void *, size_t, size_t, lydia_compare_func_t);

#endif
