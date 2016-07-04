#include "stat.h"
#include "defs.h"
#include "strdup.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifndef WIN32
# include <strings.h>
# include <sys/time.h>
#else
# include <windows.h>
#endif

/* Internal constants: */
#define TYPE_STOPWATCH    0
#define TYPE_COUNTER      1

/* Internal data-types: */
typedef struct str_stat_item *stat_item;
typedef struct str_counter *counter;
typedef struct str_stopwatch *stopwatch;

struct str_counter
{
    int i_val;
};

struct str_stopwatch
{
#ifndef WIN32
    struct timeval start_time;
    struct timeval stop_time;
    struct timeval elapsed_time;
#else
    LARGE_INTEGER start_time;
    LARGE_INTEGER stop_time;
    LARGE_INTEGER elapsed_time;
#endif
};

struct str_stat_item
{
    int type;

    char *name;
    char *mask;
    char *group;

    union
    {
        struct str_counter cnt;
        struct str_stopwatch sw;
    } u;
};

/* Internal function declarations: */
static void destroy_stat_item(const char *name);

/* Internal variables: */
static stat_item *stat_items = NULL;
static unsigned int stat_items_cnt = 0;
static unsigned int stat_items_room = 0;

#ifdef WIN32
LARGE_INTEGER frequency;
int has_frequency;
#endif

static stat_item find_stat_item(const char *name)
{
    register unsigned int ix;
    for (ix = 0; ix < stat_items_cnt; ix++) {
        if (0 == strcmp(stat_items[ix]->name, name)) {
            return stat_items[ix];
        }
    }
    return NULL;
}

static counter find_counter(const char *name)
{
    stat_item item = find_stat_item(name);
    if (NULL == item) {
        return NULL;
    }
    if (item->type == TYPE_COUNTER) {
        return &item->u.cnt;
    }
    return NULL;
}

static stopwatch find_stopwatch(const char *name)
{
    stat_item item = find_stat_item(name);
    if (NULL == item) {
        return NULL;
    }
    if (item->type == TYPE_STOPWATCH) {
        return &item->u.sw;
    }
    return NULL;
}

void stat_init()
{
    if (NULL != stat_items) {
        return;
    }
    stat_items = (stat_item *)malloc(sizeof(stat_item) * 16);
    if (NULL == stat_items) {
        assert(0);
        abort();
    }
    stat_items_room = 16;

#ifdef WIN32
    has_frequency = QueryPerformanceFrequency(&frequency);
#endif
}

void stat_destroy()
{
    register unsigned int ix;
    for (ix = stat_items_cnt - 1; ix < stat_items_cnt; ix--) {
        destroy_stat_item(stat_items[ix]->name);
    }
    free(stat_items);
    stat_items = NULL;
    stat_items_cnt = 0;
    stat_items_room = 0;
}

void stat_reset()
{
    register unsigned int ix;

    for (ix = 0; ix < stat_items_cnt; ix++) {
        stat_item item = stat_items[ix];
        switch (item->type) 
            {
            case TYPE_COUNTER:
                item->u.cnt.i_val = 0;
                break;
            case TYPE_STOPWATCH:
#ifndef WIN32
                item->u.sw.elapsed_time.tv_sec = 0;
                item->u.sw.elapsed_time.tv_usec = 0;
#else
                item->u.sw.elapsed_time.QuadPart = 0LL;
#endif
                break;
            default:
                assert(0);
                abort();
        }
    }
}

static stat_item init_stat_item(const char *name, const char *mask, const char *group)
{
    stat_item item = find_stat_item(name);
    if (NULL != item) {
        return item;
    }

    item = (stat_item)malloc(sizeof(struct str_stat_item));
    memset(item, 0, sizeof(struct str_stat_item));
    item->name = strdup(name);
    item->mask = strdup(mask);
    item->group = strdup(group);
    stat_items[stat_items_cnt] = item;
    stat_items_cnt += 1;
    if (stat_items_cnt >= stat_items_room) {
        stat_items_room += 16;
        if (NULL == (stat_items = (stat_item *)realloc(stat_items, sizeof(stat_item) * stat_items_room))) {
            assert(0);
            abort();
        }
    }
    return item;
}

void init_int_counter(const char *name, const char *mask, const char *group)
{
    stat_item item = init_stat_item(name, mask, group);
    item->type = TYPE_COUNTER;
}

void increase_int_counter(const char *name)
{
    counter cnt = find_counter(name);
    if (NULL != cnt) {
        cnt->i_val += 1;
    }
}

void maximize_int_counter(const char *name, int value)
{
    counter cnt = find_counter(name);
    if (NULL != cnt && value > cnt->i_val) {
        cnt->i_val = value;
    }
}

void add_to_int_counter(const char *name, int value)
{
    counter cnt = find_counter(name);
    if (NULL != cnt) {
        cnt->i_val += value;
    }
}

void set_int_counter(const char *name, int value)
{
    counter cnt = find_counter(name);
    if (NULL != cnt) {
        cnt->i_val = value;
    }
}

static void destroy_counter(const counter UNUSED(cnt))
{
    /* Nothing. */
}

static void destroy_stopwatch(const stopwatch UNUSED(cnt))
{
    /* Nothing. */
}

static void destroy_stat_item(const char *name)
{
    stat_item item = find_stat_item(name);
    if (NULL == item) {
        return;
    }
    free(item->name);
    free(item->group);
    free(item->mask);
    switch (item->type) {
        case TYPE_STOPWATCH:
            destroy_stopwatch(&item->u.sw);
            break;
        case TYPE_COUNTER:
            destroy_counter(&item->u.cnt);
            break;
        default:
            assert(0);
            abort();
    }
    free(item);
}

void display_stat_items(FILE *outfile)
{
    register unsigned int ix;

    for (ix = 0; ix < stat_items_cnt; ix++) {
        stat_item item = stat_items[ix];
        if (ix == 0 || 0 != strcmp(item->group, stat_items[ix - 1]->group)) {
            fprintf(outfile, "%s:\n", item->group);
        }
        fprintf(outfile, "  ");
        switch (item->type) {
            case TYPE_COUNTER:
                fprintf(outfile, item->mask, item->u.cnt.i_val);
                break;
            case TYPE_STOPWATCH:
#ifndef WIN32
                fprintf(outfile,
                        item->mask,
                        item->u.sw.elapsed_time.tv_sec,
                        item->u.sw.elapsed_time.tv_usec / 1000,
                        item->u.sw.elapsed_time.tv_usec % 1000);
#else
                if (!has_frequency || frequency.QuadPart == 0LL) {
                    fprintf(outfile, item->mask, 0, 0, 0);
                    break;
                }
                fprintf(outfile,
                        item->mask,
                        (long)(item->u.sw.elapsed_time.QuadPart / frequency.QuadPart),
                        (long)(((item->u.sw.elapsed_time.QuadPart * 1000000LL) / frequency.QuadPart) / 1000LL % 1000LL),
                        (long)(((item->u.sw.elapsed_time.QuadPart * 1000000LL) / frequency.QuadPart) % 1000LL));
#endif
                break;
            default:
                assert(0);
                abort();
        }
        fprintf(outfile, "\n");
    }
}

void init_stopwatch(const char *name, const char *mask, const char *group)
{
    stat_item item = init_stat_item(name, mask, group);
    item->type = TYPE_STOPWATCH;
}

void print_stopwatch(FILE *outfile, const char *name)
{
#ifndef WIN32
    struct timeval t;
    struct timeval stop_time;
    struct timeval elapsed_time;
#else
    LARGE_INTEGER stop_time;
    LARGE_INTEGER elapsed_time;
#endif

    stopwatch sw = find_stopwatch(name);
    if (NULL != sw) {
#ifndef WIN32
        gettimeofday(&stop_time, NULL);
        t.tv_sec = stop_time.tv_sec - sw->start_time.tv_sec;
        t.tv_usec = stop_time.tv_usec - sw->start_time.tv_usec;
        if (t.tv_usec < 0) {
            t.tv_sec -= 1;
            t.tv_usec += 1000000;
        }

        elapsed_time.tv_sec = t.tv_sec;
        elapsed_time.tv_usec = t.tv_usec;
        if (elapsed_time.tv_usec > 1000000) {
            elapsed_time.tv_sec += 1;
            elapsed_time.tv_usec -= 1000000;
        }

        fprintf(outfile,
                "%ld s %03ld.%03ld ms",
                elapsed_time.tv_sec,
                elapsed_time.tv_usec / 1000,
                elapsed_time.tv_usec % 1000);
#else
        QueryPerformanceCounter(&stop_time);
        elapsed_time.QuadPart = stop_time.QuadPart - sw->start_time.QuadPart;

        if (!has_frequency || frequency.QuadPart == 0LL) {
            fprintf(outfile, item->mask, 0, 0, 0);
        } else {
            fprintf(outfile,
                    "%d s %03d.%03d ms",
                    (long)(elapsed_time.QuadPart / frequency.QuadPart),
                    (long)(((elapsed_time.QuadPart * 1000000LL) / frequency.QuadPart) / 1000LL % 1000LL),
                    (long)(((elapsed_time.QuadPart * 1000000LL) / frequency.QuadPart) % 1000LL));
        }
#endif
    }
}

void start_stopwatch(const char *name)
{
    stopwatch sw = find_stopwatch(name);
    if (NULL != sw) {
#ifndef WIN32
        gettimeofday(&sw->start_time, NULL);
#else
        QueryPerformanceCounter(&sw->start_time);
#endif
    }
}

void stop_stopwatch(const char *name)
{
#ifndef WIN32
    struct timeval t;
#endif

    stopwatch sw = find_stopwatch(name);
    if (NULL != sw) {
#ifndef WIN32
        gettimeofday(&sw->stop_time, NULL);
        t.tv_sec = sw->stop_time.tv_sec - sw->start_time.tv_sec;
        t.tv_usec = sw->stop_time.tv_usec - sw->start_time.tv_usec;
        if (t.tv_usec < 0) {
            t.tv_sec -= 1;
            t.tv_usec += 1000000;
        }

        sw->elapsed_time.tv_sec += t.tv_sec;
        sw->elapsed_time.tv_usec += t.tv_usec;
        if (sw->elapsed_time.tv_usec > 1000000) {
            sw->elapsed_time.tv_sec += 1;
            sw->elapsed_time.tv_usec -= 1000000;
        }
#else
        QueryPerformanceCounter(&sw->stop_time);
        sw->elapsed_time.QuadPart = sw->stop_time.QuadPart - sw->start_time.QuadPart;
#endif
    }
}
