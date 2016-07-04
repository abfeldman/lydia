/**
 *  \file graph.c
 *  \brief Implementation of graphs.
 */

#include "config.h"
#include "strdup.h"
#include "graph.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

g_edge graph_edge_new(const unsigned int id,
                      const unsigned int src_node,
                      const unsigned int dst_node)
{
    g_edge e = (g_edge)malloc(sizeof(struct str_g_edge));
    if (NULL == e) {
        return NULL;
    }

    e->id = id;
    e->label = NULL;
    e->context = NULL;

    e->src_node = src_node;
    e->dst_node = dst_node;
    e->weight = 0.0;

    return e;
}

void graph_edge_free(g_edge e)
{
    free(e);
}

g_node graph_node_new(const unsigned int id)
{
    g_node n = (g_node)malloc(sizeof(struct str_g_node));
    if (NULL == n) {
        return NULL;
    }

    n->id = id;
    n->label = NULL;
    n->weight = 0.0;
    n->color = 0x1ffffff;

    return n;
}

g_graph graph_copy(const g_graph g)
{
    g_graph result = graph_new();

    register unsigned int ix;

    for (ix = 0; ix < g->node_count; ix++) {
        g_node n = graph_node_new(g->nodes[ix]->id);
        n->label = (g->nodes[ix]->label == NULL ? NULL : strdup(g->nodes[ix]->label));
        n->weight = g->nodes[ix]->weight;
        n->color = g->nodes[ix]->color;
        graph_node_add(result, n);
    }
    for (ix = 0; ix < g->edge_count; ix++) {
        g_edge e = graph_edge_new(g->edges[ix]->id,
                                  g->edges[ix]->src_node,
                                  g->edges[ix]->dst_node);
        e->weight = g->edges[ix]->weight;
        graph_edge_add(result, e);
    }

    return result;
}

void graph_node_free(g_node n)
{
    free(n->label);
    free(n);
}

g_graph graph_new()
{
    g_graph g = (g_graph)malloc(sizeof(struct str_g_graph));
    if (NULL == g) {
        return NULL;
    }

    g->nodes = NULL;
    g->edges = NULL;
    g->node_count = 0;
    g->edge_count = 0;
    g->nodes_room = 0;
    g->edges_room = 0;

    return g;

}

void graph_free(g_graph g)
{
    register unsigned int ix;

    for (ix = 0; ix < g->node_count; ix++) {
        graph_node_free(g->nodes[ix]);
    }
    for (ix = 0; ix < g->edge_count; ix++) {
        graph_edge_free(g->edges[ix]);
    }

    free(g->nodes);
    free(g->edges);

    free(g);
}

g_edge graph_edge_copy(const g_edge e)
{
    g_edge result = graph_edge_new(e->id, e->src_node, e->dst_node);

    result->weight = e->weight;
    result->label = (e->label == NULL ? NULL : strdup(e->label));

    return result;
}

g_node graph_node_copy(const g_node n)
{
    g_node result = graph_node_new(n->id);

    result->weight = n->weight;
    result->label = (n->label == NULL ? NULL : strdup(n->label));
    result->color = n->color;

    return result;
}

int graph_node_add(g_graph g, const g_node n)
{
    if (g->nodes_room < g->node_count + 1) {
        void **nodes = (void **)realloc(g->nodes, (g->nodes_room + GRAPH_NODES_ROOM) * sizeof(g_node *));
        if (NULL == nodes) {
            return 0;
        }
        g->nodes = (g_node *)nodes;
        g->nodes_room += GRAPH_NODES_ROOM;
    }

    g->nodes[g->node_count] = n;
    g->node_count += 1;

    return 1;
}

int graph_edge_add(g_graph g, const g_edge n)
{
    if (g->edges_room < g->edge_count + 1) {
        void **edges = (void **)realloc(g->edges, (g->edges_room + GRAPH_EDGES_ROOM) * sizeof(g_edge *));
        if (NULL == edges) {
            return 0;
        }
        g->edges = (g_edge *)edges;
        g->edges_room += GRAPH_EDGES_ROOM;
    }

    g->edges[g->edge_count] = n;
    g->edge_count += 1;

    return 1;
}

int graph_node_list_member(const g_node_list l, const unsigned int id)
{
    register unsigned int ix;

    for (ix = 0; ix < l->node_count; ix++) {
        if (l->nodes[ix]->id == id) {
            return 1;
        }
    }

    return 0;
}

g_node graph_node_find(const g_graph g, const unsigned int id)
{
    register unsigned int ix;

    for (ix = 0; ix < g->node_count; ix++) {
        if (g->nodes[ix]->id == id) {
            return g->nodes[ix];
        }
    }

    return NULL;
}

g_node graph_node_find_offset(const g_graph g, const unsigned int id, unsigned int *pos)
{
    register unsigned int ix;

    for (ix = 0; ix < g->node_count; ix++) {
        if (g->nodes[ix]->id == id) {
            *pos = ix;
            return g->nodes[ix];
        }
    }

    return NULL;
}

g_edge graph_edge_find(const g_graph g,
                       const unsigned int src_node,
                       const unsigned int dst_node)
{
    register unsigned int ix;

    for (ix = 0; ix < g->edge_count; ix++) {
        if ((g->edges[ix]->src_node == src_node && g->edges[ix]->dst_node == dst_node) ||
            (g->edges[ix]->src_node == dst_node && g->edges[ix]->dst_node == src_node)) {
            return g->edges[ix];
        }
    }

    return NULL;
}

g_node_list graph_node_neighbours(g_graph g, const unsigned int nid)
{
    register unsigned int ix;

    g_node_list result = graph_node_list_new();

    for (ix = 0; ix < g->edge_count; ix++) {
        if (g->edges[ix]->src_node == nid) {
            g_node dst = graph_node_find(g, g->edges[ix]->dst_node);
            assert(dst != NULL);

            graph_node_list_add(result, dst);
        }
        if (g->edges[ix]->dst_node == nid) {
            g_node src = graph_node_find(g, g->edges[ix]->src_node);
            assert(src != NULL);

            graph_node_list_add(result, src);
        }
    }

    return result;
}

g_adjacency_list graph_adjacency_list_new(const unsigned int node_count,
                                          const unsigned int edge_count)
{
    g_adjacency_list l = (g_adjacency_list)malloc(sizeof(struct str_g_adjacency_list));
    if (NULL == l) {
        return NULL;
    }

    l->node_count = node_count;
    l->edge_count = edge_count;
    l->nodes = (unsigned int *)malloc((node_count + 1) * sizeof(unsigned int));
    if (l->nodes == NULL) {
        free(l);
        return NULL;
    }
    l->neighbours = (unsigned int *)malloc(2 * edge_count * sizeof(unsigned int));
    if (l->neighbours == NULL) {
        free(l->nodes);
        free(l);
        return NULL;
    }
    l->edge_weights = (unsigned int *)malloc(2 * edge_count * sizeof(unsigned int));
    if (l->edge_weights == NULL) {
        free(l->nodes);
        free(l->neighbours);
        free(l);
        return NULL;
    }

    return l;
}

void graph_adjacency_list_free(g_adjacency_list l)
{
    free(l->nodes);
    free(l->neighbours);
    free(l->edge_weights);
    free(l);
}

g_adjacency_list graph_adjacency_list_get(const g_graph g)
{
    register unsigned int ix, iy, iz = 0;
    unsigned int pos;

    g_adjacency_list result = graph_adjacency_list_new(g->node_count,
                                                       g->edge_count);
    if (NULL == result) {
        return NULL;
    }

    for (ix = 0; ix < g->node_count; ix++) {
        result->nodes[ix] = iz;
        for (iy = 0; iy < g->edge_count; iy++) {
            if (g->edges[iy]->src_node == g->nodes[ix]->id) {
                g_node dst = graph_node_find_offset(g, g->edges[iy]->dst_node, &pos);
                assert(dst != NULL);
/*
                result->neighbours[iz] = dst->id;
*/
                result->neighbours[iz] = pos;
                result->edge_weights[iz] = g->edges[iy]->weight;
                iz += 1;
            }
            if (g->edges[iy]->dst_node == g->nodes[ix]->id) {
                g_node src = graph_node_find_offset(g, g->edges[iy]->src_node, &pos);
                assert(src != NULL);
/*
                result->neighbours[iz] = src->id;
*/
                result->neighbours[iz] = pos;
                result->edge_weights[iz] = g->edges[iy]->weight;
                iz += 1;
            }
        }
    }
    result->nodes[ix] = iz;

    return result;
}

g_node_list graph_node_list_new()
{
    g_node_list l = (g_node_list)malloc(sizeof(struct str_g_edge_list));
    if (NULL == l) {
        return NULL;
    }

    l->nodes = NULL;
    l->node_count = 0;
    l->nodes_room = 0;

    return l;
}

g_edge_list graph_edge_list_new()
{
    g_edge_list l = (g_edge_list)malloc(sizeof(struct str_g_edge_list));
    if (NULL == l) {
        return NULL;
    }

    l->edges = NULL;
    l->edge_count = 0;
    l->edges_room = 0;

    return l;
}

g_edge_list graph_node_remove(const g_graph g, const unsigned int id)
{
    register unsigned int ix, iy = 0;

    g_edge_list result = graph_edge_list_new();

    for (ix = g->node_count - 1; ix < g->node_count; ix--) {
        if (g->nodes[ix]->id == id) {
            graph_node_free(g->nodes[ix]);
            memmove(&g->nodes[ix], &g->nodes[ix + 1], sizeof(g_node) * (g->node_count - ix - 1));
            g->node_count -= 1;
        }
    }
    for (ix = g->edge_count - 1; ix < g->edge_count; ix--) {
        if (g->edges[ix]->src_node == id ||
            g->edges[ix]->dst_node == id) {
            graph_edge_list_add(result, g->edges[ix]);
            memmove(&g->edges[ix], &g->edges[ix + 1], sizeof(g_edge) * (g->edge_count - ix - 1));
            g->edge_count -= 1;
            iy += 1;
        }
    }

    return result;
}

int graph_node_list_add(g_node_list l, const g_node n)
{
    if (l->nodes_room < l->node_count + 1) {
        void **nodes = (void **)realloc(l->nodes, (l->nodes_room + GRAPH_NODES_ROOM) * sizeof(g_node *));
        if (NULL == nodes) {
            return 0;
        }
        l->nodes = (g_node *)nodes;
        l->nodes_room += GRAPH_NODES_ROOM;
    }

    l->nodes[l->node_count] = n;
    l->node_count += 1;

    return 1;
}

int graph_edge_list_add(g_edge_list l, const g_edge e)
{
    if (l->edges_room < l->edge_count + 1) {
        void **edges = (void **)realloc(l->edges, (l->edges_room + GRAPH_EDGES_ROOM) * sizeof(g_edge *));
        if (NULL == edges) {
            return 0;
        }
        l->edges = (g_edge *)edges;
        l->edges_room += GRAPH_EDGES_ROOM;
    }

    l->edges[l->edge_count] = e;
    l->edge_count += 1;

    return 1;
}

void graph_node_list_free(g_node_list l)
{
    register unsigned int ix;

    for (ix = 0; ix < l->node_count; ix++) {
        graph_node_free(l->nodes[ix]);
    }
    free(l->nodes);
    free(l);
}

void graph_edge_list_free(g_edge_list l)
{
    register unsigned int ix;

    for (ix = 0; ix < l->edge_count; ix++) {
        graph_edge_free(l->edges[ix]);
    }
    free(l->edges);
    free(l);
}

