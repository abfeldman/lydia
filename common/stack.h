#ifndef STACK
#define STACK

typedef void (* stack_element_destroy_func_t)(void *);
typedef void *(* stack_element_clone_func_t)(const void *);

typedef struct str_stack *stack;

struct str_stack
{
    unsigned int sz;
    unsigned int room;
    void **arr;
    stack_element_destroy_func_t destroy;
    stack_element_clone_func_t clone;
};

#define STACK_ROOM 8

extern stack stack_new(stack_element_destroy_func_t, stack_element_clone_func_t);
extern void stack_free(stack);
extern stack stack_copy(stack);
extern int stack_empty(stack);
extern void *stack_pop(stack);
extern int stack_push(stack, void *);
extern unsigned int stack_size(stack);
extern void *stack_top(stack);

#endif
