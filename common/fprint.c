#include <assert.h>
#include <string.h>

#include "config.h"
#include "fprint.h"
#include "strdup.h"
#include "array.h"
#include "defs.h"

static char buf[1024];

static void item_free(void *item)
{
    if (((print_item)item)->type == PRINT_ITEM_WORD) {
        free(((print_item)item)->u.word);
    }
    if (((print_item)item)->type == PRINT_ITEM_SYMBOL) {
/* Noop. */
    }
    if (((print_item)item)->type == PRINT_ITEM_LIST) {
        array_free(((print_item)item)->u.list);
    }
    free(item);
}

static print_item item_new_int(const int i)
{
    print_item item = (print_item)malloc(sizeof(struct str_print_item));
    if (NULL == item) {
        return item;
    }

    sprintf(buf, "%d", i);

    item->type = PRINT_ITEM_INT;
    item->u.ival = i;
    item->len = strlen(buf);

    return item;
}

static print_item item_new_unsigned(const int u)
{
    print_item item = (print_item)malloc(sizeof(struct str_print_item));
    if (NULL == item) {
        return item;
    }

    sprintf(buf, "%u", u);

    item->type = PRINT_ITEM_UNSIGNED;
    item->u.uval = u;
    item->len = strlen(buf);

    return item;
}

static print_item item_new_word(const char *ptr)
{
    print_item item = (print_item)malloc(sizeof(struct str_print_item));
    if (NULL == item) {
        return item;
    }

    item->type = PRINT_ITEM_WORD;
    item->u.word = strdup(ptr);
    item->len = strlen(ptr);

    return item;
}

static print_item item_new_symbol(lydia_symbol symbol)
{
    print_item item = (print_item)malloc(sizeof(struct str_print_item));
    if (NULL == item) {
        return item;
    }

    item->type = PRINT_ITEM_SYMBOL;
    item->u.symbol = symbol;
    if (lydia_symbolNIL == symbol) {
        item->len = 4;
    } else {
        item->len = strlen(symbol->name);
    }

    return item;
}

static print_item item_new_cons(const char *ptr)
{
    print_item item = (print_item)malloc(sizeof(struct str_print_item));
    if (NULL == item) {
        return item;
    }

    item->type = PRINT_ITEM_CONS;
    item->u.symbol = add_lydia_symbol(ptr);
    item->len = strlen(ptr) + 1;

    return item;
}

static print_item item_new_list()
{
    print_item item = (print_item)malloc(sizeof(struct str_print_item));
    if (NULL == item) {
        return item;
    }

    item->type = PRINT_ITEM_LIST;
    item->u.list = array_new(item_free, NULL);
    item->len = 0;

    return item;
}

static void fprint_item(print_state st, print_item item)
{
    register unsigned int iv = 0, ix, iy, iz;

    if (item->type == PRINT_ITEM_WORD) {
        fputs(item->u.word, st->file);
        st->pos += item->len;
    }
    if (item->type == PRINT_ITEM_CONS) {
        fputc(':', st->file);
        fputs(item->u.symbol->name, st->file);
        st->pos += item->len;
    }
    if (item->type == PRINT_ITEM_SYMBOL) {
        if (lydia_symbolNIL == item->u.symbol) {
            fputs("$nil", st->file);
        } else {
            fputs(item->u.symbol->name, st->file);
        }
        st->pos += item->len;
    }
    if (item->type == PRINT_ITEM_INT) {
        fprintf(st->file, "%d", item->u.ival);
        st->pos += item->len;
    }
    if (item->type == PRINT_ITEM_UNSIGNED) {
        fprintf(st->file, "%u", item->u.uval);
        st->pos += item->len;
    }
    if (item->type == PRINT_ITEM_LIST || item->type == PRINT_ITEM_EMPTY_LIST) {
        if (st->pos + item->len > st->wrap) {
            st->level += 1;
            iv = 1;
        }
        fputc('(', st->file);
        st->pos += 1;
        if (item->type == PRINT_ITEM_LIST) {
            for (ix = 0; ix < item->u.list->sz; ix++) {
                if (iv) {
                    fputc('\n', st->file);
                    st->pos = 0;
                    iy = st->level * st->iwidth;
                    for (iz = 0; iz < iy / st->tabsize; iz++) {
                        fputc('\t', st->file);
                    }
                    for (iz = 0; iz < iy % st->tabsize; iz++) {
                        fputc(' ', st->file);
                    }
                    st->pos += iy;
                } else {
                    if (0 != ix) {
                        fputc(' ', st->file);
                        st->pos += 1;
                    }
                }
                fprint_item(st, item->u.list->arr[ix]);
            }
        }
        if (iv) {
            st->level -= 1;

            fputc('\n', st->file);
            st->pos = 0;
            iy = st->level * st->iwidth;
            for (iz = 0; iz < iy / st->tabsize; iz++) {
                fputc('\t', st->file);
            }
            for (iz = 0; iz < iy % st->tabsize; iz++) {
                    fputc(' ', st->file);
            }
            st->pos += iy;
        }
        fputc(')', st->file);
        st->pos += 1;
    }
}

print_state set_print(FILE *file,
                      const int iwidth,
                      const int wrap,
                      const int tabsize)
{
    print_state st = (print_state)malloc(sizeof(struct str_print_state));
    if (NULL == st) {
        return st;
    }
    st->file = file;

    st->root = NULL;
    st->current = NULL;

    st->iwidth = iwidth;
    st->tabsize = tabsize;
    st->wrap = wrap;
    st->pos = 0;
    st->level = 0;

    return st;
}

void end_print(print_state st)
{
    if (NULL != st->root) {
        fprint_item(st, st->root);
        fputc('\n', st->file);
        item_free(st->root);
    }
    free(st);
}

static void add_word(print_state st, const char *word)
{
    print_item item = item_new_word(word);
    item->parent = st->current;

    if (st->current == NULL) {
        st->root = st->current = item;
        return;
    }
    array_append(st->current->u.list, item);
    st->current->len += item->len;
}

static void add_symbol(print_state st, lydia_symbol symbol)
{
    print_item item = item_new_symbol(symbol);
    item->parent = st->current;

    if (st->current == NULL) {
        st->root = st->current = item;
        return;
    }
    array_append(st->current->u.list, item);
    st->current->len += item->len;
}

static void add_int(print_state st, int i)
{
    print_item item = item_new_int(i);
    item->parent = st->current;

    if (st->current == NULL) {
        st->root = st->current = item;
        return;
    }
    array_append(st->current->u.list, item);
    st->current->len += item->len;
}

static void add_unsigned(print_state st, unsigned u)
{
    print_item item = item_new_unsigned(u);
    item->parent = st->current;

    if (st->current == NULL) {
        st->root = st->current = item;
        return;
    }
    array_append(st->current->u.list, item);
    st->current->len += item->len;
}

static void add_cons(print_state st, const char *word)
{
    print_item item = item_new_cons(word);
    item->parent = st->current;

    array_append(st->current->u.list, item);
    st->current->len += item->len;
}

void open_list(print_state st)
{
    print_item item = item_new_list();
    item->parent = st->current;

    if (st->current == NULL) {
        st->root = st->current = item;
        return;
    }

    array_append(st->current->u.list, item);

    st->current = item;
}

void close_list(print_state st)
{
    st->current->len += (2 + st->current->u.list->sz);
    if (st->current->u.list->sz > 0) {
        st->current->len -= 1;
    }
    if (0 == st->current->u.list->sz) {
        array_free(st->current->u.list);
        st->current->type = PRINT_ITEM_EMPTY_LIST;
    }
    if (NULL != st->current->parent) {
        st->current->parent->len += st->current->len;
    }
    st->current = st->current->parent;
}

void open_cons(print_state st, const char *name)
{
    open_list(st);
    add_cons(st, name);
}

void close_cons(print_state st)
{
    close_list(st);
}

void fprint_double(print_state st, double d)
{
    sprintf(buf, "%.16g", d);
    add_word(st, buf);
}

void fprint_float(print_state st, float f)
{
    fprint_double(st, (double)f);
}

void fprint_int(print_state st, int i)
{
    add_int(st, i);
}

void fprint_unsigned(print_state st, unsigned int u)
{
    add_unsigned(st, u);
}

void fprint_lydia_bool(print_state st, lydia_bool b)
{
    add_word(st, b ? LYDIA_TRUE_STR : LYDIA_FALSE_STR);
}

void fprint_lydia_symbol(print_state st, lydia_symbol s)
{
    add_symbol(st, s);
}
