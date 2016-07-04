#ifndef IO_H
#define IO_H

#include "variable.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*commandfn)(const void *, FILE *, const char **, const unsigned);
typedef void (*sighandler_t)(int);

typedef struct str_command {
    const char *cmd;
    commandfn fn;
    unsigned int min_parms;
    unsigned int max_parms;
    const char *help;
} command;

#define MAX_COMMANDS         32
#define MAX_MODULE_NAME_LEN 256

/* Output delimiter subroutines: */
extern void lydia_ok(FILE *, const char *);
extern void lydia_start_output(FILE *, const char *);
extern void lydia_stop_output(FILE *, const char *);
extern void lydia_output(FILE *, char *, const char *, ...);
extern void lydia_info(FILE *, const char *, ...);
extern void lydia_error(FILE *, const char *, ...);
extern void lydia_assert(FILE *, int, int, const char *, ...);
extern void lydia_protocol_error(FILE *, const char *, ...);

/* Command line subroutines: */
extern void cli_init(char *, int, int);
extern void cli_loop(const void *, FILE *, char *, const command *, FILE *);
extern void cli_destroy();

extern void enable_statistics();
extern void enable_diagnosis();
extern void enable_listings();

extern void pp_fm(FILE *,
                  const_faultmode,
                  const unsigned int,
                  const_variable_list,
                  const_variable_list,
                  const_values_set_list,
                  const int);
extern void pp_card(FILE *, const_faultmode, const unsigned int);

#ifdef __cplusplus
}
#endif

#endif
