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

#define _GNU_SOURCE
#define LYDIA_DEBUG

#include "config.h"
#include "hash.h"
#include "defs.h"
#include "strndup.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#define HANDLE_NUMERIC(key, length, func) {                                                 \
        register const char *tmp = key;                                                         \
                                                                                            \
    if ((*tmp >= '0' && *tmp <= '9')) do { /* possibly a numeric index */                   \
        const char *end = tmp + length - 1;                                                     \
        unsigned long idx;                                                                  \
                                                                                            \
        if (*tmp++ == '0' && length > 2) { /* don't accept numbers with leading zeros */    \
            break;                                                                          \
        }                                                                                   \
        while (tmp < end) {                                                                 \
            if (!(*tmp >= '0' && *tmp <= '9')) {                                            \
                break;                                                                      \
            }                                                                               \
            tmp++;                                                                          \
        }                                                                                   \
        if (tmp == end && *tmp == '\0') { /* a numeric index */                             \
            idx = strtol(key, NULL, 10);                                                    \
            if (idx != LONG_MAX) {                                                          \
                return func;                                                                \
            }                                                                               \
        }                                                                                   \
    } while (0);                                                                            \
}

#define CONNECT_TO_BUCKET_DLLIST(element, list_head)        \
    (element)->next = (list_head);                          \
    (element)->last = NULL;                                 \
    if ((element)->next) {                                  \
        (element)->next->last = (element);                  \
    }

#define CONNECT_TO_GLOBAL_DLLIST(element, ht)               \
    (element)->list_last = (ht)->list_tail;                 \
    (ht)->list_tail = (element);                            \
    (element)->list_next = NULL;                            \
    if ((element)->list_last != NULL) {                     \
        (element)->list_last->list_next = (element);        \
    }                                                       \
    if (!(ht)->list_head) {                                 \
        (ht)->list_head = (element);                        \
    }                                                       \
    if ((ht)->internal_pointer == NULL) {                   \
        (ht)->internal_pointer = (element);                 \
    }

#ifdef LYDIA_DEBUG
#define HT_OK               0
#define HT_IS_DESTROYING    1
#define HT_DESTROYED        2
#define HT_CLEANING         3

static void _lydia_is_inconsistent(hash_table *ht, char *file, int line)
{
    if (ht->inconsistent==HT_OK) {
        return;
    }
    switch (ht->inconsistent) {
        case HT_IS_DESTROYING:
            fprintf(stderr, "%s(%d) : ht=0x%p is being destroyed", file, line, (void *)ht);
            break;
        case HT_DESTROYED:
            fprintf(stderr, "%s(%d) : ht=0x%p is already destroyed", file, line, (void *)ht);
            break;
        case HT_CLEANING:
            fprintf(stderr, "%s(%d) : ht=0x%p is being cleaned", file, line, (void *)ht);
            break;
    }
    assert(0);
    abort();
}
#define IS_CONSISTENT(a) _lydia_is_inconsistent(a, __FILE__, __LINE__);
#define SET_INCONSISTENT(n) ht->inconsistent = n;
#else
#define IS_CONSISTENT(a)
#define SET_INCONSISTENT(n)
#endif

#define HASH_PROTECT_RECURSION(ht)                      \
    if ((ht)->apply_protection) {                       \
        if ((ht)->apply_count++ >= 3) {                 \
            assert(0);                                  \
            abort();                                    \
        }                                               \
    }


#define HASH_UNPROTECT_RECURSION(ht)                    \
    if ((ht)->apply_protection) {                       \
        (ht)->apply_count--;                            \
    }

#define LYDIA_HASH_IF_FULL_DO_RESIZE(ht)                \
    if ((ht)->num_of_elements > (ht)->table_size) {     \
        hash_do_resize(ht);                             \
    }

static int hash_do_resize(hash_table *ht);

#define UPDATE_DATA(ht, p, data, data_size)             \
    if (data_size == sizeof(void *)) {                  \
        if (!(p)->data_ptr) {                           \
            free((p)->data);                            \
        }                                               \
        memcpy(&(p)->data_ptr, data, sizeof(void *));   \
        (p)->data = &(p)->data_ptr;                     \
    } else {                                            \
        if ((p)->data_ptr) {                            \
            (p)->data = (void *)malloc(data_size);      \
            (p)->data_ptr = NULL;                       \
        }                                               \
        memcpy((p)->data, data, data_size);             \
    }

#define INIT_DATA(ht, p, data, data_size);              \
    if (data_size == sizeof(void *)) {                  \
        memcpy(&(p)->data_ptr, data, sizeof(void *));   \
        (p)->data = &(p)->data_ptr;                     \
    } else {                                            \
        (p)->data = (void *)malloc(data_size);          \
        if (!(p)->data) {                               \
            free(p);                                    \
            return -1;                                  \
        }                                               \
        memcpy((p)->data, data, data_size);             \
        (p)->data_ptr = NULL;                           \
    }

static unsigned long default_hash_func(const char *key, unsigned int key_length)
{
    unsigned long h = 5381;
    const char *end = key + key_length;

    while (key < end) {
        h += (h << 5);
        h ^= (unsigned long)*key++;
    }
    return h;
}

int hash_init(hash_table *ht, unsigned int sz, hash_func_t hash_function, dtor_func_t destructor)
{
    unsigned int i = 3;

    SET_INCONSISTENT(HT_OK);

    while ((1U << i) < sz) {
        i++;
    }

    ht->table_size = 1 << i;
    ht->table_mask = ht->table_size - 1;
    
/* Uses "calloc" to zero initialize the data. */
    ht->buckets = (bucket **)calloc(ht->table_size, sizeof(bucket *));
    
    if (!ht->buckets) {
        return -1;
    }

    ht->hash_func = (hash_function == NULL ? default_hash_func : hash_function);
    ht->destructor = destructor;
    ht->list_head = NULL;
    ht->list_tail = NULL;
    ht->num_of_elements = 0;
    ht->next_free_element = 0;
    ht->internal_pointer = NULL;
    ht->apply_count = 0;
    ht->apply_protection = 1;

    return 0;
}

int hash_init_ex(hash_table *ht, unsigned int sz, hash_func_t hash_function, dtor_func_t destructor, int apply_protection)
{
    int retval = hash_init(ht, sz, hash_function, destructor);

    ht->apply_protection = apply_protection;
    return retval;
}

void hash_set_apply_protection(hash_table *ht, int apply_protection)
{
    ht->apply_protection = apply_protection;
}

int hash_add_or_update(hash_table *ht, const char *key, unsigned int key_length, void *data, unsigned int data_size, void **dest, int flag)
{
    unsigned long h;
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    assert(key_length > 0);

    HANDLE_NUMERIC(key, key_length, hash_index_update_or_next_insert(ht, idx, data, data_size, dest, flag));
    
    h = ht->hash_func(key, key_length);
    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && (p->key_length == key_length)) {
            if (!memcmp(p->key, key, key_length)) {
                if (flag & HASH_ADD) {
                    return -1;
                }
                assert(p->data != data);
                if (ht->destructor) {
                    ht->destructor(p->data);
                }
                UPDATE_DATA(ht, p, data, data_size);
                if (dest) {
                    *dest = p->data;
                }
                return 0;
            }
        }
        p = p->next;
    }
    
    p = (bucket *)malloc(sizeof(bucket) - 1 + key_length);
    if (!p) {
        return -1;
    }
    memcpy(p->key, key, key_length);
    p->key_length = key_length;
    INIT_DATA(ht, p, data, data_size);
    p->h = h;
    CONNECT_TO_BUCKET_DLLIST(p, ht->buckets[index]);
    if (dest) {
        *dest = p->data;
    }

    CONNECT_TO_GLOBAL_DLLIST(p, ht);
    ht->buckets[index] = p;

    ht->num_of_elements++;
    LYDIA_HASH_IF_FULL_DO_RESIZE(ht);       /* If the Hash table is full, resize it */

    return 0;
}

int hash_quick_add_or_update(hash_table *ht, char *key, unsigned int key_length, unsigned long h, void *data, unsigned int data_size, void **dest, int flag)
{
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    assert(key_length > 0);

    index = h & ht->table_mask;
    
    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && (p->key_length == key_length)) {
            if (!memcmp(p->key, key, key_length)) {
                if (flag & HASH_ADD) {
                    return -1;
                }
                assert(p->data == data);
                if (ht->destructor) {
                    ht->destructor(p->data);
                }
                UPDATE_DATA(ht, p, data, data_size);
                if (dest) {
                    *dest = p->data;
                }
                return 0;
            }
        }
        p = p->next;
    }
    
    p = (bucket *)malloc(sizeof(bucket) - 1 + key_length);
    if (!p) {
        return -1;
    }

    memcpy(p->key, key, key_length);
    p->key_length = key_length;
    INIT_DATA(ht, p, data, data_size);
    p->h = h;
    
    CONNECT_TO_BUCKET_DLLIST(p, ht->buckets[index]);

    if (dest) {
        *dest = p->data;
    }

    ht->buckets[index] = p;
    CONNECT_TO_GLOBAL_DLLIST(p, ht);

    ht->num_of_elements++;
    LYDIA_HASH_IF_FULL_DO_RESIZE(ht);       /* If the Hash table is full, resize it */
    return 0;
}


int hash_add_empty_element(hash_table *ht, char *key, unsigned int key_length)
{
    void *dummy = (void *) 1;

    return hash_add(ht, key, key_length, &dummy, sizeof(void *), NULL);
}


int hash_index_update_or_next_insert(hash_table *ht, unsigned long h, void *data, unsigned int data_size, void **dest, int flag)
{
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    if (flag & HASH_NEXT_INSERT) {
        h = ht->next_free_element;
    }
    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->key_length == 0) && (p->h == h)) {
            if (flag & HASH_NEXT_INSERT || flag & HASH_ADD) {
                return -1;
            }
            assert(p->data != data);
            if (ht->destructor) {
                ht->destructor(p->data);
            }
            UPDATE_DATA(ht, p, data, data_size);
            if (h >= ht->next_free_element) {
                ht->next_free_element = h + 1;
            }
            if (dest) {
                *dest = p->data;
            }
            return 0;
        }
        p = p->next;
    }
    p = (bucket *)malloc(sizeof(bucket) - 1);
    if (!p) {
        return -1;
    }
    p->key_length = 0;          /*  Numeric indices are marked by making the key_length == 0 */
    p->h = h;
    INIT_DATA(ht, p, data, data_size);
    if (dest) {
        *dest = p->data;
    }

    CONNECT_TO_BUCKET_DLLIST(p, ht->buckets[index]);

    ht->buckets[index] = p;
    CONNECT_TO_GLOBAL_DLLIST(p, ht);

    if (h >= ht->next_free_element) {
        ht->next_free_element = h + 1;
    }
    ht->num_of_elements++;
    LYDIA_HASH_IF_FULL_DO_RESIZE(ht);
    return 0;
}


static int hash_do_resize(hash_table *ht)
{
    bucket **t;

    IS_CONSISTENT(ht);

    if ((ht->table_size << 1) > 0) {    /* Let's double the table size */
        t = (bucket **)realloc(ht->buckets, (ht->table_size << 1) * sizeof(bucket *));
        if (t) {
            ht->buckets = t;
            ht->table_size = (ht->table_size << 1);
            ht->table_mask = ht->table_size - 1;
            hash_rehash(ht);
            return 0;
        }
        return -1;
    }
    return 0;
}

int hash_rehash(hash_table *ht)
{
    bucket *p;
    unsigned int index;

    IS_CONSISTENT(ht);

    memset(ht->buckets, 0, ht->table_size * sizeof(bucket *));
    p = ht->list_head;
    while (p != NULL) {
        index = p->h & ht->table_mask;
        CONNECT_TO_BUCKET_DLLIST(p, ht->buckets[index]);
        ht->buckets[index] = p;
        p = p->list_next;
    }
    return 0;
}

int hash_del_key_or_index(hash_table *ht, const char *key, unsigned int key_length, unsigned long h, int flag)
{
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    if (flag == HASH_DEL_KEY) {
        HANDLE_NUMERIC(key, key_length, hash_del_key_or_index(ht, key, key_length, idx, HASH_DEL_INDEX));
        h = ht->hash_func(key, key_length);
    }
    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && ((p->key_length == 0) || /* Numeric index */
            ((p->key_length == key_length) && (!memcmp(p->key, key, key_length))))) {
            if (p == ht->buckets[index]) {
                ht->buckets[index] = p->next;
            } else {
                p->last->next = p->next;
            }
            if (p->next) {
                p->next->last = p->last;
            }
            if (p->list_last != NULL) {
                p->list_last->list_next = p->list_next;
            } else { 
                /* Deleting the head of the list */
                ht->list_head = p->list_next;
            }
            if (p->list_next != NULL) {
                p->list_next->list_last = p->list_last;
            } else {
                ht->list_tail = p->list_last;
            }
            if (ht->internal_pointer == p) {
                ht->internal_pointer = p->list_next;
            }
            if (ht->destructor) {
                ht->destructor(p->data);
            }
            if (!p->data_ptr) {
                free(p->data);
            }
            free(p);
            ht->num_of_elements--;
            return 0;
        }
        p = p->next;
    }
    return -1;
}

void hash_destroy(hash_table *ht)
{
    bucket *p, *q;

    IS_CONSISTENT(ht);

    SET_INCONSISTENT(HT_IS_DESTROYING);

    p = ht->list_head;
    while (p != NULL) {
        q = p;
        p = p->list_next;
        if (ht->destructor) {
            ht->destructor(q->data);
        }
        if (!q->data_ptr && q->data) {
/**
 * To Do: The whole concept is messy. If we add an integer with a
 * value 0, then this will cause an invalid free. For now comment out
 * and do not use data pointers in data but only integers. Find out
 * how to fix and send an e-mail to Andy.
 */
/*          free(q->data); */
        }
        free(q);
    }
    free(ht->buckets);

    SET_INCONSISTENT(HT_DESTROYED);
}

void hash_clean(hash_table *ht)
{
    bucket *p, *q;

    IS_CONSISTENT(ht);

    SET_INCONSISTENT(HT_CLEANING);

    p = ht->list_head;
    while (p != NULL) {
        q = p;
        p = p->list_next;
        if (ht->destructor) {
            ht->destructor(q->data);
        }
        if (!q->data_ptr && q->data) {
            free(q->data);
        }
        free(q);
    }
    memset(ht->buckets, 0, ht->table_size*sizeof(bucket *));
    ht->list_head = NULL;
    ht->list_tail = NULL;
    ht->num_of_elements = 0;
    ht->next_free_element = 0;
    ht->internal_pointer = NULL;

    SET_INCONSISTENT(HT_OK);
}

/*
 * This function is used by the various apply() functions.
 * It deletes the passed bucket, and returns the address of the
 * next bucket.  The hash *may* be altered during that time, the
 * returned value will still be valid.
 */
static bucket *hash_apply_deleter(hash_table *ht, bucket *p)
{
    bucket *retval;

    if (ht->destructor) {
        ht->destructor(p->data);
    }
    if (!p->data_ptr) {
        free(p->data);
    }
    retval = p->list_next;

    if (p->last) {
        p->last->next = p->next;
    } else {
        unsigned int index;

        index = p->h & ht->table_mask;
        ht->buckets[index] = p->next;
    }
    if (p->next) {
        p->next->last = p->last;
    } else {
        /* Nothing to do as this list doesn't have a tail */
    }

    if (p->list_last != NULL) {
        p->list_last->list_next = p->list_next;
    } else { 
        /* Deleting the head of the list */
        ht->list_head = p->list_next;
    }
    if (p->list_next != NULL) {
        p->list_next->list_last = p->list_last;
    } else {
        ht->list_tail = p->list_last;
    }
    if (ht->internal_pointer == p) {
        ht->internal_pointer = p->list_next;
    }
    free(p);
    ht->num_of_elements--;

    return retval;
}

void hash_graceful_destroy(hash_table *ht)
{
    bucket *p;

    IS_CONSISTENT(ht);

    p = ht->list_head;
    while (p != NULL) {
        p = hash_apply_deleter(ht, p);
    }
    free(ht->buckets);

    SET_INCONSISTENT(HT_DESTROYED);
}

void hash_graceful_reverse_destroy(hash_table *ht)
{
    bucket *p;

    IS_CONSISTENT(ht);

    p = ht->list_tail;
    while (p != NULL) {
        hash_apply_deleter(ht, p);
        p = ht->list_tail;
    }

    free(ht->buckets);

    SET_INCONSISTENT(HT_DESTROYED);
}

/*
 * This is used to selectively delete certain entries from a hashtable.
 * destruct() receives the data and decides if the entry should be deleted 
 * or not
 */

void hash_apply(hash_table *ht, apply_func_t apply_func)
{
    bucket *p;

    IS_CONSISTENT(ht);

    HASH_PROTECT_RECURSION(ht);
    p = ht->list_head;
    while (p != NULL) {
        if (apply_func(p->data)) {
            p = hash_apply_deleter(ht, p);
        } else {
            p = p->list_next;
        }
    }
    HASH_UNPROTECT_RECURSION(ht);
}

void hash_apply_with_argument(hash_table *ht, apply_func_arg_t apply_func, void *argument)
{
    bucket *p;

    IS_CONSISTENT(ht);

    HASH_PROTECT_RECURSION(ht);
    p = ht->list_head;
    while (p != NULL) {
        if (apply_func(p->data, argument)) {
            p = hash_apply_deleter(ht, p);
        } else {
            p = p->list_next;
        }
    }
    HASH_UNPROTECT_RECURSION(ht);
}

void hash_apply_with_arguments(hash_table *ht, apply_func_args_t destruct, int num_args, ...)
{
    bucket *p;
    va_list args;
    hash_key hash_key;

    IS_CONSISTENT(ht);

    HASH_PROTECT_RECURSION(ht);

    va_start(args, num_args);
    p = ht->list_head;
    while (p != NULL) {
        hash_key.key = p->key;
        hash_key.key_length = p->key_length;
        hash_key.h = p->h;
        if (destruct(p->data, num_args, args, &hash_key)) {
            p = hash_apply_deleter(ht, p);
        } else {
            p = p->list_next;
        }
    }
    va_end(args);

    HASH_UNPROTECT_RECURSION(ht);
}

void hash_reverse_apply(hash_table *ht, apply_func_t apply_func)
{
    bucket *p, *q;

    IS_CONSISTENT(ht);

    HASH_PROTECT_RECURSION(ht);
    p = ht->list_tail;
    while (p != NULL) {
        int result = apply_func(p->data);

        q = p;
        p = p->list_last;
        if (result & LYDIA_HASH_APPLY_REMOVE) {
            if (q->key_length>0) {
                hash_del(ht, q->key, q->key_length);
            } else {
                hash_index_del(ht, q->h);
            }
        }
        if (result & LYDIA_HASH_APPLY_STOP) {
            break;
        }
    }
    HASH_UNPROTECT_RECURSION(ht);
}

void hash_copy(hash_table *target, hash_table *source, copy_ctor_func_t pCopyConstructor, void *UNUSED(tmp), unsigned int size)
{
    bucket *p;
    void *new_entry;

    IS_CONSISTENT(source);
    IS_CONSISTENT(target);

    p = source->list_head;
    while (p) {
        if (p->key_length) {
            hash_update(target, p->key, p->key_length, p->data, size, &new_entry);
        } else {
            hash_index_update(target, p->h, p->data, size, &new_entry);
        }
        if (pCopyConstructor) {
            pCopyConstructor(new_entry);
        }
        p = p->list_next;
    }
    target->internal_pointer = target->list_head;
}

void hash_merge(hash_table *target, hash_table *source, copy_ctor_func_t pCopyConstructor, void *UNUSED(tmp), unsigned int size, int overwrite)
{
    bucket *p;
    void *t;
    int mode = (overwrite ? HASH_UPDATE : HASH_ADD);

    IS_CONSISTENT(source);
    IS_CONSISTENT(target);

    p = source->list_head;
    while (p) {
        if (p->key_length>0) {
            if (hash_add_or_update(target, p->key, p->key_length, p->data, size, &t, mode) == 0 && pCopyConstructor) {
                pCopyConstructor(t);
            }
        } else {
            if ((mode == HASH_UPDATE || !hash_index_exists(target, p->h)) && hash_index_update(target, p->h, p->data, size, &t) == 0 && pCopyConstructor) {
                pCopyConstructor(t);
            }
        }
        p = p->list_next;
    }
    target->internal_pointer = target->list_head;
}

void hash_merge_ex(hash_table *target, hash_table *source, copy_ctor_func_t pCopyConstructor, unsigned int size, int (*pReplaceOrig)(void *orig, void *p_new))
{
    bucket *p;
    void *t;
    void *pOrig;

    IS_CONSISTENT(source);
    IS_CONSISTENT(target);

    p = source->list_head;
    while (p) {
        if (p->key_length>0) {
            if (hash_find(target, p->key, p->key_length, &pOrig) == -1
                || pReplaceOrig(pOrig, p->data)) {
                if (hash_update(target, p->key, p->key_length, p->data, size, &t) == 0 && pCopyConstructor) {
                    pCopyConstructor(t);
                }
            }
        } else {
            if (hash_index_find(target, p->h, &pOrig) == -1
                || pReplaceOrig(pOrig, p->data)) {
                if (hash_index_update(target, p->h, p->data, size, &t) == 0 && pCopyConstructor) {
                    pCopyConstructor(t);
                }
            }
        }
        p = p->list_next;
    }
    target->internal_pointer = target->list_head;
}

#ifdef LYDIA_DEBUG
unsigned long lydia_get_hash_value(hash_table *ht, char *key, unsigned int key_length)
#else
unsigned long lydia_get_hash_value(hash_table *UNUSED(ht), char *key, unsigned int key_length)
#endif
{
    IS_CONSISTENT(ht);

    return ht->hash_func(key, key_length);
}


/*
 * Returns 0 if found and -1 if not. The pointer to the
 * data is returned in data. The reason is that there's no reason
 * someone using the hash table might not want to have NULL data
 */
int hash_find(hash_table *ht, const char *key, unsigned int key_length, void **data)
{
    unsigned long h;
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    HANDLE_NUMERIC(key, key_length, hash_index_find(ht, idx, data));

    h = ht->hash_func(key, key_length);
    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && (p->key_length == key_length)) {
            if (!memcmp(p->key, key, key_length)) {
                *data = p->data;
                return 0;
            }
        }
        p = p->next;
    }
    return -1;
}

int hash_quick_find(hash_table *ht, char *key, unsigned int key_length, unsigned long h, void **data)
{
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && (p->key_length == key_length)) {
            if (!memcmp(p->key, key, key_length)) {
                *data = p->data;
                return 0;
            }
        }
        p = p->next;
    }
    return -1;
}

int hash_exists(hash_table *ht, char *key, unsigned int key_length)
{
    unsigned long h;
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    HANDLE_NUMERIC(key, key_length, hash_index_exists(ht, idx));

    h = ht->hash_func(key, key_length);
    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && (p->key_length == key_length)) {
            if (!memcmp(p->key, key, key_length)) {
                return 1;
            }
        }
        p = p->next;
    }
    return 0;
}

int hash_index_find(hash_table *ht, unsigned long h, void **data)
{
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && (p->key_length == 0)) {
            *data = p->data;
            return 0;
        }
        p = p->next;
    }
    return -1;
}

int hash_index_exists(hash_table *ht, unsigned long h)
{
    unsigned int index;
    bucket *p;

    IS_CONSISTENT(ht);

    index = h & ht->table_mask;

    p = ht->buckets[index];
    while (p != NULL) {
        if ((p->h == h) && (p->key_length == 0)) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

int hash_num_elements(hash_table *ht)
{
    IS_CONSISTENT(ht);

    return ht->num_of_elements;
}

void hash_internal_pointer_reset_ex(hash_table *ht, hash_position *pos)
{
    IS_CONSISTENT(ht);

    if (pos) {
        *pos = ht->list_head;
    } else {
        ht->internal_pointer = ht->list_head;
    }
}

/*
 * This function will be extremely optimized by remembering 
 * the end of the list
 */
void hash_internal_pointer_end_ex(hash_table *ht, hash_position *pos)
{
    IS_CONSISTENT(ht);

    if (pos) {
        *pos = ht->list_tail;
    } else {
        ht->internal_pointer = ht->list_tail;
    }
}

int hash_move_forward_ex(hash_table *ht, hash_position *pos)
{
    hash_position *current = pos ? pos : &ht->internal_pointer;

    IS_CONSISTENT(ht);

    if (*current) {
        *current = (*current)->list_next;
        return 0;
    } else {
        return -1;
    }
}

int hash_move_backwards_ex(hash_table *ht, hash_position *pos)
{
    hash_position *current = pos ? pos : &ht->internal_pointer;

    IS_CONSISTENT(ht);

    if (*current) {
        *current = (*current)->list_last;
        return 0;
    } else {
        return -1;
    }
}

/* This function should be made binary safe  */
int hash_get_current_key_ex(hash_table *ht, char **str_index, unsigned int *str_length, unsigned long *num_index, int duplicate, hash_position *pos)
{
    bucket *p;
   
    p = pos ? (*pos) : ht->internal_pointer;

    IS_CONSISTENT(ht);

    if (p) {
        if (p->key_length) {
            if (duplicate) {
                *str_index = (char *)strndup(p->key, p->key_length);
            } else {
                *str_index = p->key;
            }
            if (str_length) {
                *str_length = p->key_length;
            }
            return HASH_KEY_IS_STRING;
        } else {
            *num_index = p->h;
            return HASH_KEY_IS_LONG;
        }
    }
    return HASH_KEY_NON_EXISTANT;
}

int hash_get_current_key_type_ex(hash_table *ht, hash_position *pos)
{
    bucket *p;
   
    p = pos ? (*pos) : ht->internal_pointer;

    IS_CONSISTENT(ht);

    if (p) {
        if (p->key_length) {
            return HASH_KEY_IS_STRING;
        } else {
            return HASH_KEY_IS_LONG;
        }
    }
    return HASH_KEY_NON_EXISTANT;
}

int hash_get_current_data_ex(hash_table *ht, void **data, hash_position *pos)
{
    bucket *p;
   
    p = pos ? (*pos) : ht->internal_pointer;

    IS_CONSISTENT(ht);

    if (p) {
        *data = p->data;
        return 0;
    } else {
        return -1;
    }
}

int hash_sort(hash_table *ht, sort_func_t sort_func, compare_func_t compar, int renumber)
{
    bucket **tmp;
    bucket *p;
    int i, j;

    IS_CONSISTENT(ht);

    if (ht->num_of_elements <= 1) { /* Doesn't require sorting */
        return 0;
    }
    tmp = (bucket **)malloc(ht->num_of_elements * sizeof(bucket *));
    if (!tmp) {
        return -1;
    }
    p = ht->list_head;
    i = 0;
    while (p) {
        tmp[i] = p;
        p = p->list_next;
        i++;
    }

    (*sort_func)((void *) tmp, i, sizeof(bucket *), compar);

    ht->list_head = tmp[0];
    ht->list_tail = NULL;
    ht->internal_pointer = ht->list_head;

    for (j = 0; j < i; j++) {
        if (ht->list_tail) {
            ht->list_tail->list_next = tmp[j];
        }
        tmp[j]->list_last = ht->list_tail;
        tmp[j]->list_next = NULL;
        ht->list_tail = tmp[j];
    }
    free(tmp);

    if (renumber) {
        p = ht->list_head;
        i = 0;
        while (p != NULL) {
            p->key_length = 0;
            p->h = i++;
            p = p->list_next;
        }
        ht->next_free_element = i;
        hash_rehash(ht);
    }
    return 0;
}

int hash_compare(hash_table *ht1, hash_table *ht2, compare_func_t compar, int ordered)
{
    bucket *p1, *p2 = NULL;
    int result;
    void *data2;

    IS_CONSISTENT(ht1);
    IS_CONSISTENT(ht2);

    HASH_PROTECT_RECURSION(ht1); 
    HASH_PROTECT_RECURSION(ht2); 

    result = ht1->num_of_elements - ht2->num_of_elements;
    if (result!=0) {
        HASH_UNPROTECT_RECURSION(ht1); 
        HASH_UNPROTECT_RECURSION(ht2); 
        return result;
    }

    p1 = ht1->list_head;
    if (ordered) {
        p2 = ht2->list_head;
    }

    while (p1) {
        if (ordered && !p2) {
            HASH_UNPROTECT_RECURSION(ht1); 
            HASH_UNPROTECT_RECURSION(ht2); 
            return 1; /* That's not supposed to happen */
        }
        if (ordered) {
            if (p1->key_length==0 && p2->key_length==0) { /* numeric indices */
                result = p1->h - p2->h;
                if (result!=0) {
                    HASH_UNPROTECT_RECURSION(ht1); 
                    HASH_UNPROTECT_RECURSION(ht2); 
                    return result;
                }
            } else { /* string indices */
                result = p1->key_length - p2->key_length;
                if (result!=0) {
                    HASH_UNPROTECT_RECURSION(ht1); 
                    HASH_UNPROTECT_RECURSION(ht2); 
                    return result;
                }
                result = memcmp(p1->key, p2->key, p1->key_length);
                if (result!=0) {
                    HASH_UNPROTECT_RECURSION(ht1); 
                    HASH_UNPROTECT_RECURSION(ht2); 
                    return result;
                }
            }
            data2 = p2->data;
        } else {
            if (p1->key_length==0) { /* numeric index */
                if (hash_index_find(ht2, p1->h, &data2) == -1) {
                    HASH_UNPROTECT_RECURSION(ht1); 
                    HASH_UNPROTECT_RECURSION(ht2); 
                    return 1;
                }
            } else { /* string index */
                if (hash_find(ht2, p1->key, p1->key_length, &data2) == -1) {
                    HASH_UNPROTECT_RECURSION(ht1); 
                    HASH_UNPROTECT_RECURSION(ht2); 
                    return 1;
                }
            }
        }
        result = compar(p1->data, data2);
        if (result!=0) {
            HASH_UNPROTECT_RECURSION(ht1); 
            HASH_UNPROTECT_RECURSION(ht2); 
            return result;
        }
        p1 = p1->list_next;
        if (ordered) {
            p2 = p2->list_next;
        }
    }
    
    HASH_UNPROTECT_RECURSION(ht1); 
    HASH_UNPROTECT_RECURSION(ht2); 
    return 0;
}

int hash_minmax(hash_table *ht, compare_func_t compar, int flag, void **data)
{
    bucket *p, *res;

    IS_CONSISTENT(ht);

    if (ht->num_of_elements == 0 ) {
        *data = NULL;
        return -1;
    }

    res = p = ht->list_head;
    while ((p = p->list_next)) {
        if (flag) {
            if (compar(&res, &p) < 0) { /* max */
                res = p;
            }
        } else {
            if (compar(&res, &p) > 0) { /* min */
                res = p;
            }
        }
    }
    *data = res->data;
    return 0;
}

unsigned long hash_next_free_element(hash_table *ht)
{
    IS_CONSISTENT(ht);

    return ht->next_free_element;

}

#ifdef LYDIA_DEBUG
void hash_display_list_tail(hash_table *ht)
{
    bucket *p;

    p = ht->list_tail;
    while (p != NULL) {
        fprintf(stderr, "list_tail has key %s\n", p->key);
        p = p->list_last;
    }
}

void hash_display(FILE *outfile, hash_table *ht)
{
    bucket *p;
    register unsigned int i;

    for (i = 0; i < ht->table_size; i++) {
        p = ht->buckets[i];
        while (p != NULL) {
            fprintf(outfile, "%s: 0x%lX\n", p->key, p->h);
            p = p->next;
        }
    }

    p = ht->list_tail;
    while (p != NULL) {
        fprintf(outfile, "%s: 0x%lX\n", p->key, p->h);
        p = p->list_last;
    }
}
#endif
