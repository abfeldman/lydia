#ifndef LYDIA_STAT_H
#define LYDIA_STAT_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern void init_stopwatch(const char *, const char *, const char *);
extern void start_stopwatch(const char *);
extern void stop_stopwatch(const char *);
extern void print_stopwatch(FILE *, const char *);

extern void init_int_counter(const char *, const char *, const char *);
extern void increase_int_counter(const char *);
extern void maximize_int_counter(const char *, int);
extern void add_to_int_counter(const char *, int);
extern void set_int_counter(const char *, int);

extern void display_stat_items(FILE *);

extern void stat_init();
extern void stat_destroy();
extern void stat_reset();

#ifdef __cplusplus
}
#endif

#endif
