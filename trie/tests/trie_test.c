#include "trie_io.h"
#include "graphml.h"
#include "config.h"
#include "array.h"
#include "graph.h"
#include "trie.h"
#include "defs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned int edge_id = 0;
static unsigned int node_id = 1;
/*
static int keys[][10] =
{
    {   1,  2,  3,  4,  5,  0 },
    {   1,  2,  6,  7,  8,  9,  0 },
    {   1,  2,  6, 10, 11, 12, 13, 14,  0 },
    {   1,  2,  6, 10, 15, 16, 17, 18,  0 },
    {   1,  2,  6, 19, 20, 21, 22, 23,  0 },
    {   1,  2,  6, 19, 24, 25, 26, 27, 28,  0 },
    {  -1 },
};
*/
/*
static int keys1[][10] =
{
    {  1, 2, 88, 0 },
    {  5, 0 },
    {  5, 7, 0 },
    {  5, 9, 0 },
    {  8, 10, 11, 0 },
    {  8, 15, 0 },
    {  8, 25, 27, 0 },
    { -1 }
};

static int keys2[][10] =
{
    { 5, 9, 12, 0 },
    { -1 }
};
*/
/*
static int keys1[][10] =
{
    {  5,  0 },
    {  8, 10, 0 },
    {  8, 15, 0 },
    {  5,  9, 12, 17, 21, 22, 0 },
    { -1 }
};

static int keys2[][10] =
{
    {  9, 12 },
    { -1 }
};
*/
/*
static int keys2[][10] =
{
    {  1,  2 },
    {  1,  9 },
    {  1, 10 },
    {  1, 11 },
    {  62, 68, 70 },
    { -1 }
};

static int keys1[][10] =
{
    {  1,  2,  3 },
    {  1,  2,  3,  4,  5 },
    {  4 },
    {  4, 5 },
    {  4, 8 },
    { 62, 71 },
    { -1 }
};
*/

static int keys1[][10] =
{
    {  1, 2, 3, 4, 5, 0 },
    {  1, 2, 7, 8, 9, 0 },
    { -1 }
};

static int keys2[][10] =
{
/*
    {  60, 72, 74, 76, 78, 80 },
    {  60, 82, 84, 86, 88 },
*/
    { 1, 2, 3, 4, 5, 0 },
    { -1 }
};

array make_key(int keys[][10], int key_idx)
{
    register unsigned int ix;

    array result = array_new(NULL, NULL);

    for (ix = 0; keys[key_idx][ix] != 0; ix++) {
        array_append(result, (void *)keys[key_idx][ix]);
    }

    return result;
}

trie make_trie(int keys[][10])
{
    register unsigned int ix;

    trie result = trie_new(NULL, NULL);

    ix = 0;

    while (keys[ix][0] != -1) {
        array key = make_key(keys, ix);
        trie_add(result, key, NULL);
        array_free(key);

        ix += 1;
    }

    return result;
}

static void trie_to_graph_internal(g_graph g,
                                   trie_node node,
                                   unsigned int trie_node_id,
                                   unsigned int parent_id)
{
    g_node n;
    g_edge e;

    register unsigned int ix;

    unsigned int id;

    n = graph_node_new(node_id);

    n->label = (char *)malloc(256 * sizeof(char));
    sprintf(n->label, "%d", trie_node_id);
    n->label = realloc(n->label, strlen(n->label) + 1);
    if (node->is_terminal) {
        n->color = 0x00FF00;
    }
    if (node->is_deleted == 1) {
        n->color = 0xFF0000;
    }
    if (node->is_deleted == 2) {
        n->color = 0xFFFF00;
    }
    graph_node_add(g, n);

    if (parent_id != 0) {
        e = graph_edge_new(edge_id++, parent_id, node_id);
        graph_edge_add(g, e);
    }

    id = node_id;
    node_id += 1;

    if (NULL == node->kids || 0 == node->kids->sz) {
        return;
    }
    for (ix = 0; ix < node->kids->sz; ix++) {
        trie_to_graph_internal(g,
                               node->kids->arr[ix],
                               (unsigned int)node->edges->arr[ix],
                               id);
    }
}

void trie_to_graph(g_graph g, trie t)
{
    trie_to_graph_internal(g, t->root, 0, 0);
}

/** Simple debug utility **/
array build_int_array(int from, int to){
	array new_array = array_new(NULL,NULL);
	int i;
	for(i=from;i<=to;i++)
		array_append(new_array, INT_TO_POINTER(i));
	return new_array;
}

/**
 * Set of tests for trie_is_subsumed_fixed
 */
int test_trie_is_subsumed_fixed()
{
	trie t = trie_new((trie_node_destroy_func_t)array_free,
            (trie_node_clone_func_t)array_copy);
	array array123 = build_int_array(1,3);
	array array1234 = build_int_array(1,4);
	array array1234567 = build_int_array(1,7);
	array array123a = build_int_array(1,3);
	array array23 = build_int_array(2,3);
	array array23456 = build_int_array(2,6);
	array array1 = build_int_array(1,1);
	array array7 = build_int_array(7,7);
	array array789 = build_int_array(7,9);

	fprintf(stdout,"test_trie()\n");

	fprintf(stdout,"-- Add ");
	pp_int_array(stdout, array123);
	fprintf(stdout," -- \n");
	trie_remove_subsumed(t, array123);
	trie_add(t,array123,array123);

	assert(trie_is_subsumed_fixed(t,array123));
	assert(trie_is_subsumed_fixed(t,array123a));
	assert(trie_is_subsumed_fixed(t,array1234));
	assert(trie_is_subsumed_fixed(t,array1234567));
	assert(trie_is_subsumed_fixed(t,array1)==0);
	assert(trie_is_subsumed_fixed(t,array23)==0);
	assert(trie_is_subsumed_fixed(t,array23456)==0);
	assert(trie_is_subsumed_fixed(t,array7)==0);
	assert(trie_is_subsumed_fixed(t,array789)==0);

	fprintf(stdout,"-- Add ");
	pp_int_array(stdout, array23);
	fprintf(stdout," -- \n");
	trie_remove_subsumed(t, array23);
	trie_add(t,array23,array23);

	assert(trie_is_subsumed_fixed(t,array123));
	assert(trie_is_subsumed_fixed(t,array123a));
	assert(trie_is_subsumed_fixed(t,array1234));
	assert(trie_is_subsumed_fixed(t,array1234567));
	assert(trie_is_subsumed_fixed(t,array23));
	assert(trie_is_subsumed_fixed(t,array23456));
	assert(trie_is_subsumed_fixed(t,array1)==0);
	assert(trie_is_subsumed_fixed(t,array7)==0);
	assert(trie_is_subsumed_fixed(t,array789)==0);


	fprintf(stdout,"-- Add ");
	pp_int_array(stdout, array1);
	fprintf(stdout," -- \n");
	trie_remove_subsumed(t, array1);
	trie_add(t,array1,array1);

	assert(trie_is_subsumed_fixed(t,array123));
	assert(trie_is_subsumed_fixed(t,array123a));
	assert(trie_is_subsumed_fixed(t,array1234));
	assert(trie_is_subsumed_fixed(t,array1234567));
	assert(trie_is_subsumed_fixed(t,array23));
	assert(trie_is_subsumed_fixed(t,array23456));
	assert(trie_is_subsumed_fixed(t,array1));
	assert(trie_is_subsumed_fixed(t,array7)==0);
	assert(trie_is_subsumed_fixed(t,array789)==0);

	trie_free(t);

	fprintf(stdout,"passed test_trie() !\n");

	return 1;
}

/**
 * Example of bug in old trie_is_subsumed() implementation, and test the new implementation.
 */
int test_trie_is_subsumed_bug()
{
	trie t = trie_new((trie_node_destroy_func_t)array_free,
            (trie_node_clone_func_t)array_copy);
	array array123 = build_int_array(1,3);
	array array23 = build_int_array(2,3);
	array array1 = build_int_array(1,1);

	fprintf(stdout,"test_trie()\n");

	fprintf(stdout,"-- Add ");
	pp_int_array(stdout, array123);
	fprintf(stdout," -- \n");
	trie_remove_subsumed(t, array123);
	trie_add(t,array123,array123);

	fprintf(stdout,"-- Add ");
	pp_int_array(stdout, array23);
	fprintf(stdout," -- \n");
	trie_remove_subsumed(t, array23);
	trie_add(t,array23,array23);

	fprintf(stdout,"-- Add ");
	pp_int_array(stdout, array1);
	fprintf(stdout," -- \n");
	trie_remove_subsumed(t, array1);
	trie_add(t,array1,array1);

	if(trie_is_subsumed(t,array1)){
		pp_int_array(stdout, array1);
		fprintf(stdout," is subsumed\n");
	}
	else{
		pp_int_array(stdout, array1);
		fprintf(stdout, " not subsumed\n");
	}

	if(trie_is_subsumed_fixed(t,array1)){
		pp_int_array(stdout, array1);
		fprintf(stdout," is subsumed fixed \n");
	}
	else{
		pp_int_array(stdout, array1);
		fprintf(stdout, " not subsumed fixed \n");
	}

	assert(trie_is_subsumed_fixed(t,array1));

	trie_free(t);

	fprintf(stdout,"passed test_trie() !\n");

	return 1;
}

void test_trie_element_subsumed_by_key()
{
	trie t = trie_new((trie_node_destroy_func_t)array_free,
	            (trie_node_clone_func_t)array_copy);
	array array123 = build_int_array(1,3);
	array array23 = build_int_array(2,3);
	array array1 = build_int_array(1,1);

	fprintf(stdout,"test_trie_element_subsumed_by_key()\n");
	trie_add(t, array123,array123);
	assert(trie_element_subsumed_by_key(t, array1));
	assert(trie_element_subsumed_by_key(t, array23));

	trie_remove_subsumed(t,array123);
	trie_gc(t);

	array123 = build_int_array(1,3);

	trie_add(t, array1,array1);
	assert(trie_element_subsumed_by_key(t, array1));
	assert(trie_element_subsumed_by_key(t, array23)==0);
	assert(trie_element_subsumed_by_key(t, array123)==0);

	trie_add(t,array123,array123);
	assert(trie_element_subsumed_by_key(t, array1));
	assert(trie_element_subsumed_by_key(t, array23));
	assert(trie_element_subsumed_by_key(t, array123));

	trie_free(t);

	fprintf(stdout,"Finished test_trie_element_subsumed_by_key()!\n");
}


void test_trie_contains_key()
{
	trie t = trie_new((trie_node_destroy_func_t)array_free,
	            (trie_node_clone_func_t)array_copy);
	array array123 = build_int_array(1,3);
	array array23 = build_int_array(2,3);
	array array1 = build_int_array(1,1);

	fprintf(stdout,"test_trie_contains_key()\n");
	trie_add(t, array123,array123);
	assert(trie_contains_key(t, array1)==0);
	assert(trie_contains_key(t, array23)==0);
	assert(trie_contains_key(t, array123));


	trie_remove_subsumed(t,array123);
	trie_gc(t);

	array123 = build_int_array(1,3);

	trie_add(t, array1,array1);
	assert(trie_contains_key(t, array1));
	assert(trie_contains_key(t, array23)==0);
	assert(trie_contains_key(t, array123)==0);


	trie_add(t,array123,array123);
	assert(trie_contains_key(t, array1));
	assert(trie_contains_key(t, array23)==0);
	assert(trie_contains_key(t, array123));
	trie_free(t);

	fprintf(stdout,"Finished test_trie_contains_key()!\n");
}



int main(int UNUSED(argc), char **UNUSED(argv))
{
    trie l, r;
    g_graph g;

    test_trie_is_subsumed_bug();
    test_trie_is_subsumed_fixed();
    test_trie_element_subsumed_by_key();
    test_trie_contains_key();
    return 1;

    l = make_trie(keys1);
    {
        r = make_trie(keys2);
/*
        trie_multiply_trie(l, r, NULL);
*/
        trie_remove_not_subsumed_trie(l, r);
        trie_free(r);
/*
        {
            array key1 = array_append(array_new(NULL, NULL), (void *)3);
            array key2 = array_append(array_new(NULL, NULL), (void *)70);
            trie_remove_subsumed(l, key1);
            trie_remove_subsumed(l, key2);
            trie_add(l, array_new(NULL, NULL), NULL);
            fprintf(stderr, "trie = %p\n", (void *)l);
            fprintf(stderr, "node_count = %d\n", l->node_count);
            fprintf(stderr, "deleted_nodes = %d\n", l->deleted_nodes);
            fprintf(stderr, "is_empty = %d\n", trie_is_empty(l));
            array_free(key2);
            array_free(key1);
        }
*/
    }

    g = graph_new();
    trie_to_graph(g, l);

    print_graphml(stdout, g);

    graph_free(g);

    trie_free(l);

    return EXIT_SUCCESS;
}
