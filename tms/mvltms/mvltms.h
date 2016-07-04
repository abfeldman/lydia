/* filename:     mvltms.h
 * description:  multivalued Truth Maintenance System
 * author:       Tom Janssen (TU Delft)
 */

#ifndef __MVLTMS_H__
#define __MVLTMS_H__

#include "mv.h"
#include "array.h"
#include "stack.h"

#define MVLTMS_NODE_LABEL_UNASSIGNED      -1
#define MVLTMS_NODE_LABEL_ASSIGNED         1


typedef struct str_mvltms *mvltms;
typedef struct str_mvltms_node *mvltms_node;
typedef struct str_mvltms_clause *mvltms_clause;
typedef struct str_mvltms_literal *mvltms_literal;

struct str_mvltms                                /* This is the LTMS engine context. */
{
    array nodes;
    array clauses;

    stack violated;
    stack unit_open;
    stack undo_nodes;

    signed char constructed;

    unsigned int time_stamp;

    unsigned int level;			/* maintains the current level of assumptions */
    unsigned int index;			/* maintains the current index of assignments */

    /* TODO: shouldn't be in this struct
     * only for adding pure positive literalled clauses */
    variable_list variables;
    values_set_list domains;
};

struct str_mvltms_node                           /* LTMS node is a synonym of proposition. */
{
    signed int index;                          /* Variable name. */

    signed char label;                         /* UNASSIGNED or ASSIGNED. */
    int value;

    array clauses;

    mvltms_clause supporting_clause;

    mvltms_clause assumption_clause;      /* If the node is assumed, a unit open clause, NULL otherwise. */

    unsigned int assume_level;				/* level at which node was assumed */
    unsigned int assign_index;				/* index at which node was assigned */
};

struct str_mvltms_clause                         /* A clause consists of a disjunction of literals. */
{
    signed char label;
    signed char assumption;
    signed char observation;

    array literals;

    unsigned int unassigned;                   /* Number of unassigned literals. */

    mvltms_node satisfying_node;                 /* Node which satisfied this clause. */
    mvltms_node supported_node;                  /* Node which is forces by this clause. */

    unsigned int unit_open_index;              /* Index in the unit open clause stack or -1 if not unit open. */
    unsigned int violated_index;               /* Index in the violated clauses stack or -1 if not unsat. */

    unsigned int time_stamp;

    mvltms_node conflicting_node;		/* node which violated this clause */
};

struct str_mvltms_literal
{
    mvltms_node proposition;
    int value;
};

/* Interface: */
extern mvltms mvltms_new(variable_list variables, values_set_list domains);
extern void mvltms_free(mvltms);

extern int mvltms_add_node(mvltms, const int);
extern int mvltms_add_clause(mvltms, mv_clause); /* because of mvltms handling conversion of mvclause2posmvclause, mv_clause is no longer const */

extern int mvltms_enable_assumption(mvltms, const int, const int, const signed char);
extern void mvltms_retract_assumption(mvltms, const int);
extern int mvltms_has_contradiction(mvltms);

extern array mvltms_get_conflict_assumptions(mvltms);
extern mv_clause mvltms_get_conflict_learning_clause(mvltms engine, unsigned int *backjump_node);

#endif

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
