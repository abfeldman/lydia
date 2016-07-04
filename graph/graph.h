#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>

typedef struct str_g_graph *g_graph;
typedef struct str_g_node *g_node;
typedef struct str_g_edge *g_edge;
typedef struct str_g_node_list *g_node_list;
typedef struct str_g_edge_list *g_edge_list;
typedef struct str_g_adjacency_list *g_adjacency_list;

struct str_g_adjacency_list
{
    unsigned int node_count;
    unsigned int edge_count;
    unsigned int *nodes;
    unsigned int *neighbours;
    unsigned int *edge_weights;
};

struct str_g_node_list
{
    unsigned int node_count;
    unsigned int nodes_room;
    
    g_node *nodes;
};

struct str_g_edge_list
{
    unsigned int edge_count;
    unsigned int edges_room;
    
    g_edge *edges;
};

struct str_g_node
{
    unsigned int id;
    char *label;

    double weight;

    void *datum;

    unsigned int color;
};

struct str_g_edge
{
    unsigned int id;
    unsigned int src_node;
    unsigned int dst_node;

    char *label;
    char *context;

    double weight;
};

struct str_g_graph
{
    unsigned int id;

    g_node *nodes;
    g_edge *edges;
    unsigned int node_count;
    unsigned int edge_count;
    unsigned int nodes_room;
    unsigned int edges_room;
};

#define GRAPH_NODES_ROOM 16
#define GRAPH_EDGES_ROOM 16

/* Function prototypes: */
extern g_adjacency_list graph_adjacency_list_new(const unsigned int, const unsigned int);
extern void graph_adjacency_list_free(g_adjacency_list);

extern g_graph graph_new();
extern g_edge graph_edge_new(const unsigned int, const unsigned int, const unsigned int);
extern g_node graph_node_new(const unsigned int);
extern g_edge_list graph_edge_list_new();
extern g_node_list graph_node_list_new();

extern g_graph graph_copy(const g_graph);
extern g_edge graph_edge_copy(const g_edge);
extern g_node graph_node_copy(const g_node);

extern void graph_free(g_graph);
extern void graph_edge_free(g_edge);
extern void graph_node_free(g_node);
extern void graph_edge_list_free(g_edge_list);
extern void graph_node_list_free(g_node_list);

extern int graph_edge_add(g_graph, const g_edge);
extern int graph_node_add(g_graph, const g_node);
extern int graph_edge_list_add(g_edge_list, const g_edge);
extern int graph_node_list_add(g_node_list, const g_node);

extern g_edge_list graph_node_remove(const g_graph, const unsigned int);

extern g_node_list graph_node_neighbours(g_graph, const unsigned int);

extern g_edge graph_edge_find(const g_graph, const unsigned int, const unsigned int);
extern g_node graph_node_find(const g_graph, const unsigned int);
extern g_node graph_node_find_offset(const g_graph, const unsigned int, unsigned int *);

extern int graph_node_list_member(const g_node_list, const unsigned int);

extern g_adjacency_list graph_adjacency_list_get(const g_graph);

#include "tv.h"

extern g_graph sd2graph(const_tv_wff);

#endif
