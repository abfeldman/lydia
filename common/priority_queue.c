/**
 *  \file priority_queue.c
 *  \brief Implementation of priority queues.
 */

#include "priority_queue.h"

#include <stdlib.h>
#include <string.h>

/**
 * Allocate a new priority queue.
 *
 * @param cmp a pointer to an ordering function;
 * @returns a queue handler, NULL if memory allocation error.
 */
priority_queue priority_queue_new(priority_queue_cmp_func_t cmp,
				  priority_queue_element_destroy_func_t destroy,
				  void *cmp_context)
{
    priority_queue qh = (priority_queue)malloc(sizeof(struct str_priority_queue));
    if (NULL == qh) {
	return NULL;
    }

    qh->sz = 0;
    qh->room = PRIORITY_QUEUE_ROOM;
    qh->cmp = cmp;
    qh->destroy = destroy;
    qh->cmp_context = cmp_context;
    qh->arr = (void **)malloc(PRIORITY_QUEUE_ROOM * sizeof(void *));
    if (NULL == qh->arr) {
	free(qh);
	return NULL;
    }

    return qh;
}

/**
 * Free a priority queue.
 *
 * @param qh a queue handler.
 */
void priority_queue_free(priority_queue qh)
{
    register unsigned int ix;

    if (NULL != qh->destroy) {
	for (ix = 0; ix < qh->sz; ix++) {
	    qh->destroy(qh->arr[ix]);
	}
    }
    free(qh->arr);
    free(qh);
}

/**
 * Check if a priority queue is empty.
 *
 * @param qh a queue handler;
 * @returns 1 if the queue is empty, 0 otherwise.
 */
int priority_queue_empty(priority_queue qh)
{
    return qh->sz == 0;
}

static void heapify(priority_queue qh, unsigned int i)
{
    unsigned int left = 2 * i + 1;
    unsigned int right = 2 * i + 2;

    unsigned int largest = i;

    if (left < qh->sz && qh->cmp(qh->cmp_context, qh->arr[left], qh->arr[largest]) > 0) {
	largest = left;
    }
    
    if (right < qh->sz && qh->cmp(qh->cmp_context, qh->arr[right], qh->arr[largest]) > 0) {
	largest = right;
    }

    if (largest != i) {
	void *tmp = qh->arr[i];
	qh->arr[i] = qh->arr[largest];
	qh->arr[largest] = tmp;
	heapify(qh, largest);
    }
}


/**
 * Discard the first element in a priority queue.
 *
 * @param qh a queue handler;
 * @returns the element with the highest priority.
 */
void *priority_queue_pop(priority_queue qh)
{
    void *el = NULL;

    if (qh->sz > 0) {
	el = qh->arr[0];
	qh->sz -= 1;
	qh->arr[0] = qh->arr[qh->sz];
	heapify(qh, 0);
    }
    return el;
}

/**
 * Adds an element to a priority queue.
 *
 * @param qh a queue handler;
 * @param el a pointer to the element;
 * @returns 0 if memory allocation error, 1 otherwise.
 */
int priority_queue_push(priority_queue qh, void *el)
{
    int i = qh->sz; /* point to last element */

    if (qh->room < qh->sz + 1) {
	void **arr = (void **)realloc(qh->arr, (qh->room + PRIORITY_QUEUE_ROOM) * sizeof(void *));
	if (NULL == arr) {
	    return 0;
	}
	qh->arr = arr;
	qh->room += PRIORITY_QUEUE_ROOM;
    }

    qh->sz += 1;

    while(i && qh->cmp(qh->cmp_context, qh->arr[(i - 1) / 2], el) < 0) {
	qh->arr[i] = qh->arr[(i - 1) / 2];
	i = (i - 1) / 2;
    }
    qh->arr[i] = el;

    return 1;
}

/**
 * Get the size of a priority queue.
 *
 * @param qh a queue handler;
 * @returns the number of elements in the queue.
 */
unsigned priority_queue_size(priority_queue qh)
{
    return qh->sz;
}

/**
 * Peek the first element in a priority queue.
 *
 * @param qh a queue handler;
 * @returns the element with the highest priority.
 */
void *priority_queue_top(priority_queue qh)
{
    return qh->sz > 0 ? qh->arr[0] : NULL;
}

/*
 * Local variables:
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
