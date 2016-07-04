#include "rand_nr.h"
#include "strsep.h"
#include "cones.h"
#include "defs.h"
#include "stat.h"
#include "util.h"
#include "sat.h"

#include <assert.h>

cones_context cones_context_new(diagnostic_problem ds,
                                diagnostic_problem full_ds,
                                array map)
{
    cones_context result;

    if (NULL == (result = (cones_context)malloc(sizeof(struct str_cones_context)))) {
        return result;
    }

    result->ds = ds;
    result->full_ds = full_ds;
    result->map = map;

    return result;
}

void cones_context_free(cones_context context)
{
    if (context != NULL) {
        if (NULL != context->map) {
            array_free(context->map);
        }
        free(context);
    }
}

static int sd_obs_omega_is_sat(const_tv_cnf sd,
                               const_tv_clause_list alpha,
                               const_tv_term candidate,
                               const int solver)
{
    tv_clause_list candidate_clauses = term_to_tv_clause_list(candidate);
    tv_clause_list sat_instance = new_tv_clause_list();

    unsigned int ix;

    int result;

    ix = alpha->sz + candidate_clauses->sz + sd->clauses->sz;

    setroom_tv_clause_list(sat_instance, ix);

    memcpy(sat_instance->arr,
           alpha->arr,
           sizeof(tv_clause) * alpha->sz);
    memcpy(sat_instance->arr + alpha->sz,
           candidate_clauses->arr,
           sizeof(tv_clause) * candidate_clauses->sz);
    memcpy(sat_instance->arr + alpha->sz + candidate_clauses->sz,
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

cones_context map_cones(FILE *conesfile,
                        const diagnostic_problem ds,
                        const diagnostic_problem full_ds,
                        signed char *rc)
{
    char *line;
    char *p;
    char *cone;
    char *cone_contents;

    char *cone_variable;

    identifier cone_id;
    identifier cone_variable_id;

    unsigned int cone_idx;
    unsigned int cone_variable_idx;

    array map;

    register unsigned int ix;

    cones_context result;

    map = array_new((array_element_destroy_func_t)array_free,
                    (array_element_clone_func_t)array_copy);

    array_setroom(map, ds->tv_cache->assumables);

    for (ix = 0; ix < ds->tv_cache->assumables; ix++) {
        map->arr[ix] = array_new(NULL, NULL);
    }
    map->sz = ds->tv_cache->assumables;

    while ((line = read_line(conesfile, 10485760 /* 1 Mb */)) != NULL) {
        p = strchr(line, ':');
        assert(p != NULL);

        *p = '\0';

        cone = stripwhite(line);
        cone_contents = stripwhite(p + 1);

        cone_id = make_variable_identifier(cone);
        if (!search_variable_list(ds->variables, cone_id, &cone_idx)) {
            array_free(map);
            rfre_identifier(cone_id);
            free(line);

            *rc = MAP_CONES_UNKNOWN_CONE;
            return NULL;
        }

        rfre_identifier(cone_id);

        while ((cone_variable = strsep(&cone_contents, ",")) != NULL) {
            stripwhite(cone_variable);

            cone_variable_id = make_variable_identifier(cone_variable);
            if (!search_variable_list(full_ds->variables,
                                      cone_variable_id,
                                      &cone_variable_idx)) {
                array_free(map);
                rfre_identifier(cone_variable_id);
                free(line);

                *rc = MAP_CONES_UNKNOWN_VARIABLE;
                return NULL;
            }

            rfre_identifier(cone_variable_id);

            array_append(map->arr[ds->tv_cache->v_to_a[cone_idx]],
                         ui2p(full_ds->tv_cache->v_to_a[cone_variable_idx]));
        }

        free(line);
    }

    result = cones_context_new(ds, full_ds, map);
    if (NULL == result) {
        array_free(map);
        *rc = MAP_CONES_OUT_OF_MEMORY;
        return NULL;
    }

    *rc = MAP_CONES_SUCCESS;

    return result;
}

static signed char increase_counter(array counter, array max)
{
    register unsigned int ix;

    counter->arr[0] = ui2p(p2ui(counter->arr[0]) + 1);

    for (ix = 0; ix < counter->sz; ix++) {
        if (counter->arr[ix] < max->arr[ix]) {
            break;
        }
        counter->arr[ix] = 0;
        if (ix + 1 == max->sz) {
            return 0;
        }
        counter->arr[ix + 1] = ui2p(p2ui(counter->arr[ix + 1]) + 1);
    }

    return 1;
}

signed char expand_cone_exhaustive(const cones_context context,
                                   const_tv_term alpha,
                                   const_faultmode fault)
{
    variable_assignment assignment;

    register unsigned int iy;
    register unsigned int iz;

    unsigned int cone_var;
    lydia_bool cone_value;
    unsigned int cone_size;
    unsigned int full_model_var;

    tv_term candidate;

    array cone_contents;
    array counter;
    array max;

    int terminate = 0;

    tv_clause_list alpha_clauses;

    char *buf;

    buf = malloc(context->full_ds->tv_cache->assumables);
    if (NULL == buf) {
        return 0;
    }

    alpha_clauses = term_to_tv_clause_list(alpha);

    counter = array_new(NULL, NULL);
    max = array_new(NULL, NULL);

    for (iy = 0; iy < fault->assignments->sz; iy++) {
        assignment = fault->assignments->arr[iy];

        assert(assignment->tag == TAGbool_variable_assignment);

        cone_value = to_bool_variable_assignment(assignment)->value;
        cone_var = context->ds->tv_cache->v_to_a[assignment->var];
        if ((cone_value && (context->ds->tv_cache->health_states[cone_var] & 1)) ||
            (!cone_value && (context->ds->tv_cache->health_states[cone_var]) & 2)) {

            cone_size = ((array)(context->map->arr[cone_var]))->sz;

            array_append(counter, ui2p(0));
            array_append(max, ui2p(cone_size));
        }
    }

    if (max->sz == 0) {
/* Add the empty diagnosis (nominal state). */
        candidate = new_tv_term(new_int_list(), new_int_list());
        if (!add_diagnosis_from_tv_term(context->full_ds,
                                        candidate,
                                        &terminate,
                                        1)) {
        }
        rfre_tv_term(candidate);

        array_free(counter);
        array_free(max);
 
        rfre_tv_clause_list(alpha_clauses);
 
        free(buf);
 
        return 1;
    }

    do {
        candidate = new_tv_term(new_int_list(), new_int_list());

        memset(buf, 0, sizeof(char) * context->full_ds->tv_cache->assumables);

        for (iy = iz = 0; iy < fault->assignments->sz; iy++) {
            assignment = fault->assignments->arr[iy];

            cone_value = to_bool_variable_assignment(assignment)->value;
            cone_var = context->ds->tv_cache->v_to_a[assignment->var];
            if ((cone_value && (context->ds->tv_cache->health_states[cone_var] & 1)) ||
                (!cone_value && (context->ds->tv_cache->health_states[cone_var] & 2))) {
                cone_contents = context->map->arr[cone_var];

                full_model_var = p2i(cone_contents->arr[p2i(counter->arr[iz])]);
                buf[full_model_var] = 1;

                append_int_list(candidate->neg,
                                context->full_ds->tv_cache->a_to_v[full_model_var]);

                iz += 1;
            }
        }
        for (iy = 0; iy < context->full_ds->tv_cache->assumables; iy++) {
            if (!buf[iy]) {
                append_int_list(candidate->pos,
                                context->full_ds->tv_cache->a_to_v[iy]);
            }
        }
        if (sd_obs_omega_is_sat(context->full_ds->u.tv_cnf_sd,
                                alpha_clauses,
                                candidate,
                                SAT_SOLVER_MINISAT)) {
            if (!add_diagnosis_from_tv_term(context->full_ds,
                                            candidate,
                                            &terminate,
                                            1)) {
            }
        }
        rfre_tv_term(candidate);
    } while (!terminate && increase_counter(counter, max));

    array_free(counter);
    array_free(max);

    rfre_tv_clause_list(alpha_clauses);

    free(buf);

    return 1;
}

signed char expand_cone_random(const cones_context context,
                               const_tv_term alpha,
                               const_faultmode fault)
{
    variable_assignment assignment;

    register unsigned int iy;
    register unsigned int iz;

    unsigned int cone_var;
    lydia_bool cone_value;
    unsigned int cone_size;
    unsigned int full_model_var;
    unsigned int rand_cone_var;

    tv_term candidate;

    array cone_contents;
    array max;

    int terminate = 0;

    tv_clause_list alpha_clauses;

    char *buf;

    buf = malloc(context->full_ds->tv_cache->assumables);
    if (NULL == buf) {
        return 0;
    }

    alpha_clauses = term_to_tv_clause_list(alpha);

    max = array_new(NULL, NULL);

    for (iy = 0; iy < fault->assignments->sz; iy++) {
        assignment = fault->assignments->arr[iy];

        assert(assignment->tag == TAGbool_variable_assignment);

        cone_value = to_bool_variable_assignment(assignment)->value;
        cone_var = context->ds->tv_cache->v_to_a[assignment->var];
        if ((cone_value && (context->ds->tv_cache->health_states[cone_var] & 1)) ||
            (!cone_value && (context->ds->tv_cache->health_states[cone_var]) & 2)) {

            cone_size = ((array)(context->map->arr[cone_var]))->sz;

            array_append(max, ui2p(cone_size));
        }
    }

    if (max->sz == 0) {
/* Add the empty diagnosis (nominal state). */
        candidate = new_tv_term(new_int_list(), new_int_list());
        if (!add_diagnosis_from_tv_term(context->full_ds,
                                        candidate,
                                        &terminate,
                                        1)) {
        }
        rfre_tv_term(candidate);

        array_free(max);
 
        rfre_tv_clause_list(alpha_clauses);
 
        free(buf);
 
        return 1;
    }

    do {
        candidate = new_tv_term(new_int_list(), new_int_list());

        memset(buf, 0, sizeof(char) * context->full_ds->tv_cache->assumables);

        for (iy = iz = 0; iy < fault->assignments->sz; iy++) {
            assignment = fault->assignments->arr[iy];

            cone_value = to_bool_variable_assignment(assignment)->value;
            cone_var = context->ds->tv_cache->v_to_a[assignment->var];
            if ((cone_value && (context->ds->tv_cache->health_states[cone_var] & 1)) ||
                (!cone_value && (context->ds->tv_cache->health_states[cone_var] & 2))) {
                cone_contents = context->map->arr[cone_var];

                rand_cone_var = (unsigned int)(rand() / (((double)RAND_MAX + 1) / p2i(max->arr[iz])));

                full_model_var = p2i(cone_contents->arr[rand_cone_var]);
                buf[full_model_var] = 1;

                append_int_list(candidate->neg,
                                context->full_ds->tv_cache->a_to_v[full_model_var]);

                iz += 1;
            }
        }
        for (iy = 0; iy < context->full_ds->tv_cache->assumables; iy++) {
            if (!buf[iy]) {
                append_int_list(candidate->pos,
                                context->full_ds->tv_cache->a_to_v[iy]);
            }
        }
        if (sd_obs_omega_is_sat(context->full_ds->u.tv_cnf_sd,
                                alpha_clauses,
                                candidate,
                                SAT_SOLVER_MINISAT)) {
            if (!add_diagnosis_from_tv_term(context->full_ds,
                                            candidate,
                                            &terminate,
                                            1)) {
            }
            if (context->full_ds->diagnoses->sz == 1) {
                terminate = 1;
            }
        }
        rfre_tv_term(candidate);
    } while (!terminate);

    array_free(max);

    rfre_tv_clause_list(alpha_clauses);

    free(buf);

    return 1;
}
