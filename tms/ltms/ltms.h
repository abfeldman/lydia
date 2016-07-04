#ifndef LTMS_H
#define LTMS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tv.h"
#include "array.h"
#include "stack.h"

#define LTMS_LITERAL_TRUE           1
#define LTMS_LITERAL_FALSE          0

#define LTMS_NODE_FG_LABELED_FALSE  1
#define LTMS_NODE_FG_LABELED_TRUE   2
#define LTMS_NODE_FG_PREMISE        4
#define LTMS_NODE_FG_ASSUMPTION     8
#define LTMS_NODE_FG_ENABLED_FALSE 16
#define LTMS_NODE_FG_ENABLED_TRUE  32

/* Macros: */
#define ltms_node_set_flag(node, flag) ((node)->flags |= (flag))
#define ltms_node_has_flag(node, flag) (((node)->flags & (flag)) != 0)
#define ltms_node_clear_flag(node, flag) ((node)->flags &= ~(flag))

#define ltms_node_is_false(node) (((node)->flags & (LTMS_NODE_FG_LABELED_FALSE)) != 0)
#define ltms_node_is_true(node) (((node)->flags & (LTMS_NODE_FG_LABELED_TRUE)) != 0)
#define ltms_node_is_unknown(node) (((node)->flags & (LTMS_NODE_FG_LABELED_FALSE | LTMS_NODE_FG_LABELED_TRUE)) == 0)
#define ltms_node_set_false(node) ((node)->flags |= (LTMS_NODE_FG_LABELED_FALSE))
#define ltms_node_set_true(node) ((node)->flags |= (LTMS_NODE_FG_LABELED_TRUE))
#define ltms_node_set_unknown(node) ((node)->flags &= ~(LTMS_NODE_FG_LABELED_FALSE | LTMS_NODE_FG_LABELED_TRUE))
#define ltms_node_is_enabled_false(node) (((node)->flags & (LTMS_NODE_FG_ENABLED_FALSE)) != 0)
#define ltms_node_is_enabled_true(node) (((node)->flags & (LTMS_NODE_FG_ENABLED_TRUE)) != 0)

#define LTMS_CLAUSE_FG_UNIT_OPEN    1

#define ltms_clause_set_flag(clause, flag) ((clause)->flags |= (flag))
#define ltms_clause_has_flag(clause, flag) (((clause)->flags & (flag)) != 0)
#define ltms_clause_clear_flag(clause, flag) ((clause)->flags &= ~(flag))

typedef struct str_ltms *ltms;
typedef struct str_ltms_node *ltms_node;
typedef struct str_ltms_clause *ltms_clause;
typedef struct str_ltms_literal *ltms_literal;
typedef struct str_ltms_state *ltms_state;

struct str_ltms                                /* This is the LTMS engine context. */
{
    array nodes;
    array clauses;

    stack unit_open;
    stack undo_nodes;
    stack unset_nodes;

    unsigned int conflicts;
};

struct str_ltms_node                           /* LTMS node is a synonym of proposition. */
{
    signed int index;                          /* Variable name. */

    signed char flags;

    array pos_clauses;
    array neg_clauses;

    ltms_clause supporting_clause;
};

struct str_ltms_clause                         /* A clause consists of a disjunction of literals. */
{
    array literals;

    unsigned int pvs;                          /* Number of potential violators. */

    signed char flags;

    ltms_node satisfying_node;                 /* Node which satisfied this clause. */
    ltms_node supported_node;                  /* Node which is forces by this clause. */
};

struct str_ltms_literal
{
    ltms_node proposition;
    signed char sign;
};

struct str_ltms_state
{
    stack unit_open;
    stack undo_nodes;
    stack unset_nodes;

    unsigned int conflicts;

    unsigned int *clauses_pvs;
    ltms_node *clauses_satisfying_node;
    ltms_node *clauses_supported_node;

    signed char *nodes_flags;
    ltms_clause *nodes_supporting_clause;
};

/* Interface: */
extern ltms ltms_new();
extern void ltms_free(ltms);

extern int ltms_add_node(ltms, const int, const signed char, const signed char);
extern int ltms_add_clause(ltms, const_tv_clause);

extern int ltms_enable_assumption(ltms, const int, const signed char);
extern void ltms_retract_assumption(ltms, const int);
extern int ltms_has_contradiction(ltms);

extern array ltms_get_conflict_assumptions(ltms);

extern ltms_state ltms_state_get(const ltms);
extern void ltms_state_free(ltms_state);
extern void ltms_state_restore(ltms, ltms_state);

#ifdef __cplusplus
}
#endif

#endif
