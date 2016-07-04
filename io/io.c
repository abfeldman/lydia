#include "pp_variable.h"
#include "lreadline.h"
#include "variable.h"
#include "config.h"
#include "defs.h"
#include "util.h"
#include "stat.h"
#include "diag.h"
#include "io.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef WIN32
# include <Winsock2.h>
#else
# include <sys/time.h>
#endif

#ifndef HAVE_DECL_GETOPT_FILENO
extern int fileno(FILE *stream);
#endif

static int option_terminate_stdin = 0;

static int quit_program = 0;
static int version_major = 0;
static int version_minor = 0;
static char module_name[MAX_MODULE_NAME_LEN] = "lydia";

static command std_commands[MAX_COMMANDS];

#ifndef PATH_MAX
# define PATH_MAX 4096
#endif

static char lydia_history[PATH_MAX];

#define LYDIA_HISTORY_NAME "/.lydia_history"

static int stdin_has_data()
{
#ifdef WIN32
    return 0; /* To Do: Implement this. */
#else
    int result, fd;
    struct timeval timeout;
    fd_set read_fds;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    fd = fileno(stdin);
    assert(fd != -1);

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    result = select(FD_SETSIZE, &read_fds, 0, 0, &timeout);

    return (result != 0);
#endif
}

static int is_io_terminate()
{
    if (option_terminate_stdin && stdin_has_data()) {
        return 1;
    }

    return 0;
}

void lydia_assert(FILE *outfile, int test, int errcode, const char *msg, ...)
{
    if (!test) {
        va_list args;

        va_start(args, msg);
        fprintf(outfile, "@ internal error %02d ", errcode);
        vfprintf(outfile, msg, args);
        fputs("\n", outfile);
        va_end(args);
    }
    fflush(outfile);
}

/* Report that everything is ok. */
void lydia_ok(FILE *outfile, const char *command)
{
    fprintf(outfile, "@ ok <%s>\n", command);
    fflush(outfile);
}

/* Report that command output will be next. */
void lydia_start_output(FILE *outfile, const char *command)
{
    fprintf(outfile, "@ start output <%s>\n", command);
    fflush(outfile);
}

/* Report that command output has ended. */
void lydia_stop_output(FILE *outfile, const char *command)
{
    fprintf(outfile, "@ stop output <%s>\n", command);
    fflush(outfile);
}

void lydia_output(FILE *outfile, char *command, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(outfile, "@ output <%s> ", command);
    vfprintf(outfile, msg, args);
    fputs("\n", outfile);
    va_end(args);
    fflush(outfile);
}

void lydia_error(FILE *outfile, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(outfile, "@ error ");
    vfprintf(outfile, msg, args);
    fputs("\n", outfile);
    va_end(args);
    fflush(outfile);
}

void lydia_info(FILE *outfile, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(outfile, "@ info ");
    vfprintf(outfile, msg, args);
    fputs("\n", outfile);
    va_end(args);
    fflush(outfile);
}

void lydia_protocol_error(FILE *outfile, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(outfile, "@ protocol error ");
    vfprintf(outfile, msg, args);
    fputs("\n", outfile);
    va_end(args);
    fflush(outfile);
}

static const command *search_command(const char *nm, const command *commandtab)
{
    register unsigned int ix = 0; 

    while (commandtab[ix].cmd != NULL) {
        if (strcmp(nm, commandtab[ix].cmd) == 0) {
            return &commandtab[ix];
        }
        ix++;
    }
    return NULL;
}

static void execute_command(const void *input,
                            FILE *outfile,
                            const char **parmv,
                            const unsigned int parmc,
                            const command *commandtab)
{
    const command *cmd;

    if (parmc == 0) {
/* Nothing to do. */
        return;
    }
    if (NULL == (cmd = search_command(parmv[0], commandtab))) {
        lydia_error(outfile, "Unknown command `%s'.", parmv[0]);
        return;
    }
    if (parmc < cmd->min_parms + 1 || parmc > cmd->max_parms + 1) {
        if (cmd->min_parms == cmd->max_parms) {
            lydia_protocol_error(outfile,
                                 "Wrong number of arguments (expected %d, received %d).",
                                 cmd->min_parms,
                                 parmc - 1);
            return;
        }
        lydia_protocol_error(outfile,
                             "Wrong number of arguments (expected between %d and %d, received %d).",
                             cmd->min_parms,
                             cmd->max_parms, parmc - 1);
        return;
    }

/* To Do: The next 2 lines should be moved from here. */
    set_terminate(is_io_terminate);
    diagnosis_reset();

    (*cmd->fn)(input, outfile, parmv, parmc);

    fflush(outfile);
}

void cli_init(char *mod_name, int ver_major, int ver_minor)
{
    strncpy(module_name, mod_name, sizeof(module_name));
    version_major = ver_major;
    version_minor = ver_minor;

#ifdef HAVE_LIBREADLINE
    using_history();

    strncpy(lydia_history, getenv("HOME"), sizeof(lydia_history) - 1);
    strncat(lydia_history, LYDIA_HISTORY_NAME, sizeof(lydia_history) - 1);

    read_history(lydia_history);
#endif
}

void cli_loop(const void *input,
              FILE *infile,
              char *prompt,
              const command *commands,
              FILE *outfile)
{
    register unsigned int ix = 0, iy = 0;
    unsigned int params = 0;
    char **parmv;
    int tty = 1;

    while (NULL != std_commands[iy].cmd) {
        iy += 1;
    }
    do {
        memcpy(&std_commands[iy], &commands[ix], sizeof(command));
        iy += 1;
        ix += 1;
    } while (commands[ix].cmd != NULL);

#ifndef WIN32
    tty = isatty(fileno(infile));
    if (!tty) {
        prompt = NULL;
    }
#endif
    while (!quit_program) {
        char *line = NULL;
        if (tty) {
#ifdef HAVE_LIBREADLINE
            line = readline(prompt);
            if (line && *line) {
                char *expansion;

                int result = history_expand(line, &expansion);

                if (result) {
                    fprintf(outfile, "%s\n", expansion);
                }
                if (result < 0 || result == 2) {
                    free(expansion);
                    continue;
                }

                add_history(expansion);
                strncpy(line, expansion, sizeof(line) - 1);
                free(expansion);
            }
#else
            line = lreadline(prompt);
#endif
        } else {
            line = lreadline(prompt);
        }

        if (line == NULL) {
            fputs("\n", outfile);
            break;
        }
        line = stripwhite(line);
        if (line[0] == '#' || line[0] == '\0') {
/* Skip comments and empty lines. */
            free(line);
            continue;
        }
        parmv = chop_string(line, &params);
        free(line);
        if (NULL == parmv) {
/* Unbalanced quotes. */
            assert(0);
        }
        execute_command(input,
                        outfile,
                        (const char **)parmv,
                        params,
                        std_commands);
        rfre_string_list(parmv, params);
    }
}

void cli_destroy()
{
#ifdef HAVE_LIBREADLINE
    write_history(lydia_history);
#endif
}

void pp_fm(FILE *outfile,
           const_faultmode fault,
           const unsigned int num,
           const_variable_list variables,
           const_variable_list encoded_variables,
           const_values_set_list domains,
           const int encoding)
{
    fprintf(outfile, "d%d = { ", num);
    pp_faultmode(outfile,
                 fault,
                 encoding == ENCODING_NONE ? variables : encoded_variables,
                 domains);
    fprintf(outfile, " }\n");
}

static void pp_prob(FILE *outfile,
                    const_faultmode fault,
                    const unsigned int num)
{
    fprintf(outfile, "P(d%d) = %g\n", num, fault->probability);
}

void pp_card(FILE *outfile,
             const_faultmode fault,
             const unsigned int num)
{
    fprintf(outfile, "|d%d| = %d\n", num, fault->cardinality);
}

static void pp_entropy(FILE *outfile, const double entropy)
{
    fprintf(outfile, "H(D) = %g\n", entropy);
}

static void pp_count(FILE *outfile, const unsigned count)
{
    fprintf(outfile, "|D| = %d\n", count);
}

static void pp_list_item(FILE *outfile, const char *item)
{
    fprintf(outfile, "%s\n", item);
}

void cmd_cut(const void *UNUSED(input),
             FILE *outfile,
             const char **parmv,
             const unsigned int parmc)
{
    int i;

    double f;

    if (0 == strcmp("time", parmv[1])) {
        if (parmc == 2) {
            set_max_time(-1);
        } else {
            if (!parse_float_value(parmv[2], &f)) {
                lydia_error(outfile, "Error parsing %s as a floating point value.", parmv[2]);
                return;
            }
            set_max_time(f);
        }
        lydia_ok(outfile, "cut");
        return;
    }

    if (0 == strcmp("diagnoses", parmv[1])) {
        if (parmc == 2) {
            set_max_diagnoses(-1);
        } else {
            if (!parse_int_value(parmv[2], &i)) {
                lydia_error(outfile, "Error parsing %s as an integer value.", parmv[2]);
                return;
            }
            set_max_diagnoses(i);
        }
        lydia_ok(outfile, "cut");
        return;
    }

    if (0 == strcmp("cardinality", parmv[1])) {
        if (parmc == 2) {
            set_max_cardinality(-1);
        } else {
            if (!parse_int_value(parmv[2], &i)) {
                lydia_error(outfile, "Error parsing %s as an integer value.", parmv[2]);
                return;
            }
            set_max_cardinality(i);
        }
        lydia_ok(outfile, "cut");
        return;
    }

    if (0 == strcmp("stdin", parmv[1])) {
        if (parmc == 2) {
            option_terminate_stdin = 0;
        } else {
            if (!parse_bool_value(parmv[2], &option_terminate_stdin)) {
                lydia_error(outfile, "Error parsing %s as a Boolean value.", parmv[2]);
                return;
            }
        }
        lydia_ok(outfile, "cut");
        return;
    }

    if (0 == strcmp("mc", parmv[1])) {
        if (parmc == 2) {
            set_option_terminate_mc(0);
        } else {
            if (!parse_bool_value(parmv[2], &i)) {
                lydia_error(outfile, "Error parsing %s as a Boolean value.", parmv[2]);
                return;
            }
            set_option_terminate_mc(i);
        }
        lydia_ok(outfile, "cut");
        return;
    }

    lydia_error(outfile, "Unknown argument `%s'.", parmv[1]);
}

static void cmd_list(const void *UNUSED(context),
                     FILE *outfile,
                     const char **parmv,
                     const unsigned int parmc)
{
    if (0 == strcmp("models", parmv[1])) {
        if (parmc == 3) {
            lydia_error(outfile, "Unexpected argument `%s'.", parmv[2]);
        }
        lydia_start_output(outfile, parmv[0]);
        diagnostic_problems_list(outfile, pp_list_item);
        lydia_stop_output(outfile, parmv[0]);
        return;
    }
    if (0 == strcmp("observations", parmv[1])) {
        diagnostic_problem dp;

        if (parmc != 3) {
            lydia_error(outfile, "Missing model name.");
            return;
        }
        if (NULL == (dp = diagnostic_problem_get(parmv[2]))) {
            lydia_error(outfile, "Can not find a model `%s'.", parmv[2]);
            return;
        }

        lydia_start_output(outfile, parmv[0]);
        observations_list(outfile, dp, pp_list_item);
        lydia_stop_output(outfile, parmv[0]);
        return;
    }
    lydia_error(outfile, "Unknown argument `%s'.", parmv[1]);
}

static void cmd_quit(const void *UNUSED(input),
                     FILE *outfile,
                     const char **UNUSED(parmv),
                     const unsigned int UNUSED(parmc))
{
    lydia_ok(outfile, "quit");
    quit_program = 1;
}

static void cmd_help(const void *UNUSED(input),
                     FILE *outfile,
                     const char **UNUSED(parmv),
                     const unsigned int UNUSED(parmc))
{
    unsigned int ix = 0; 

    lydia_start_output(outfile, "help");
    while (std_commands[ix].cmd != NULL) {
        fprintf(outfile,
                "%-15s %s\n",
                std_commands[ix].cmd,
                std_commands[ix].help);
        ix++;
    }
    lydia_stop_output(outfile, "help");
}

static void cmd_version(const void *UNUSED(input),
                        FILE *outfile,
                        const char **UNUSED(parmv),
                        const unsigned int UNUSED(parmc))
{
    lydia_output(outfile,
                 "version",
                 "lydia v. %s, %s v. %d.%d",
                 PACKAGE_VERSION,
                 module_name,
                 version_major,
                 version_minor);
}

static void cmd_stat(const void *UNUSED(input),
                     FILE *outfile,
                     const char **parmv,
                     const unsigned int parmc)
{
    if (parmc == 2) {
        if (0 == strcmp(parmv[1], "reset")) {
            stat_reset();
            lydia_ok(outfile, parmv[0]);
        } else {
            lydia_error(outfile, "Unknown command option '%s'.", parmv[1]);
        }
        return;
    }
    lydia_start_output(outfile, parmv[0]);
    display_stat_items(outfile);
    lydia_stop_output(outfile, parmv[0]);
}

void cmd_fm(const void *context,
            FILE *outfile,
            const char **parmv,
            const unsigned int UNUSED(parmc))
{
    diagnostic_problem problem = (diagnostic_problem)context;

    lydia_start_output(outfile, parmv[0]);
    problem->fm(outfile, problem, pp_fm);
    lydia_stop_output(outfile, parmv[0]);
}

void cmd_probs(const void *context,
               FILE *outfile,
               const char **parmv,
               const unsigned int UNUSED(parmc))
{
    diagnostic_problem problem = (diagnostic_problem)context;

    lydia_start_output(outfile, parmv[0]);
    problem->probs(outfile, problem, pp_prob);
    lydia_stop_output(outfile, parmv[0]);
}

void cmd_cards(const void *context,
               FILE *outfile,
               const char **parmv,
               const unsigned int UNUSED(parmc))
{
    diagnostic_problem problem = (diagnostic_problem)context;

    lydia_start_output(outfile, parmv[0]);
    problem->cards(outfile, problem, pp_card);
    lydia_stop_output(outfile, parmv[0]);
}

void cmd_entropy(const void *context,
                 FILE *outfile,
                 const char **parmv,
                 const unsigned int UNUSED(parmc))
{
    diagnostic_problem problem = (diagnostic_problem)context;

    lydia_start_output(outfile, parmv[0]);
    problem->entropy(outfile, problem, pp_entropy);
    lydia_stop_output(outfile, parmv[0]);
}

void cmd_count(const void *context,
               FILE *outfile,
               const char **parmv,
               const unsigned int UNUSED(parmc))
{
    diagnostic_problem problem = (diagnostic_problem)context;

    lydia_start_output(outfile, parmv[0]);
    problem->count(outfile, problem, pp_count);
    lydia_stop_output(outfile, parmv[0]);
}

static command std_commands[MAX_COMMANDS] = {
    { "cut",            cmd_cut,              1,  2, "set termination criteria" },
    { "help",           cmd_help,             0,  0, "show help" },
    { "quit",           cmd_quit,             0,  0, "quit the program" },
    { "version",        cmd_version,          0,  0, "print the version of the Lydia software" },
    { NULL,             NULL,                -1, -1, NULL } /* End of the table. */
};

void enable_statistics()
{
    register unsigned int ix = 0, iy = 0;

    command commands[] = {
        { "stat",           cmd_stat,             0,  1, "display statistics" },
        { NULL,             NULL,                -1, -1, NULL }  /* End of the table. */
    };

    while (NULL != std_commands[iy].cmd) {
        iy += 1;
    }
    do {
        memcpy(&std_commands[iy], &commands[ix], sizeof(command));
        iy += 1;
        ix += 1;
    } while (commands[ix].cmd != NULL);
}

void enable_diagnosis()
{
    register unsigned int ix = 0, iy = 0;

    command commands[] = {
        { "fm",             cmd_fm,               0,  0, "list the fault modes of the system" },
        { "probs",          cmd_probs,            0,  0, "list the a posteriori probabilities" },
        { "cards",          cmd_cards,            0,  0, "list the cardinalities" },
        { "count",          cmd_count,            0,  0, "count the diagnoses" },
        { "entropy",        cmd_entropy,          0,  0, "calculate the diagnostic entropy" },
        { NULL,             NULL,                -1, -1, NULL }    /* End of the table. */
    };

    while (NULL != std_commands[iy].cmd) {
        iy += 1;
    }
    do {
        memcpy(&std_commands[iy], &commands[ix], sizeof(command));
        iy += 1;
        ix += 1;
    } while (commands[ix].cmd != NULL);
}

void enable_listings()
{
    register unsigned int ix = 0, iy = 0;

    command commands[] = {
        { "list",           cmd_list,             1,  2, "list data structures" },
        { NULL,             NULL,                -1, -1, NULL }    /* End of the table. */
    };

    while (NULL != std_commands[iy].cmd) {
        iy += 1;
    }
    do {
        memcpy(&std_commands[iy], &commands[ix], sizeof(command));
        iy += 1;
        ix += 1;
    } while (commands[ix].cmd != NULL);
}
