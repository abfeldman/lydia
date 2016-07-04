#ifndef DIAG
#define DIAG

#include "variable.h"
#include "array.h"
#include "trie.h"
#include "obs.h"
#include "tv.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct str_diagnostic_problem *diagnostic_problem;

typedef void (* fm_printer_func_t)(FILE *,
                                   const_faultmode,
                                   const unsigned int,
                                   const_variable_list,
                                   const_variable_list,
                                   const_values_set_list,
                                   const int);
typedef void (* prob_printer_func_t)(FILE *,
                                     const_faultmode,
                                     const unsigned int);
typedef void (* card_printer_func_t)(FILE *,
                                     const_faultmode,
                                     const unsigned int);
typedef void (* entropy_printer_func_t)(FILE *, const double);
typedef void (* count_printer_func_t)(FILE *, const unsigned);
typedef void (* list_item_printer_func_t)(FILE *, const char *);

typedef int (* fm_func_t)(FILE *, diagnostic_problem, fm_printer_func_t);
typedef int (* probs_func_t)(FILE *, diagnostic_problem, prob_printer_func_t);
typedef int (* cards_func_t)(FILE *, diagnostic_problem, card_printer_func_t);
typedef int (* entropy_func_t)(FILE *, diagnostic_problem, entropy_printer_func_t);
typedef int (* count_func_t)(FILE *, diagnostic_problem, count_printer_func_t);

typedef int (* terminate_func_t)();

struct str_diagnostic_problem
{
    array diagnoses;

    trie all_diagnoses;

    tv_variables_cache tv_cache;
    mv_variables_cache mv_cache;

    union
    {
        const_horn horn_sd;
        const_tv_cnf tv_cnf_sd;
        const_tv_dnf tv_dnf_sd;
        struct
        {
            node root;
            void *model; /* dnf_tree */
        } tv_tdnf_sd;
    } u;

    variable_list variables;
    variable_list encoded_variables;
    values_set_list domains;
    int encoding;

    fm_func_t fm;
    probs_func_t probs;
    cards_func_t cards;
    entropy_func_t entropy;
    count_func_t count;

    lydia_symbol name;

    const_serializable model;
    observations alphas;
};

extern diagnostic_problem diagnostic_problem_new(const_serializable);
extern int diagnostic_problem_reset(diagnostic_problem);
extern int add_diagnosis(diagnostic_problem, const_faultmode, int *);
extern int add_diagnosis_from_tv_term(diagnostic_problem, const_tv_term, int *, const signed char);
extern int add_diagnosis_from_mv_term(diagnostic_problem, const_mv_term, int *);
extern void diagnostic_problem_free(diagnostic_problem);

extern int diagnostic_problem_add(const char *, diagnostic_problem, observations);
extern diagnostic_problem diagnostic_problem_get(const char *);
extern void diagnostic_problems_free();
extern void diagnostic_problems_list(FILE *, list_item_printer_func_t);
extern void observations_list(FILE *, const diagnostic_problem, list_item_printer_func_t);

/* Termination: */
extern void set_max_diagnoses(const int);
extern int  get_max_diagnoses();
extern void set_max_cardinality(const int);
extern void set_max_time(const double);
extern void set_option_terminate_mc(const signed char);
extern void set_terminate(const terminate_func_t);
extern void diagnosis_reset();
extern void diagnosis_found(const int);
extern void candidate_found(const int);
extern int stop_diagnoses();
extern int is_terminate();
extern int is_timeout();

/* Others: */
extern faultmode tv_term_to_faultmode(const_tv_variables_cache,
                                      const_tv_term,
                                      unsigned int *,
                                      const signed char);
extern faultmode mv_term_to_faultmode(diagnostic_problem, const_mv_term);

#ifdef __cplusplus
}
#endif

#endif
