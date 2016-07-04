#include "config.h"
#include "strdup.h"
#include "strsep.h"
#include "error.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

struct str_oc_error_message loeh_messages[] =
{
    { LOEH_ERR, ERR_UNDEFINED_IDENTIFIER, "%o`%s' undefined (first use in this observation)", "Each undefined identifier is reported only once\nfor each observation it appears in.", 0 },

    { LOEH_COMPILER, COMPILER_OPEN_ERROR, "%o%s: %s", NULL, 0 },
    { LOEH_COMPILER, COMPILER_INCLUDE_ERROR, "%o%s: #include expects \"FILENAME\"", NULL, 0 },

    { -1, -1, NULL, NULL, -1 }
};

static char errpos[LOEH_ERRARGLEN] = "";
static char *last = NULL;

static void orig_errpos(const char *fnm, int lineno)
{
    sprintf(errpos, "%s:%d", fnm, lineno);
}

static void vmessage(const char *msg, va_list args)
{
    if (errpos[0] != '\0') {
        fprintf(stderr, "%s: ", errpos);
    }
    vfprintf(stderr, msg, args);
    fputs("\n", stderr);
    errpos[0] = '\0';
}

void oc_parser_error(const char *nm, int lineno, const char *msg, ...)
{
    va_list args;

    orig_errpos(nm, lineno);
    va_start(args, msg);
    vmessage(msg, args);
    va_end(args);
}

static void loeh_memory_error(const_obs_orig org)
{
    fprintf(stderr, "%s:%d: critical: memory allocation error\n", org->file->name, org->line);
    fprintf(stderr, "%s:%d: critical: aborting compilation\n", org->file->name, org->line);
/*
 * To Do: Try to back-jump to a place where we can collect the garbage
 * and abort in a more graceful way.
 */
    exit(0);
}

static char *loeh_error_type(int type)
{
    switch (type) {
        case LOEH_ERR:
            return "error";
        case LOEH_WARN:
            return "warning";
        case LOEH_INTERNAL:
            return "critical";
        case LOEH_COMPILER:
            return NULL;
    }
    assert(0);
    abort();
}

void loeh_error(int id, int location, const_obs_orig org, ...)
{
    va_list args;

    register unsigned int ix = 0, iy;

    int type;

    obs_orig o;
    char *s;
    int d;
    double f;

    while (id != loeh_messages[ix].id) {
        if (NULL == loeh_messages[ix].msg) {
            assert(0); /* To Do: To internal error. */
            break;
        }
        ix += 1;
    }

    type = loeh_messages[ix].type;

    va_start(args, org);
    if (LOEH_LOCATION_OBSERVATION == location) {
        char *where = va_arg(args, char *);
        if (NULL == last || 0 != strcmp(last, where)) {
            last = strdup(where);
            fprintf(stderr, "%s: In observation `%s':\n", org->file->name, where);
        }
    }

    for (iy = 0; '\0' != loeh_messages[ix].msg[iy]; iy++) {
        if (loeh_messages[ix].msg[iy] == '%') {
            iy += 1;
            switch (loeh_messages[ix].msg[iy]) {
                case 'o': /* origin */
                    if (org == obs_origNIL) {
                        fprintf(stderr, "oc: ");
                        continue;
                    }
                    if (type == LOEH_COMPILER) {
                        fprintf(stderr, "%s:%d: ", org->file->name, org->line);
                        continue;
                    }
                    fprintf(stderr, "%s:%d: %s: ", org->file->name, org->line, loeh_error_type(type));
                    continue;
                case 'p': /* origin */
                    o = va_arg(args, obs_orig);
                    fprintf(stderr, "%s:%d: %s: ", o->file->name, o->line, loeh_error_type(type));
                    continue;
                case 's':
                    s = va_arg(args, char *);
                    fprintf(stderr, "%s", s);
                    continue;
                case 'd':
                    d = va_arg(args, int);
                    fprintf(stderr, "%d", d);
                    continue;
                case 'g':
                    f = va_arg(args, double);
                    fprintf(stderr, "%g", f);
                    continue;
                case 'f':
                    f = va_arg(args, double);
                    fprintf(stderr, "%f", f);
                    continue;
            }
        }
        fputc(loeh_messages[ix].msg[iy], stderr);
    }
    va_end(args);
    fputc('\n', stderr);

    if (NULL != loeh_messages[ix].legend && 0 == loeh_messages[ix].cnt) {
        char *p, *legend = (char *)malloc(sizeof(char) * (strlen(loeh_messages[ix].legend) + 3));
        if (NULL == legend) {
            loeh_memory_error(org);
        }
        sprintf(legend, "(%s)", loeh_messages[ix].legend);
        while (NULL != (p = strsep(&legend, "\n"))) {
            fprintf(stderr, "%s:%d: %s: %s\n", org->file->name, org->line, loeh_error_type(type), p);
        }
        free(legend);
    }
    loeh_messages[ix].cnt += 1;

    fflush(stderr);
}
