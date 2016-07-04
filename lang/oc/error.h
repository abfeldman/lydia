#ifndef ERROR_H
#define ERROR_H

#include "oc.h"

#define LOEH_MAXNAMELEN                           512
#define LOEH_ERRARGLEN                            256

typedef struct str_oc_error_message *oc_error_message;

struct str_oc_error_message
{
    int type;
    int id;
    const char *msg;
    const char *legend;
    int cnt;
};

#define ERR_UNDEFINED_IDENTIFIER                    1

#define COMPILER_OPEN_ERROR                      3001
#define COMPILER_INCLUDE_ERROR                   3002

#define LOEH_LOCATION_GLOBAL                        1
#define LOEH_LOCATION_OBSERVATION                   0

#define LOEH_ERR                                    0
#define LOEH_WARN                                   1
#define LOEH_INTERNAL                               2
#define LOEH_COMPILER                               3

/* Function prototypes */
extern void loeh_error(int, int, const_obs_orig, ...);
extern void oc_parser_error(const char *, int, const char *, ...);

#endif
