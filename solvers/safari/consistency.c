#include "consistency.h"

#include "array.h"
#include "stat.h"
#include "ltms.h"
#include "sat.h"
#include "ds.h"
#include "tv.h"

#include <assert.h>

int safari_is_sat(const_tv_cnf sd,
                  const_tv_clause_list obs_clauses,
                  const_tv_term candidate,
                  const int solver)
{
    tv_clause_list candidate_clauses = term_to_tv_clause_list(candidate);
    tv_clause_list sat_instance = new_tv_clause_list();

    unsigned int ix = obs_clauses->sz + candidate_clauses->sz + sd->clauses->sz;

    int result;

    setroom_tv_clause_list(sat_instance, ix);

    memcpy(sat_instance->arr,
           obs_clauses->arr,
           sizeof(tv_clause) * obs_clauses->sz);
    memcpy(sat_instance->arr + obs_clauses->sz,
           candidate_clauses->arr,
           sizeof(tv_clause) * candidate_clauses->sz);
    memcpy(sat_instance->arr + obs_clauses->sz + candidate_clauses->sz,
           sd->clauses->arr,
           sizeof(tv_clause) * sd->clauses->sz);

    sat_instance->sz = ix;

    increase_int_counter("dpll_calls");
    if (!(result = is_consistent(sat_instance,
                                 sd->variables->sz,
                                 solver))) {
        increase_int_counter("dpll_unsat");
    }
    for ( ; ix < sat_instance->sz; ix++) {
        rfre_tv_clause(sat_instance->arr[ix]);
    }

    fre_tv_clause_list(sat_instance);
    rfre_tv_clause_list(candidate_clauses);

    return result;
}

int safari_enable_observation_assumptions(ltms tms, const_tv_term term)
{
    register unsigned int ix;

    const_int_list pos = term->pos;
    const_int_list neg = term->neg;

/* Enable the observations in the LTMS. */
    for (ix = 0; ix < pos->sz; ix++) {
        ltms_enable_assumption(tms, pos->arr[ix], 1);
        if (ltms_has_contradiction(tms)) {
            return 0;
        }
    }
    for (ix = 0; ix < neg->sz; ix++) {
        ltms_enable_assumption(tms, neg->arr[ix], 0);
        if (ltms_has_contradiction(tms)) {
            return 0;
        }
    }
    return !ltms_has_contradiction(tms);
}

int safari_enable_health_assumptions(ltms tms,
                                     const_tv_variables_cache tv_cache,
                                     array assignments)
{
    register unsigned int ix;

    for (ix = 0; ix < assignments->sz; ix++) {
        safari_literal lit = (safari_literal)assignments->arr[ix];
        ltms_enable_assumption(tms, tv_cache->a_to_v[lit->var], lit->val);
        if (ltms_has_contradiction(tms)) {
            return 0;
        }
    }

    return !ltms_has_contradiction(tms);
}

void safari_retract_observation_assumptions(ltms tms, const_tv_term term)
{
    register unsigned int ix;

    const_int_list neg = term->neg;
    const_int_list pos = term->pos;

/* Enable the observations in the LTMS. */
    for (ix = 0; ix < neg->sz; ix++) {
        ltms_retract_assumption(tms, neg->arr[ix]);
    }
    for (ix = 0; ix < pos->sz; ix++) {
        ltms_retract_assumption(tms, pos->arr[ix]);
    }
}

void safari_retract_health_assumptions(ltms tms,
                                       const_tv_variables_cache tv_cache,
                                       array assignments)
{
    register unsigned int ix;

    for (ix = 0; ix < assignments->sz; ix++) {
        safari_literal lit = (safari_literal)assignments->arr[ix];

        ltms_retract_assumption(tms, tv_cache->a_to_v[lit->var]);
    }
}
