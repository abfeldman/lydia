/**
 *  \file stack.c
 *  \brief Implementation of stacks.
 */

#include "stack.h"

#include <stdlib.h>
#include <string.h>

/**
 * Allocate a new stack.
 *
 * @returns a stack handler, NULL if memory allocation error.
 */
stack stack_new(stack_element_destroy_func_t destroy,
                stack_element_clone_func_t clone)
{
    stack sh = (stack)malloc(sizeof(struct str_stack));
    if (NULL == sh) {
        return NULL;
    }

    sh->sz = 0;
    sh->room = STACK_ROOM;
    sh->arr = (void **)malloc(STACK_ROOM * sizeof(void *));
    sh->destroy = destroy;
    sh->clone = clone;
    if (NULL == sh->arr) {
        free(sh);
        return NULL;
    }

    return sh;
}

/**
 * Free a stack.
 *
 * @param sh a stack handler.
 */
void stack_free(stack sh)
{
    register unsigned int ix;

    if (NULL == sh) {
        return;
    }

    if (NULL != sh->destroy) {
        for (ix = 0; ix < sh->sz; ix++) {
            sh->destroy(sh->arr[ix]);
        }
    }
    free(sh->arr);
    free(sh);
}

/**
 * Check if a stack is empty.
 *
 * @param sh a stack handler;
 * @returns 1 if the stack is empty, 0 otherwise.
 */
int stack_empty(stack sh)
{
    return sh->sz == 0;
}

/**
 * Pops the top element of a stack.
 *
 * @param sh a stack handler;
 * @returns the top element.
 */
void *stack_pop(stack sh)
{
    void *el = NULL;

    if (sh->sz > 0) {
        el = sh->arr[sh->sz - 1];
        sh->sz -= 1;
    }
    return el;
}

/**
 * Pushes an element to the top of a stack.
 *
 * @param sh a stack handler;
 * @param el a pointer to the element;
 * @returns 0 if memory allocation error, 1 otherwise.
 */
int stack_push(stack sh, void *el)
{
    if (sh->room < sh->sz + 1) {
        void **arr = (void **)realloc(sh->arr, (sh->room + STACK_ROOM) * sizeof(void *));
        if (NULL == arr) {
            return 0;
        }
        sh->arr = arr;
        sh->room += STACK_ROOM;
    }

    sh->arr[sh->sz] = el;
    sh->sz += 1;

    return 1;
}

/**
 * Get the size of a stack.
 *
 * @param sh a stack handler;
 * @returns the number of elements in the stack.
 */
unsigned stack_size(stack sh)
{
    return sh->sz;
}

/**
 * Peek the top of a stack.
 *
 * @param sh a stack handler;
 * @returns the element with the highest priority.
 */
void *stack_top(stack sh)
{
    return sh->sz > 0 ? sh->arr[sh->sz - 1] : NULL;
}

stack stack_copy(stack sh)
{
    register unsigned int ix;

    stack result;

    if (NULL == sh) {
        return NULL;
    }

    result = stack_new(sh->destroy, sh->clone);
    if (NULL != sh->clone) {
        for (ix = 0; ix < sh->sz; ix++) {
            stack_push(result, sh->clone(sh->arr[ix]));
        }
    } else {
        for (ix = 0; ix < sh->sz; ix++) {
            stack_push(result, sh->arr[ix]);
        }
    }

    return result;
}
