#ifndef LSSS_CONTEXT_H
#define LSSS_CONTEXT_H

#include "tv.h"
#include "list.h"
#include "queue.h"

#define ASSIGNMENT_UNKNOWN -1
#define ASSIGNMENT_TRUE     1
#define ASSIGNMENT_FALSE    0

#define SIGN_FALSE          0
#define SIGN_TRUE           1

#define LITERAL_MAP_ROOM   64

typedef struct str_dpll_variable *dpll_variable;
typedef struct str_dpll_problem *dpll_problem;

struct str_dpll_variable
{
/** The current value of the variable. */
    char assignment;
/** The clauses in which this variable appears as a positive literal. */
    int_list pos_clauses;
/** The clauses in which this variable appears as a negative literal. */
    int_list neg_clauses;

    tv_clause pos_filter_clause;
    tv_clause neg_filter_clause;
    tv_clause delete_clause;
};

struct str_dpll_problem
{
    dpll_variable *variables;

    unsigned int *satisfied;
    unsigned int *labelled;

    unsigned int satisfied_clauses;

/** Number of variables. */
    unsigned int variables_count;
/** Number of clauses in the original CNF. */
    unsigned int clauses_count;
/** A copy to the clausal list. */
    tv_clause_list clauses;

    tv_clause empty_clause;

    queue unit_variables_pos;
    queue unit_variables_neg;

    unsigned char *unit_variables_pos_map;
    unsigned char *unit_variables_neg_map;

    unsigned int *assignment_history;

    unsigned int assigned_variables;
    unsigned int contradiction_variable;
    unsigned int current_variable;

    unsigned int branch_points;

    unsigned char consistent;

    unsigned char randomized;
};

extern void dpll_enable_clause_simplification(const signed char);
extern dpll_problem dpll_new_problem(tv_clause_list, const unsigned int);
extern void dpll_free_problem(dpll_problem);
extern void dpll_add_clause(dpll_problem, tv_clause);

#endif
