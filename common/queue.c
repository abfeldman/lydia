/**
 *  \file queue.c
 *  \brief Implementation of queues.
 */

#include "queue.h"

#include <stdlib.h>
#include <string.h>

/**
 * Allocate a new queue.
 *
 * @returns a queue handler, NULL if memory allocation error.
 */
queue queue_new(queue_element_destroy_func_t destroy,
                queue_element_clone_func_t clone)
{
    queue qh = (queue)malloc(sizeof(struct str_queue));
    if (NULL == qh) {
        return NULL;
    }

    qh->sz = 0;
    qh->room = QUEUE_ROOM;
    qh->arr = (void **)malloc(QUEUE_ROOM * sizeof(void *));
    qh->destroy = destroy;
    qh->clone = clone;
    if (NULL == qh->arr) {
        free(qh);
        return NULL;
    }

    return qh;
}

/**
 * Free a queue.
 *
 * @param qh a queue handler.
 */
void queue_free(queue qh)
{
    register unsigned int ix;

    if (NULL == qh) {
        return;
    }

    if (NULL != qh->destroy) {
        for (ix = 0; ix < qh->sz; ix++) {
            qh->destroy(qh->arr[ix]);
        }
    }
    free(qh->arr);
    free(qh);
}

/**
 * Check if a queue is empty.
 *
 * @param qh a queue handler;
 * @returns 1 if the queue is empty, 0 otherwise.
 */
int queue_empty(queue qh)
{
    return qh->sz == 0;
}

void queue_clear(queue qh)
{
    qh->sz = 0;
}

/**
 * Discard the first element in a queue.
 *
 * @param qh a queue handler;
 * @returns the element with the highest priority.
 */
void *queue_pop(queue qh)
{
    void *el = NULL;

    if (qh->sz > 0) {
        el = qh->arr[0];
        qh->sz -= 1;
        memmove(&qh->arr[0], &qh->arr[1], sizeof(void *) * qh->sz);
        if (qh->room - qh->sz > QUEUE_ROOM) {
            qh->room -= QUEUE_ROOM;
            qh->arr = (void **)realloc(qh->arr, qh->room * sizeof(void *)); /* Shouldn't fail here. */
        }
    }
    return el;
}

/**
 * Adds an element to a queue.
 *
 * @param qh a queue handler;
 * @param el a pointer to the element;
 * @returns 0 if memory allocation error, 1 otherwise.
 */
int queue_push(queue qh, void *el)
{
    if (qh->room < qh->sz + 1) {
        void **arr = (void **)realloc(qh->arr, (qh->room + QUEUE_ROOM) * sizeof(void *));
        if (NULL == arr) {
            return 0;
        }
        qh->arr = arr;
        qh->room += QUEUE_ROOM;
    }

    qh->arr[qh->sz] = el;
    qh->sz += 1;

    return 1;
}

/**
 * Get the size of a queue.
 *
 * @param qh a queue handler;
 * @returns the number of elements in the queue.
 */
unsigned queue_size(queue qh)
{
    return qh->sz;
}

/**
 * Peek the last element in a queue.
 *
 * @param qh a queue handler;
 * @returns the element with the highest priority.
 */
void *queue_back(queue qh)
{
    return qh->sz > 0 ? qh->arr[qh->sz - 1] : NULL;
}

/**
 * Peek the first element in a queue.
 *
 * @param qh a queue handler;
 * @returns the element with the highest priority.
 */
void *queue_front(queue qh)
{
    return qh->sz > 0 ? qh->arr[0] : NULL;
}
