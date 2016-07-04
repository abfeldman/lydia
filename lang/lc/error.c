#include "config.h"
#include "strdup.h"
#include "strsep.h"
#include "error.h"

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

struct str_error_message leh_messages[] =
{
    { LEH_ERR, ERR_UNDEFINED_TYPE, "%o`%s' undefined here (neither in a system nor in a function)", "Each undefined type is reported only once.", 0 },
    { LEH_ERR, ERR_UNDEFINED_IDENTIFIER, "%o`%s' undefined (first use in this system)", "Each undefined identifier is reported only once\nfor each system it appears in.", 0 },
    { LEH_ERR, ERR_REDEFINED_IDENTIFIER, "%oredefinition of `%s'\n%p`%s' previously defined here", NULL, 0 },
    { LEH_ERR, ERR_REDEFINED_ATTRIBUTE, "%oredefinition of `%s(%s)'\n%p`%s(%s)' previously defined here", NULL, 0 },
    { LEH_ERR, ERR_TOO_MANY_ARGUMENTS_FUNCTION, "%otoo many arguments to function `%s'", NULL, 0 },
    { LEH_ERR, ERR_TOO_MANY_ARGUMENTS_SYSTEM, "%otoo many arguments to system `%s'", NULL, 0 },
    { LEH_ERR, ERR_TOO_FEW_ARGUMENTS_FUNCTION, "%otoo few arguments to function `%s'", NULL, 0 },
    { LEH_ERR, ERR_TOO_FEW_ARGUMENTS_SYSTEM, "%otoo few arguments to system `%s'", NULL, 0 },
    { LEH_ERR, ERR_INCOMPATIBLE_ARGUMENT_TYPE_FUNCTION, "%oincompatible type for argument %d to function `%s'", NULL, 0 },
    { LEH_ERR, ERR_INCOMPATIBLE_ARGUMENT_TYPE_SYSTEM, "%oincompatible type for argument %d to system `%s'", NULL, 0 },
    { LEH_ERR, ERR_BAD_ARGUMENT_SIZE_FUNCTION, "%obad size for argument %d to function `%s'", NULL, 0 },
    { LEH_ERR, ERR_BAD_ARGUMENT_SIZE_SYSTEM, "%obad size for argument %d to system `%s'", NULL, 0 },
    { LEH_ERR, ERR_INCOMPATIBLE_RETURN_TYPE, "%oincompatible type in return", NULL, 0 },
/* TODO: The legends of the following two messages should be shown only once. */
    { LEH_ERR, ERR_EVALUATE_ATTRIBUTE_ERROR, "%ocan't evaluate %s(%s) for %s = %s", "Each non-evaluated value is reported only once\nfor each attribute it appears in.", 0 },
    { LEH_ERR, ERR_EVALUATE_ATTRIBUTE_ENUM_ERROR, "%ocan't evaluate %s(%s) for %s = %s.%s", "Each non-evaluated value is reported only once\nfor each attribute it appears in.", 0 },
    { LEH_ERR, ERR_INCOMPATIBLE_INITIALIZER_TYPE, "%oincompatible type in initalizer of `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_ARITH_IF_COND, "%oinvalid condition in ternary expression", NULL, 0 },
    { LEH_ERR, ERR_TYPE_ARITH_IF_VALS, "%otype mismatch in ternary expression", NULL, 0 },
    { LEH_ERR, ERR_TYPE_BINOP, "%oinvalid operands to `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_BINOP_SIZES, "%oinvalid operand sizes to `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_UNOP, "%oinvalid operand to `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_ENUM_NOSEL, "%omissing enumeration selector in `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_ENUM_BADSEL, "%oinvalid enumeration selector in `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_BADSEL, "%oinvalid selector in `%s'", NULL, 0 },
    { LEH_ERR, ERR_MISSING_DEFAULT, "%omissing default statement in conditional expression", NULL, 0 },
    { LEH_ERR, ERR_TYPE_COND, "%otype mismatch in conditional expression", NULL, 0 },
    { LEH_ERR, ERR_TYPE_CAST, "%oinvalid type conversion", NULL, 0 },
    { LEH_ERR, ERR_ALREADY_INSTANTIATED, "%osystem `%s' is already instantiated", NULL, 0 },
    { LEH_ERR, ERR_ALREADY_INSTANTIATED_ARRAY, "%o`%s%s' is already instantiated", "Reporting only the first doubly-instantiated\nelement for each system array.", 0 },
    { LEH_ERR, ERR_TYPE_ATTR, "%oinvalid attribute value type\n%oexpected %s, found %s", NULL, 0 },
    { LEH_ERR, ERR_TYPE_CIRCULAR, "%ocircular definition of `%s'", "Each chain is reported only once.", 0 },
    { LEH_ERR, ERR_TYPE_CIRCULAR_REPORT, "%o`%s' aliases %s`%s' here", NULL, 0 },
    { LEH_ERR, ERR_REDEFINED_INT_ATTR, "%oredefinition of the internal attribute `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_ARRAY_DIMS_DECLARATION, "%oincompatible array type of `%s'\n%pdeclaration is an array of %d dimension%s\n%obut the expression specifies %d dimension%s", NULL, 0 },
    { LEH_ERR, ERR_TYPE_ARRAY_DIMS, "%oincompatible array type of `%s'\n%preferee is an array of %d dimension%s\n%obut the referrer specifies %d dimension%s", NULL, 0 },
    { LEH_ERR, ERR_TYPE_ARRAY_DIMS_INSTANTIATION, "%oincompatible array type of `%s'\n%preferee is an array of %d dimension%s\n%obut the referrer specifies %d dimension%s", NULL, 0 },
    { LEH_ERR, ERR_TYPE_SUBSCRIPT, "%osubscript of non-array", NULL, 0 },
    { LEH_ERR, ERR_TYPE_OUT_OF_RANGE, "%osubscript out of range", NULL, 0 },
    { LEH_ERR, ERR_TYPE_EXTENT_SIZE, "%oincompatible size for dimension %d of `%s'\n%preferee has %d element%s in this dimension\n%obut the referrer has %d element%s", NULL, 0 },
    { LEH_ERR, ERR_REDEFINED_STRUCT_VALUE, "%oredefinition of `%s'\n%p`%s' previously defined here", NULL, 0 },
    { LEH_ERR, ERR_TYPE_NOT_STRUCT, "%o`%s' is not a structure", NULL, 0 },
    { LEH_ERR, ERR_TYPE_NOT_ARRAY, "%o`%s' is not an array", NULL, 0 },
    { LEH_ERR, ERR_TYPE_STRUCT_NO_ENTRY, "%ono member `%s' in structure `%s'", NULL, 0 },
    { LEH_ERR, ERR_TYPE_SEQUENCE, "%onon-homogeneous sequence types", NULL, 0 },
    { LEH_ERR, ERR_EMPTY_SEQUENCE, "%oempty sequence", NULL, 0 },
    { LEH_ERR, ERR_MISSING_PROBABILITIES, "%o`%s' has no probability attribute", NULL, 0 },
    { LEH_ERR, ERR_SELF_REFERENCE, "%o`%s' is a self-reference", NULL, 0 },
    { LEH_ERR, ERR_TYPE_PREDICATE, "%opredicate does not evaluate to a Boolean", NULL, 0 },
    { LEH_ERR, ERR_TYPE_IF_COND, "%otype mismatch in conditional", NULL, 0 },

    { LEH_WARN, WARN_UNUSED_VARIABLE, "%ounused variable `%s'", NULL, 0 },
    { LEH_WARN, WARN_UNUSED_VARIABLE_ARRAY, "%ounused variable array `%s'", NULL, 0 },
    { LEH_WARN, WARN_UNUSED_VARIABLE_STRUCTURE, "%ounused variable structure `%s'", NULL, 0 },
    { LEH_WARN, WARN_UNUSED_VARIABLE_ARRAY_ELEMENT, "%ounused variable array element `%s%s'", "Reporting only the first unused\nelement for each array.", 0 },
    { LEH_WARN, WARN_UNUSED_VARIABLE_STRUCTURE_MEMBER, "%ounused variable structure member `%s%s'", "Reporting only the first unused\nmember for each structure.", 0 },
    { LEH_WARN, WARN_UNUSED_SYSTEM, "%ounused system declaration `%s'", NULL, 0 },
    { LEH_WARN, WARN_UNUSED_SYSTEM_ARRAY, "%ounused system array element `%s%s'", "Reporting only the first unused\nelement for each system array.", 0 },
    { LEH_WARN, WARN_UNUSED_SYSTEM_ARRAY_ALL, "%ounused system array declaration `%s'", NULL, 0 },
    { LEH_WARN, WARN_REDEFINED_ENUM_VALUE, "%oredefinition of `%s'\n%p`%s' previously defined here", NULL, 0 },
    { LEH_WARN, WARN_CONST_HEALTH, "%o constant health attribute of `%s'", NULL, 0 },
    { LEH_WARN, WARN_NON_CONST_OBSERVABLE, "%o non-const observable attribute of `%s'", NULL, 0 },
    { LEH_WARN, WARN_BAD_PROBABILITY, "%o the total probability mass of `%s' is out of range", NULL, 0 },
    { LEH_WARN, WARN_BAD_PROBABILITY_ENTRY, "%o the probability value %g of `%s' is out of range", "Each out of range value of an attribute\nis reported only once.", 0 },

    { LEH_INTERNAL, INTERNAL_UNSUPPORTED_EXPR, "%oexpression not supported", NULL, 0 },
    { LEH_INTERNAL, INTERNAL_BRACKET_LEVEL, "bracket level = %d", NULL, 0 },

    { LEH_COMPILER, COMPILER_OPEN_ERROR, "%o%s: %s", NULL, 0 },
    { LEH_COMPILER, COMPILER_INCLUDE_ERROR, "%o%s: #include expects \"FILENAME\"", NULL, 0 },

    { -1, -1, NULL, NULL, -1 }
};

static char errpos[LEH_ERRARGLEN] = "";
static char *last = NULL;

static void leh_memory_error(const_origin org)
{
    fprintf(stderr, "%s:%d: critical: memory allocation error\n", org->file->name, org->line);
    fprintf(stderr, "%s:%d: critical: aborting compilation\n", org->file->name, org->line);
/*
 * To Do: Try to back-jump to a place where we can collect the garbage
 * and abort in a more graceful way.
 */
    exit(0);
}

static char *leh_error_type(int type)
{
    switch (type) {
        case LEH_ERR:
            return "error";
        case LEH_WARN:
            return "warning";
        case LEH_INTERNAL:
            return "critical";
        case LEH_COMPILER:
            return NULL;
    }
    assert(0);
    abort();
}

void leh_error(int id, int location, const_origin org, ...)
{
    va_list args;

    register unsigned int ix = 0, iy;

    int type;

    origin o;
    char *s;
    int d;
    double f;

    while (id != leh_messages[ix].id) {
        if (NULL == leh_messages[ix].msg) {
            assert(0); /* To Do: To internal error. */
            break;
        }
        ix += 1;
    }

    type = leh_messages[ix].type;

    va_start(args, org);
    if (LEH_LOCATION_FUNCTION == location) {
        char *where = va_arg(args, char *);
        if (NULL == last || 0 != strcmp(last, where)) {
            last = strdup(where);
            fprintf(stderr, "%s: In function `%s':\n", org->file->name, where);
        }
    }
    if (LEH_LOCATION_SYSTEM == location) {
        char *where = va_arg(args, char *);
        if (NULL == last || 0 != strcmp(last, where)) {
            last = strdup(where);
            fprintf(stderr, "%s: In system `%s':\n", org->file->name, where);
        }
    }

    for (iy = 0; '\0' != leh_messages[ix].msg[iy]; iy++) {
        if (leh_messages[ix].msg[iy] == '%') {
            iy += 1;
            switch (leh_messages[ix].msg[iy]) {
                case 'o': /* origin */
                    if (org == originNIL) {
                        fprintf(stderr, "lc: ");
                        continue;
                    }
                    if (type == LEH_COMPILER) {
                        fprintf(stderr, "%s:%d: ", org->file->name, org->line);
                        continue;
                    }
                    fprintf(stderr, "%s:%d: %s: ", org->file->name, org->line, leh_error_type(type));
                    continue;
                case 'p': /* origin */
                    o = va_arg(args, origin);
                    fprintf(stderr, "%s:%d: %s: ", o->file->name, o->line, leh_error_type(type));
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
        fputc(leh_messages[ix].msg[iy], stderr);
    }
    va_end(args);
    fputc('\n', stderr);

    if (NULL != leh_messages[ix].legend && 0 == leh_messages[ix].cnt) {
        char *p, *legend = (char *)malloc(sizeof(char) * (strlen(leh_messages[ix].legend) + 3));
        if (NULL == legend) {
            leh_memory_error(org);
        }
        sprintf(legend, "(%s)", leh_messages[ix].legend);
        while (NULL != (p = strsep(&legend, "\n"))) {
            fprintf(stderr, "%s:%d: %s: %s\n", org->file->name, org->line, leh_error_type(type), p);
        }
        free(legend);
    }
    leh_messages[ix].cnt += 1;

    fflush(stderr);

    if (LEH_INTERNAL == type) {
/*
 * To Do: Try to back-jump to a place where we can collect the garbage
 * and abort in a more graceful way.
 */
        exit(EXIT_FAILURE);
    }
}

/* Given an origin 'org', fill 'errpos' with the origin info from 'org'. */
static void orig_errpos(const char *fnm, int lineno)
{
    sprintf(errpos, "%s:%d", fnm, lineno);
}

/* Central handler for all error and warning printing routines. */
static void vmessage(const char *msg, va_list args)
{
    if (errpos[0] != '\0') {
        fprintf(stderr, "%s: ", errpos);
    }
    vfprintf(stderr, msg, args);
    fputs("\n", stderr);
    errpos[0] = '\0';
}

/*
 * Given a filename, a line number, and a message 'msg', 
 * generate an error message.
 */
void lc_parser_error(const char *nm, int lineno, const char *msg, ...)
{
    va_list args;

    orig_errpos(nm, lineno);
    va_start(args, msg);
    vmessage(msg, args);
    va_end(args);
}
