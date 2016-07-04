/**
 * +----------------------------------------------------------------------+
 * | Zend Engine                                                          |
 * +----------------------------------------------------------------------+
 * | Copyright (c) 1998-2002 Zend Technologies Ltd. (http://www.zend.com) |
 * +----------------------------------------------------------------------+
 * | This source file is subject to version 2.00 of the Zend license,     |
 * | that is bundled with this package in the file LICENSE, and is        | 
 * | available at through the world-wide-web at                           |
 * | http://www.zend.com/license/2_00.txt.                                |
 * | If you did not receive a copy of the Zend license and are unable to  |
 * | obtain it through the world-wide-web, please send a note to          |
 * | license@zend.com so we can mail you a copy immediately.              |
 * +----------------------------------------------------------------------+
 * | Authors: Andi Gutmans <andi@zend.com>                                |
 * |          Zeev Suraski <zeev@zend.com>                                |
 * +----------------------------------------------------------------------+
 */

#ifndef HASH_H
#define HASH_H

#define LYDIA_DEBUG

#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif

#define HASH_KEY_IS_STRING      1
#define HASH_KEY_IS_LONG        2
#define HASH_KEY_NON_EXISTANT   3

#define HASH_UPDATE             (1 << 0)
#define HASH_ADD                (1 << 1)
#define HASH_NEXT_INSERT        (1 << 2)

#define HASH_DEL_KEY            0
#define HASH_DEL_INDEX          1

typedef unsigned long (* hash_func_t)(const char *, unsigned int);
typedef int  (* compare_func_t)(const void *, const void *);
typedef void (* sort_func_t)(void *, size_t, register size_t, compare_func_t);
typedef void (* dtor_func_t)(void *);
typedef void (* copy_ctor_func_t)(void *);

typedef struct str_bucket {
    unsigned long h;                        /* Used for numeric indexing */
    unsigned int key_length;
    void *data;
    void *data_ptr;
    struct str_bucket *list_next;
    struct str_bucket *list_last;
    struct str_bucket *next;
    struct str_bucket *last;
    char key[1]; /* Must be last element */
} bucket;

typedef struct str_hashtable {
    unsigned int table_size;
    unsigned int table_mask;
    unsigned int num_of_elements;
    unsigned long next_free_element;
    bucket *internal_pointer;   /* Used for element traversal */
    bucket *list_head;
    bucket *list_tail;
    bucket **buckets;
    hash_func_t hash_func;
    dtor_func_t destructor;
    unsigned char apply_count;
    int apply_protection;
#ifdef LYDIA_DEBUG
    int inconsistent;
#endif
} hash_table;

typedef bucket *hash_position;

/* startup/shutdown */
int hash_init(hash_table *, unsigned int, hash_func_t, dtor_func_t);
int hash_init_ex(hash_table *, unsigned int, hash_func_t, dtor_func_t, int);
void hash_destroy(hash_table *);
void hash_clean(hash_table *);

/* additions/updates/changes */
int hash_add_or_update(hash_table *, const char *, unsigned int, void *, unsigned int, void **, int);
#define hash_update(ht, key, key_length, data, data_size, dest) \
        hash_add_or_update(ht, key, key_length, data, data_size, dest, HASH_UPDATE)
#define hash_add(ht, key, key_length, data, data_size, dest) \
        hash_add_or_update(ht, key, key_length, data, data_size, dest, HASH_ADD)

int hash_quick_add_or_update(hash_table *, char *, unsigned int, unsigned long, void *, unsigned int, void **, int);
#define hash_quick_update(ht, key, key_length, h, data, data_size, dest) \
        hash_quick_add_or_update(ht, key, key_length, h, data, data_size, dest, HASH_UPDATE)
#define hash_quick_add(ht, key, key_length, h, data, data_size, dest) \
        hash_quick_add_or_update(ht, key, key_length, h, data, data_size, dest, HASH_ADD)

int hash_index_update_or_next_insert(hash_table *, unsigned long, void *, unsigned int, void **, int);
#define hash_index_update(ht, h, data, data_size, dest) \
        hash_index_update_or_next_insert(ht, h, data, data_size, dest, HASH_UPDATE)
#define hash_next_index_insert(ht, data, data_size, dest) \
        hash_index_update_or_next_insert(ht, 0, data, data_size, dest, HASH_NEXT_INSERT)

int hash_add_empty_element(hash_table *, char *, unsigned int);

#define LYDIA_HASH_APPLY_KEEP               0
#define LYDIA_HASH_APPLY_REMOVE             1 << 0
#define LYDIA_HASH_APPLY_STOP               1 << 1

typedef struct str_hash_key {
    char *key;
    unsigned int key_length;
    unsigned long h;
} hash_key;

typedef int (*apply_func_t)(void *);
typedef int (*apply_func_arg_t)(void *, void *);
typedef int (*apply_func_args_t)(void *, int, va_list, hash_key *);

void hash_graceful_destroy(hash_table *);
void hash_graceful_reverse_destroy(hash_table *);
void hash_apply(hash_table *ht, apply_func_t);
void hash_apply_with_argument(hash_table *, apply_func_arg_t, void *);
void hash_apply_with_arguments(hash_table *, apply_func_args_t, int, ...);

/*
 * This function should be used with special care (in other words,
 * it should usually not be used).  When used with the LYDIA_HASH_APPLY_STOP
 * return value, it assumes things about the order of the elements in the hash.
 * Also, it does not provide the same kind of reentrancy protection that
 * the standard apply functions do.
 */
void hash_reverse_apply(hash_table *, apply_func_t);

/* Deletes */
int hash_del_key_or_index(hash_table *, const char *, unsigned int, unsigned long, int);
#define hash_del(ht, key, key_length) hash_del_key_or_index(ht, key, key_length, 0, HASH_DEL_KEY)
#define hash_index_del(ht, h) hash_del_key_or_index(ht, NULL, 0, h, HASH_DEL_INDEX)

unsigned long lydia_get_hash_value(hash_table *, char *, unsigned int);

/* Data retreival: */
int hash_find(hash_table *, const char *, unsigned int, void **);
int hash_quick_find(hash_table *, char *, unsigned int, unsigned long, void **);
int hash_index_find(hash_table *, unsigned long, void **);

/* Misc.: */
int hash_exists(hash_table *, char *, unsigned int);
int hash_index_exists(hash_table *, unsigned long);
unsigned long hash_next_free_element(hash_table *);

/* Hash traversal: */
int hash_move_forward_ex(hash_table *, hash_position *);
int hash_move_backwards_ex(hash_table *, hash_position *);
int hash_get_current_key_ex(hash_table *, char **, unsigned int *, unsigned long *, int, hash_position *);
int hash_get_current_key_type_ex(hash_table *, hash_position *);
int hash_get_current_data_ex(hash_table *, void **, hash_position *);
void hash_internal_pointer_reset_ex(hash_table *, hash_position *);
void hash_internal_pointer_end_ex(hash_table *, hash_position *);

#define hash_move_forward(ht) hash_move_forward_ex(ht, NULL)
#define hash_move_backwards(ht) hash_move_backwards_ex(ht, NULL)
#define hash_get_current_key(ht, str_index, num_index, duplicate) hash_get_current_key_ex(ht, str_index, NULL, num_index, duplicate, NULL)
#define hash_get_current_key_type(ht) hash_get_current_key_type_ex(ht, NULL)
#define hash_get_current_data(ht, data) hash_get_current_data_ex(ht, data, NULL)
#define hash_internal_pointer_reset(ht) hash_internal_pointer_reset_ex(ht, NULL)
#define hash_internal_pointer_end(ht) hash_internal_pointer_end_ex(ht, NULL)

/* Copying, merging and sorting: */
void hash_copy(hash_table *, hash_table *, copy_ctor_func_t, void *, unsigned int);
void hash_merge(hash_table *, hash_table *, copy_ctor_func_t, void *, unsigned int, int);
void hash_merge_ex(hash_table *, hash_table *, copy_ctor_func_t, unsigned int, int (*)(void *, void *));
int hash_sort(hash_table *, sort_func_t, compare_func_t, int);
int hash_compare(hash_table *, hash_table *, compare_func_t, int);
int hash_minmax(hash_table *, compare_func_t, int, void **);

int hash_num_elements(hash_table *);

int hash_rehash(hash_table *);

#ifdef LYDIA_DEBUG
void hash_display_list_tail(hash_table *);
void hash_display(FILE *, hash_table *);
#endif

#endif
