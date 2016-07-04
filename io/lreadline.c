/* A dummy replacement implementation for readline. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "lreadline.h"

#define LREADLINESIZE 2048

/* Provide our own (stupid) readline function. */
char *lreadline(const char *prompt)
{
    char buf[LREADLINESIZE + 1], *p, *s;
    size_t sz;

    if (prompt != NULL){
        fputs(prompt, stdout);
    }
    p = fgets(buf, LREADLINESIZE, stdin);
    if (p == NULL) {
        return NULL;
    }
    sz = strlen(buf);
/*
 * Leo: Strip possible trailing newline, because the
 *      libreadline() function also does that, and if this
 *      dummy version doesn't behave the same way it messes up
 *      the testfiles later on (e.g. when the tests echo back
 *      the line they just read).
 */   
    if (buf[sz - 1] == '\n') {
        buf[sz - 1] = '\0';
        sz--;
    }
    s = (char *)malloc(sz + 1);
    strcpy(s, buf);
    return s;
}
