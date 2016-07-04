#ifndef ARRAY
#define ARRAY

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef void (* array_element_destroy_func_t)(void *);
typedef void *(* array_element_clone_func_t)(const void *);

typedef struct str_array *array;

struct str_array
{
    size_t sz;
    size_t room;
    void **arr;
    array_element_destroy_func_t destroy;
    array_element_clone_func_t clone;
};

#define ARRAY_ROOM 8

#define INT_TO_POINTER(i) ((void*) (intptr_t) (i))
#define POINTER_TO_INT(p) ((int)  (intptr_t) (p))

extern array array_new(array_element_destroy_func_t, array_element_clone_func_t);
extern void array_free(array);
extern array array_copy(array);
extern void array_clear(array);
extern int array_empty(array);
extern size_t array_size(array);
extern array array_setroom(array, const size_t);
extern array array_append(array, void *);
extern array array_reverse(array);
extern array array_insert(array, const unsigned int, void *);
extern array array_delete(array, const unsigned int);
extern array array_concat(array, array);
extern array array_int_sort(array);
extern int array_sorted_int_member(array, int);
extern int array_index_of(array, void *);
extern array array_sorted_insert(array, void *);
extern void pp_int_array(FILE *, array);
extern int array_contains(array, void *);
extern int array_intersects(array, array);

#endif
