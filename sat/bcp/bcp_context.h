#ifndef BCP_CONTEXT_H
#define BCP_CONTEXT_H

#include "tv.h"
#include "list.h"
#include "queue.h"

#define ASSIGNMENT_UNKNOWN -1
#define ASSIGNMENT_TRUE     1
#define ASSIGNMENT_FALSE    0

#define SIGN_FALSE          0
#define SIGN_TRUE           1

typedef struct str_bcp_problem *bcp_problem;
typedef struct str_bcp_variable *bcp_variable;

struct str_bcp_variable
{
/** The current value of the variable. */
    char assignment;
/** The clauses in which this variable appears as a positive literal. */
    int_list pos_clauses;
/** The clauses in which this variable appears as a negative literal. */
    int_list neg_clauses;

    tv_clause pos_filter_clause;
    tv_clause neg_filter_clause;
};

struct str_bcp_problem
{
    bcp_variable *variables;

    unsigned int *satisfied;
    unsigned int *labelled;

    unsigned int satisfied_clauses;

/** Number of variables. */
    unsigned int variables_count;
/** Number of clauses in the original CNF. */
    unsigned int clauses_count;
/** A copy to the clausal list. */
    tv_clause_list clauses;

    queue unit_variables_pos;
    queue unit_variables_neg;

    unsigned char *unit_variables_pos_map;
    unsigned char *unit_variables_neg_map;

    unsigned char consistent;
};

extern bcp_problem bcp_new_problem(tv_clause_list clauses, const unsigned int variables_count);
extern void bcp_free_problem(bcp_problem context);

#endif
