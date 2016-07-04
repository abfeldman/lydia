#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "util.h"

char *read_line(FILE *fp, size_t buffer_size)
{
    char *result = NULL;

    size_t string_length;

    result = (char *)malloc(buffer_size * sizeof(char));
    if (NULL == result) {
        return NULL;
    }

    if (fgets(result, buffer_size, fp) == NULL) {
        free(result);
        return NULL;
    }

    string_length = strlen(result);

    while (string_length > 0 && 
           (result[string_length - 1] == '\n' ||
            result[string_length - 1] == '\r')) {
        string_length -= 1;
    }
    result[string_length] = '\0';

    if (string_length == 0) {
        free(result);
        return NULL;
    }

    result = (char *)realloc(result, string_length + 1);
    assert(result != NULL); /* We are shrinking the buffer. */

    return result;
}

void rfre_string_list(char **p, unsigned int parmc)
{
    unsigned int i;

    for (i = 0; i < parmc; i++) {
        free(p[i]);
    }
    free(p);
}

/*
 * Given a string 'p', chop it into words using 'scanword' and return
 * an array of strings containing the parmc.
 */
char **chop_string(const char *p, unsigned int *parmc)
{
    char *s;
    char **sl = NULL, **nl;

    *parmc = 0;
    while (1) {
        if (NULL == (p = scanword(p, &s))) {
            free(sl);
            return NULL;
        }
        if (NULL == s) {
            break;
        }
        nl = (char **)realloc(sl, sizeof(char *) * (*parmc + 1));
        if (NULL == nl) {
            return sl;
        }
        sl = nl;
        sl[*parmc] = s;
        *parmc += 1;
    }
    return sl;
}

/*
 * Given a pointer to a string 's' and a pointer to a character pointer 'w',
 * scan the string for a 'word'.
 * 
 * First, all characters matching isspace() are skipped.
 * After that a word is scanned, a new buffer is allocated for the word,
 * and assigned to '*w'. If there is no other word in the string, '*w'
 * is set to NULL.
 *
 * A word is one of the following regular expressions:
 * [^ \t\n\r\f\0]+: An arbitrary tmstring of nonblanks and non-specials.
 * "[^"\0]*": Arbitrary characters surrounded by "".
 */
const char *scanword(const char *s, char **w)
{
    char *buf;
    const char *start;
    const char *end;
    size_t room;
    unsigned int i;

    while (isspace(*s)) {
                s++;
    }
    if (*s == '\0') {
        *w = NULL;
        return s;
    }
    if (*s == '\"') {
        s++;
        start = s;
        while (*s != '\"' && *s != '\0') {
            if (*s == '\\') {
                s++;
            }
            s++;
        }
        end = s;
        if (*s != '\"') {
            return NULL;
        } else {
            s++;
        }
    } else {
        start = s;
        while (*s != '\0' && !isspace(*s)) {
            s++;
        }
        end = s;
    }
    room = (size_t)(end - start);
    buf = malloc(sizeof(char) * (room + 1));
    i = 0;
    while (start < end) {
        if (*start == '\\' && start + 1 < end) {
            start++;
        }
        buf[i++] = *start++;
    }
    buf[i] = '\0';
    *w = buf;
    return s;
}

/*
 * Given a string 's', return it, but with all the leading and trailing
 * whitespace removed.
 */
char *stripwhite(char *s)
{
    size_t pos;

    if (s == NULL) {
        return s;
    }

/* Strip trailing whitespace by overwriting it with '\0'. */
    pos = strlen(s);
    while (pos > 0) {
        pos--;
        if (!isspace(s[pos])) {
            break;
        }
        s[pos] = '\0';
    }

/*
 * Strip leading whitespace by copying the remainder of the string
 * over it.
 */
    if (isspace(s[0])) {
        int i = 0;
        pos = 1;

        while (isspace(s[pos])) {
            pos++;
        }
        while (s[pos] != '\0') {
            s[i++] = s[pos++];
        }
        s[i] = '\0';
    }
    return s;
}

FILE *ckfopen(const char *nm, const char *acc)
{
    FILE *hnd = fopen(nm, acc);
    if (NULL == hnd) {
        fprintf(stderr, "%s: %s\n", strerror(errno), nm);
        exit(1);
    }
    return hnd;
}

int parse_int_value(const char *s, int *value)
{
    char *p = NULL;
    *value = strtol(s, &p, 10);
    if (NULL == p || '\0' != *p) {
        return 0;
    }
    return 1;
}

int parse_float_value(const char *s, double *value)
{
    char *p = NULL;
    *value = strtod(s, &p);
    if (NULL == p || '\0' != *p) {
        return 0;
    }
    return 1;
}

int parse_bool_value(const char *s, int *value)
{
    if (strcmp(s, "true") == 0 || strcmp(s, "1") == 0) {
        *value = 1;
        return 1;
    }
    if (strcmp(s, "false") == 0 || strcmp(s, "0") == 0) {
        *value = 0;
        return 1;
    }
    return 0;
}
