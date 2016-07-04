#include "trie_io.h"
#include "fprint.h"

static void trie_print_internal(print_state st, trie_node node)
{
    register unsigned int ix;

    if (NULL == node->kids || 0 == node->kids->sz) {
        return;
    }
    open_list(st);
    for (ix = 0; ix < node->kids->sz; ix++) {
    	if(node->is_deleted)
    		fprint_int(st,-1);
        fprint_int(st, (int)node->edges->arr[ix]);
        trie_print_internal(st, node->kids->arr[ix]);
    }
    close_list(st);
}

void trie_print(print_state st, trie t)
{
    open_list(st);
    trie_print_internal(st, t->root);
    close_list(st);
}
