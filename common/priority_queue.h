#ifndef __PRIORITY_QUEUE__
#define __PRIORITY_QUEUE__

typedef void (* priority_queue_element_destroy_func_t)(void *);
typedef int (* priority_queue_cmp_func_t)(void *, void *, void *);

typedef struct str_priority_queue *priority_queue;

struct str_priority_queue
{
    priority_queue_cmp_func_t cmp;
    priority_queue_element_destroy_func_t destroy;
    void *cmp_context;
    unsigned int sz;
    unsigned int room;
    void **arr;
};

#define PRIORITY_QUEUE_ROOM 16

extern priority_queue priority_queue_new(priority_queue_cmp_func_t, priority_queue_element_destroy_func_t, void *);
extern void priority_queue_free(priority_queue);
extern int priority_queue_empty(priority_queue);
extern void *priority_queue_pop(priority_queue);
extern int priority_queue_push(priority_queue, void *);
extern unsigned int priority_queue_size(priority_queue);
extern void *priority_queue_top(priority_queue);

#endif

/*
 * Local variables:
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
