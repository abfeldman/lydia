#ifndef LREADLINE_H
#define LREADLINE_H

#include "config.h"

#ifdef HAVE_READLINE_READLINE_H
# include <readline/readline.h>
#else
# ifdef HAVE_LIBREADLINE
extern char *readline(const char *prompt);
# endif
#endif

#ifdef HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#else
# ifdef HAVE_LIBREADLINE
extern void add_history(const char *line);
# endif
#endif

extern char *lreadline(const char *prompt);

#endif
