#ifndef QUEUE
#define QUEUE

typedef void (* queue_element_destroy_func_t)(void *);
typedef void *(* queue_element_clone_func_t)(const void *);

typedef struct str_queue *queue;

struct str_queue
{
    unsigned int sz;
    unsigned int room;
    void **arr;
    queue_element_destroy_func_t destroy;
    queue_element_clone_func_t clone;
};

#define QUEUE_ROOM 8

extern queue queue_new(queue_element_destroy_func_t, queue_element_clone_func_t);
extern void queue_free(queue);
extern void queue_clear(queue);
extern int queue_empty(queue);
extern void *queue_pop(queue);
extern int queue_push(queue, void *);
extern unsigned int queue_size(queue);
extern void *queue_front(queue);
extern void *queue_back(queue);

#endif
