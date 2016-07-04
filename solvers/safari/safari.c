#include "pp_variable.h"
#include "consistency.h"
#include "rand_nr.h"
#include "safari.h"
#include "qsort.h"
#include "cones.h"
#include "defs.h"
#include "lsss.h"
#include "stat.h"
#include "diag.h"
#include "math.h"
#include "dec.h"
#include "enc.h"
#include "sat.h"
#include "tv.h"
#include "ds.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#ifdef MPI
# include <mpi.h>
#endif

extern int proc_id;
extern int proc_count;

static ltms tms = NULL;

static unsigned int option_tries;
static unsigned int option_greediness;
static int option_sat;
static int option_trivial;

static cones_context cones;

static void invert_node(safari_node node)
{
    register unsigned int ix;

    safari_literal lit;

    for (ix = 0; ix < node->assignments->sz; ix++) {
        lit = node->assignments->arr[ix];
        lit->val = !lit->val;
    }
}

static void diagnosis_visitor(void *ctx, void *node_ctx)
{
    array diagnoses = (array)ctx;
    safari_node diagnosis = (safari_node)node_ctx;

    array_append(diagnoses, diagnosis);
}

static int cmp_nodes(const void *itema, const void *itemb)
{
    const safari_node a = *(const safari_node *)itema;
    const safari_node b = *(const safari_node *)itemb;

    return a->cardinality - b->cardinality;
}

static array get_node_label(const safari_node node,
                            const unsigned int max_h_domain)
{
    register unsigned int ix;

    array result = array_new(NULL, NULL);
    if (NULL == result) {
        return NULL;
    }

    for (ix = 0; ix < node->assignments->sz; ix++) {
        safari_literal lit = (safari_literal)node->assignments->arr[ix];
        array_append(result, i2p(lit->var * max_h_domain + lit->val));
    }

    array_int_sort(result);

    return result;
}

static safari_node allhealthy_candidate(diagnostic_problem problem)
{
    register unsigned int ix;

    safari_node result = safari_node_new();

    for (ix = 0; ix < problem->tv_cache->assumables; ix++) {
        if (!(problem->tv_cache->health_states[ix] & 1)) {
            array_append(result->assignments,
                         safari_literal_new(ix, SAFARI_LITERAL_TRUE));
            continue;
        }
        if (!(problem->tv_cache->health_states[ix] & 2)) {
            array_append(result->assignments,
                         safari_literal_new(ix, SAFARI_LITERAL_FALSE));
        }
    }

    return result;
}

static safari_node allfaulty_candidate(diagnostic_problem problem)
{
    register unsigned int ix;

    safari_node result = safari_node_new();

    for (ix = 0; ix < problem->tv_cache->assumables; ix++) {
        if (!(problem->tv_cache->health_states[ix] & 1)) {
            array_append(result->assignments,
                         safari_literal_new(ix, SAFARI_LITERAL_FALSE));
            result->cardinality += 1;
            continue;
        }
        if (!(problem->tv_cache->health_states[ix] & 2)) {
            array_append(result->assignments,
                         safari_literal_new(ix, SAFARI_LITERAL_TRUE));
            result->cardinality += 1;
        }
    }

    return result;
}

static safari_node tv_initial_candidate(diagnostic_problem problem,
                                        const_tv_clause_list obs)
{
    tv_clause_list sat = new_tv_clause_list();

    unsigned int ix = obs->sz + problem->u.tv_cnf_sd->clauses->sz;

    safari_node result = safari_node_new();
    tv_term solution = NULL;

    signed char *buf;

    setroom_tv_clause_list(sat, ix);

    memcpy(sat->arr, obs->arr, sizeof(tv_clause) * obs->sz);
    memcpy(sat->arr + obs->sz,
           problem->u.tv_cnf_sd->clauses->arr,
           sizeof(tv_clause) * problem->u.tv_cnf_sd->clauses->sz);
    sat->sz = ix;

    if (NULL == (buf = malloc(sizeof(signed char) * problem->tv_cache->assumables))) {
        fre_tv_clause_list(sat);
        safari_node_free(result);
        return NULL;
    }
    memset(buf, 0, sizeof(signed char) * problem->tv_cache->assumables);

    if (!lsss_get_random_solution(sat,
                                  problem->u.tv_cnf_sd->variables->sz,
                                  &solution)) {
        fre_tv_clause_list(sat);
        safari_node_free(result);

        free(buf);

        return NULL;
    }
    assert(solution != NULL);
    for ( ; ix < sat->sz; ix++) {
        rfre_tv_clause(sat->arr[ix]);
    }

    for (ix = 0; ix < solution->neg->sz; ix++) {
        if (problem->tv_cache->is_assumable[solution->neg->arr[ix]]) {
            int var = problem->tv_cache->v_to_a[solution->neg->arr[ix]];
            buf[var] = 1;
            array_append(result->assignments,
                         safari_literal_new(var, SAFARI_LITERAL_FALSE));
            if (!(problem->tv_cache->health_states[var] & 1)) {
                result->cardinality += 1;
            }
        }
    }
    for (ix = 0; ix < solution->pos->sz; ix++) {
        if (problem->tv_cache->is_assumable[solution->pos->arr[ix]]) {
            int var = problem->tv_cache->v_to_a[solution->pos->arr[ix]];
            buf[var] = 1;
            array_append(result->assignments,
                         safari_literal_new(var, SAFARI_LITERAL_TRUE));
            if (!(problem->tv_cache->health_states[var] & 2)) {
                result->cardinality += 1;
            }
        }
    }
    for (ix = 0; ix < problem->tv_cache->assumables; ix++) {
        if (buf[ix]) {
            continue;
        }
        if (problem->tv_cache->health_states[ix] & 1) {
            array_append(result->assignments,
                         safari_literal_new(ix, SAFARI_LITERAL_FALSE));
            continue;
        }
        if (problem->tv_cache->health_states[ix] & 2) {
            array_append(result->assignments,
                         safari_literal_new(ix, SAFARI_LITERAL_TRUE));
        }
    }

    rfre_tv_term(solution);
    fre_tv_clause_list(sat);

    free(buf);

    return result;
}

static safari_node mv_initial_candidate(diagnostic_problem problem,
                                        const_tv_clause_list obs)
{
    tv_clause_list sat = new_tv_clause_list();

    unsigned int ix = obs->sz + problem->u.tv_cnf_sd->clauses->sz;

    safari_node result = safari_node_new();

    tv_term solution = NULL;
    mv_term decoded_solution;

    mv_literal lit;

    int var;
    int val;

    signed char *buf;

    setroom_tv_clause_list(sat, ix);

    memcpy(sat->arr, obs->arr, sizeof(tv_clause) * obs->sz);
    memcpy(sat->arr + obs->sz,
           problem->u.tv_cnf_sd->clauses->arr,
           sizeof(tv_clause) * problem->u.tv_cnf_sd->clauses->sz);
    sat->sz = ix;

    if (NULL == (buf = malloc(sizeof(signed char) * problem->mv_cache->assumables))) {
        fre_tv_clause_list(sat);
        safari_node_free(result);
        return NULL;
    }
    memset(buf, 0, sizeof(signed char) * problem->mv_cache->assumables);

    if (!lsss_get_random_solution(sat,
                                  problem->u.tv_cnf_sd->variables->sz,
                                  &solution)) {
        fre_tv_clause_list(sat);
        safari_node_free(result);

        free(buf);

        return NULL;
    }
    assert(NULL != solution);
    for ( ; ix < sat->sz; ix++) {
        rfre_tv_clause(sat->arr[ix]);
    }

    decoded_solution = decode_term(solution,
                                   problem->variables,
                                   problem->encoded_variables,
                                   problem->domains,
                                   problem->encoding);
    assert(decoded_solution->neg->sz == 0);
    for (ix = decoded_solution->pos->sz - 1; ix < decoded_solution->pos->sz; ix--) {
        lit = decoded_solution->pos->arr[ix];
        if (!problem->mv_cache->is_assumable[lit->var]) {
            delete_mv_literal_list(decoded_solution->pos, ix);
        }
    }

    result->cardinality = 0;
    for (ix = 0; ix < decoded_solution->pos->sz; ix++) {
        lit = decoded_solution->pos->arr[ix];
        var = problem->mv_cache->v_to_a[lit->var];
        val = lit->val;
        if (!problem->mv_cache->health_states[var][val]) {
            result->cardinality += 1;
        }
        array_append(result->assignments, safari_literal_new(var, val));

        buf[var] = 1;
    }

    for (ix = 0; ix < problem->mv_cache->assumables; ix++) {
        if (!buf[ix]) {
            safari_literal lit;
            lit = safari_literal_new(ix, problem->mv_cache->p_max_idx[ix]);
            array_append(result->assignments, lit);
            if (!problem->mv_cache->health_states[lit->var][lit->val]) {
                result->cardinality += 1;
            }
        }
    }

    rfre_mv_term(decoded_solution);
    rfre_tv_term(solution);
    fre_tv_clause_list(sat);

    free(buf);

    return result;
}

static unsigned int tv_improve_diagnosis(diagnostic_problem problem,
                                         safari_node node,
                                         unsigned int flip)
{
    register unsigned int ix, iy;

    safari_literal lit;

    unsigned int *tmp;

    if (NULL == (tmp = malloc(sizeof(unsigned int) * problem->tv_cache->assumables))) {
        return (unsigned int)-1;
    }
    memset(tmp, 0, sizeof(unsigned int) * problem->tv_cache->assumables);

    for (ix = 0; ix < node->assignments->sz; ix++) {
        lit = (safari_literal)node->assignments->arr[ix];

        if (lit->val == SAFARI_LITERAL_FALSE) {
            if (!(problem->tv_cache->health_states[lit->var] & 1)) {
                tmp[lit->var] = ix + 1;
            }
        } else if (lit->val == SAFARI_LITERAL_TRUE) {
            if (!(problem->tv_cache->health_states[lit->var] & 2)) {
                tmp[lit->var] = ix + 1;
            }
        } else {
            assert(0);
            abort();
        }
    }

    for (ix = iy = 0; ix < problem->tv_cache->assumables; ix++) {
        if (0 == tmp[ix]) {
            continue;
        }
        if (iy == flip) {
            break;
        }
        iy += 1;
    }
    assert(ix < problem->tv_cache->assumables);

    assert(tmp[ix] > 0);
    ix = tmp[ix] - 1;

    free(tmp);

    return ix;
}

static unsigned int tv_improve_conflict(diagnostic_problem problem,
                                        safari_node node,
                                        unsigned int flip)
{
    register unsigned int ix, iy;

    safari_literal lit;

    unsigned int *tmp;

    if (NULL == (tmp = malloc(sizeof(unsigned int) * problem->tv_cache->assumables))) {
        return (unsigned int)-1;
    }
    memset(tmp, 0, sizeof(unsigned int) * problem->tv_cache->assumables);

    for (ix = 0; ix < node->assignments->sz; ix++) {
        lit = (safari_literal)node->assignments->arr[ix];

        if (lit->val == SAFARI_LITERAL_TRUE) {
            if (!(problem->tv_cache->health_states[lit->var] & 1)) {
                tmp[lit->var] = ix + 1;
            }
        } else if (lit->val == SAFARI_LITERAL_FALSE) {
            if (!(problem->tv_cache->health_states[lit->var] & 2)) {
                tmp[lit->var] = ix + 1;
            }
        } else {
            assert(0);
            abort();
        }
    }

    for (ix = iy = 0; ix < problem->tv_cache->assumables; ix++) {
        if (0 == tmp[ix]) {
            continue;
        }
        if (iy == flip) {
            break;
        }
        iy += 1;
    }
    assert(ix < problem->tv_cache->assumables);

    assert(tmp[ix] > 0);
    ix = tmp[ix] - 1;

    free(tmp);

    return ix;
}

static unsigned int mv_improve_diagnosis(diagnostic_problem problem,
                                         safari_node node,
                                         unsigned int flip)
{
    register unsigned int ix, iy;

    safari_literal lit;

    unsigned int *tmp;

    if (NULL == (tmp = malloc(sizeof(unsigned int) * problem->mv_cache->assumables))) {
        return (unsigned int)-1;
    }
    memset(tmp, 0, sizeof(unsigned int) * problem->mv_cache->assumables);

    for (ix = 0; ix < node->assignments->sz; ix++) {
        lit = (safari_literal)node->assignments->arr[ix];

        if (!problem->mv_cache->health_states[lit->var][lit->val]) {
            tmp[lit->var] = ix + 1;
        }
    }

    for (ix = iy = 0; ix < problem->mv_cache->assumables; ix++) {
        if (0 == tmp[ix]) {
            continue;
        }
        if (iy == flip) {
            break;
        }
        iy += 1;
    }
    assert(ix < problem->mv_cache->assumables);

    assert(tmp[ix] > 0);
    ix = tmp[ix] - 1;

    free(tmp);

    return ix;
}

static tv_term safari_node_to_tv_term(diagnostic_problem problem,
                                      const safari_node node)
{
    register unsigned int ix;

    tv_term result;

    safari_literal lit;


    result = new_tv_term(new_int_list(), new_int_list());
    if (NULL == result) {
        return NULL;
    }
    for (ix = 0; ix < node->assignments->sz; ix++) {
        lit = (safari_literal)node->assignments->arr[ix];
        if (lit->val == SAFARI_LITERAL_FALSE) {
            append_int_list(result->neg, problem->tv_cache->a_to_v[lit->var]);
        } else if (lit->val == SAFARI_LITERAL_TRUE) {
            append_int_list(result->pos, problem->tv_cache->a_to_v[lit->var]);
        } else {
            assert(0);
            abort();
        }
    }
    return result;
}

static mv_term safari_node_to_mv_term(diagnostic_problem problem,
                                      const safari_node node)
{
    register unsigned int ix;

    safari_literal lit_i;
    mv_literal lit_o;

    mv_term result;

    result = new_mv_term(new_mv_literal_list(), new_mv_literal_list());
    if (NULL == result) {
        return NULL;
    }

    for (ix = 0; ix < node->assignments->sz; ix++) {
        lit_i = (safari_literal)node->assignments->arr[ix];
        lit_o = new_mv_literal(problem->mv_cache->a_to_v[lit_i->var],
                               lit_i->val);
        append_mv_literal_list(result->pos, lit_o);
    }

    return result;
}

int safari_tv_diagnoses(FILE *outfile,
                        diagnostic_problem problem,
                        const_tv_term obs,
                        const unsigned int option_runs,
                        const unsigned int option_greediness,
                        const int option_sat,
                        const int option_trivial)
{
    unsigned int old_diagnoses;

    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;
    register int success;
    unsigned int greediness;

    safari_node candidate=NULL;
    tv_clause_list alpha;
    rand_nr_ctx rnr;
    tv_term term;
    trie result;

    array diagnoses;
    array nodes;
    array sig;

    unsigned int flip;

    int terminate = 0;
    int diagnosis_counter=0;
    int rc = 1;

    greediness = option_greediness == 0 ? 
                 problem->tv_cache->assumables : 
                 option_greediness;

    alpha = term_to_tv_clause_list(obs);

    result = trie_new((trie_node_destroy_func_t)safari_node_free,
                      (trie_node_clone_func_t)safari_node_copy);

    nodes = array_new(NULL, NULL);

    for (ix = 0; ix < option_runs && !is_timeout() && (get_max_diagnoses()<0 || diagnosis_counter<get_max_diagnoses()); ix++) {

        candidate = NULL;

        if ((int)ix % proc_count == proc_id) {
            candidate = option_trivial ?
                allfaulty_candidate(problem) :
                tv_initial_candidate(problem, alpha);
            if (NULL == candidate) {
                break;
            }
            if (!safari_enable_health_assumptions(tms,
                                                  problem->tv_cache,
                                                  candidate->assignments)) {
                rc = 0;
                assert(0);
                abort();
                break;
            }
            do {
                if (0 == candidate->cardinality) {
                    break;
                }

                success = 0;

                rnr = rand_nr_start(candidate->cardinality);
                for (iy = 0; iy < greediness && !is_timeout(); iy++) {
                    safari_literal lit = NULL;
                    if ((unsigned int)-1 == (iz = rand_nr(rnr))) {
                        break;
                    }

                    flip = tv_improve_diagnosis(problem, candidate, iz);
                    if ((unsigned int)-1 == flip) {
                        break;
                    }
                    lit = candidate->assignments->arr[flip];

                    ltms_retract_assumption(tms,
                                            problem->tv_cache->a_to_v[lit->var]);
                    ltms_enable_assumption(tms,
                                           problem->tv_cache->a_to_v[lit->var],
                                           !lit->val);

                    if (!ltms_has_contradiction(tms)) {
                        if (option_sat != TAGsat_none) {
                            lit->val = !lit->val;
                            candidate->cardinality -= 1;

                            term = safari_node_to_tv_term(problem, candidate);
                            if (safari_is_sat(problem->u.tv_cnf_sd,
                                              alpha,
                                              term,
                                              option_sat == TAGsat_lydia ? 
                                                  SAT_SOLVER_LSSS :
                                                  SAT_SOLVER_MINISAT)) {
                                success = 1;

                                rfre_tv_term(term);
                
                                break;
                            }
                            rfre_tv_term(term);

                            lit->val = !lit->val;
                            candidate->cardinality += 1;
                        } else {
                            success = 1;

                            lit->val = !lit->val;
                            candidate->cardinality -= 1;
                
                            break;
                        }
                    }
                    ltms_retract_assumption(tms,
                                            problem->tv_cache->a_to_v[lit->var]);
                    ltms_enable_assumption(tms,
                                           problem->tv_cache->a_to_v[lit->var],
                                           lit->val);
                }
                rand_nr_end(rnr);
            } while (success);

            safari_retract_health_assumptions(tms,
                                              problem->tv_cache,
                                              candidate->assignments);
        }

        array_append(nodes, candidate);
    }

    for (ix = 0; (ix < option_runs) && (ix<nodes->sz); ix++) {
        candidate = nodes->arr[ix];

#ifdef MPI
        candidate = safari_node_mpi_bcast(candidate, ix % proc_count);
#endif

        if (NULL != candidate) {
            sig = get_node_label(candidate, 2);
            if (!trie_is_subsumed(result, sig)) {
                trie_remove_subsumed(result, sig);
                trie_add(result, sig, candidate);
            } else {
                safari_node_free(candidate);
            }
            array_free(sig);
        }
    }

    array_free(nodes);

    rfre_tv_clause_list(alpha);

    diagnoses = array_new(NULL, NULL);
    trie_nodes_visit(result, diagnosis_visitor, diagnoses);

    lydia_qsort(diagnoses->arr,
                diagnoses->sz,
                sizeof(diagnoses->arr[0]),
                cmp_nodes);
    for (ix = 0; ix < diagnoses->sz && !terminate; ix++) {
        safari_node node = (safari_node)diagnoses->arr[ix];
        candidate_found(node->cardinality);
        if (stop_diagnoses()) {
            break;
        }
        term = safari_node_to_tv_term(problem, node);

        old_diagnoses = problem->diagnoses->sz;
        add_diagnosis_from_tv_term(problem,
                                   term,
                                   &terminate,
                                   1);
        if (cones != NULL && problem->diagnoses->sz - 1 == old_diagnoses) {
            assert(problem->diagnoses->sz > 0);
/* @todo: Fix the termination here. */
            expand_cone_random(cones,
                               obs,
                               problem->diagnoses->arr[problem->diagnoses->sz - 1]);
        }

        rfre_tv_term(term);
    }
    array_free(diagnoses);

    trie_free(result);

    return rc;
}

static signed char is_single_nominal(diagnostic_problem problem)
{
    register int var;

    for (var = 0; var < (int)problem->mv_cache->assumables; var++) {
        if (problem->mv_cache->num_nominal[var] > 1) {
            return 0;
        }
    }

    return 1;
}

static safari_node make_nominal_node(diagnostic_problem problem)
{
    safari_literal lit;
    safari_node node;

    unsigned int num_nominal;

    int var;
    int val;

    node = safari_node_new();

    for (var = 0; var < (int)problem->mv_cache->assumables; var++) {
        num_nominal = problem->mv_cache->num_nominal[var];
        val = problem->mv_cache->nominal[var][(int)(rand() / (((double)RAND_MAX + 1) / num_nominal))];

        lit = safari_literal_new(var, val);

        array_append(node->assignments, lit);
    }

    return node;
}

static signed char check_nominal(diagnostic_problem problem,
                                 const tv_clause_list alpha,
                                 const int solver,
                                 trie diagnoses)
{
    safari_node node;

    mv_term term;
    tv_term encoded_term;

    signed char result;

    array sig;

    node = make_nominal_node(problem);

    term = safari_node_to_mv_term(problem, node);
    encoded_term = encode_term(term,
                               problem->variables,
                               problem->encoded_variables,
                               problem->domains,
                               problem->encoding);
    result = safari_is_sat(problem->u.tv_cnf_sd,
                           alpha,
                           encoded_term,
                           solver);

    rfre_tv_term(encoded_term);
    rfre_mv_term(term);

    if (result) {
        sig = get_node_label(node,
                             problem->mv_cache->max_h_domain);
        if (!trie_is_subsumed(diagnoses, sig)) {
            trie_remove_subsumed(diagnoses, sig);
            trie_add(diagnoses, sig, node);
        } else {
            safari_node_free(node);
        }
        array_free(sig);
    } else {
        safari_node_free(node);
    }

    return result;
}

static signed char check_single_faults(diagnostic_problem problem,
                                       const tv_clause_list alpha,
                                       const int solver,
                                       trie diagnoses)
{
    register int var;
    register int val;

    unsigned int num_faulty;

    safari_literal lit;
    safari_node node;

    mv_term term;
    tv_term encoded_term;

    signed char consistent;
    signed char result = 0;

    array sig;

    for (var = 0; var < (int)problem->mv_cache->assumables; var++) {
        num_faulty = problem->mv_cache->num_faulty[var];

        for (val = 0; val < (int)num_faulty; val++) {
            node = make_nominal_node(problem);

            lit = node->assignments->arr[var];
            lit->val = problem->mv_cache->faulty[var][val];

            assert(lit->var == var);

            term = safari_node_to_mv_term(problem, node);
            encoded_term = encode_term(term,
                                       problem->variables,
                                       problem->encoded_variables,
                                       problem->domains,
                                       problem->encoding);
            consistent = safari_is_sat(problem->u.tv_cnf_sd,
                                       alpha,
                                       encoded_term,
                                       solver);
            result |= consistent;

            rfre_tv_term(encoded_term);
            rfre_mv_term(term);

            if (consistent) {
                sig = get_node_label(node,
                                     problem->mv_cache->max_h_domain);
                if (!trie_is_subsumed(diagnoses, sig)) {
                    trie_remove_subsumed(diagnoses, sig);
                    trie_add(diagnoses, sig, node);
                } else {
                    safari_node_free(node);
                }
                array_free(sig);
            } else {
                safari_node_free(node);
            }
        }
    }

    return result;
}

int safari_mv_diagnoses(diagnostic_problem problem,
                        const_tv_term obs,
                        const unsigned int option_tries,
                        const unsigned int option_greediness,
                        const int option_sat,
                        const int UNUSED(option_trivial))
{
    register unsigned int id;
    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;
    register int success;

    unsigned int greediness;
    unsigned int old;
    unsigned int num_nominal;

    safari_node candidate;
    safari_node node;
    safari_literal lit;
    tv_clause_list alpha;
    rand_nr_ctx rnr_comps;
    rand_nr_ctx rnr_domain;
    tv_term encoded_term;
    mv_term term;
    trie result;

    array diagnoses;
    array sig;

    unsigned int flip;

    int terminate = 0;

    int rc = 1;

    signed char single_nominal;

    single_nominal = is_single_nominal(problem);

    greediness = option_greediness == 0 ? 
                     problem->tv_cache->assumables : 
                     option_greediness;

    alpha = term_to_tv_clause_list(obs);

    result = trie_new((trie_node_destroy_func_t)safari_node_free,
                      (trie_node_clone_func_t)safari_node_copy);

    if (single_nominal && check_nominal(problem,
                                        alpha,
                                        option_sat == TAGsat_lydia ? 
                                            SAT_SOLVER_LSSS :
                                            SAT_SOLVER_MINISAT,
                                        result)) {
        goto terminate;
    }
    if (single_nominal && check_single_faults(problem,
                                              alpha,
                                              option_sat == TAGsat_lydia ? 
                                                  SAT_SOLVER_LSSS :
                                                  SAT_SOLVER_MINISAT,
                                              result)) {
        goto terminate;
    }

    for (ix = 0; ix < option_tries; ix++) {
        candidate = mv_initial_candidate(problem, alpha);
        if (NULL == candidate) {
            break;
        }

        do {
            if (0 == candidate->cardinality) {
                break;
            }

            success = 0;

            rnr_comps = rand_nr_start(candidate->cardinality);
            for (iy = 0; iy < greediness; iy++) {
                lit = NULL;
                if ((unsigned int)-1 == (iz = rand_nr(rnr_comps))) {
                    break;
                }

                flip = mv_improve_diagnosis(problem, candidate, iz);
                if ((unsigned int)-1 == flip) {
                    break;
                }
                lit = candidate->assignments->arr[flip];

                num_nominal = problem->mv_cache->num_nominal[lit->var];

                rnr_domain = rand_nr_start(num_nominal);
                for (id = 0; id < num_nominal; id++) {
                    old = lit->val;

                    lit->val = problem->mv_cache->nominal[lit->var][rand_nr(rnr_domain)];
                    candidate->cardinality -= 1;

                    term = safari_node_to_mv_term(problem, candidate);
                    encoded_term = encode_term(term,
                                               problem->variables,
                                               problem->encoded_variables,
                                               problem->domains,
                                               problem->encoding);
                    if (safari_is_sat(problem->u.tv_cnf_sd,
                                      alpha,
                                      encoded_term,
                                      option_sat == TAGsat_lydia ? 
                                          SAT_SOLVER_LSSS :
                                          SAT_SOLVER_MINISAT)) {
                        success = 1;

                        rfre_tv_term(encoded_term);
                        rfre_mv_term(term);
                    
                        break;
                    }

                    rfre_tv_term(encoded_term);
                    rfre_mv_term(term);

                    lit->val = old;
                    candidate->cardinality += 1;
                }
                rand_nr_end(rnr_domain);
                if (success) {
                    break;
                }
            }
            rand_nr_end(rnr_comps);
        } while (success);

        sig = get_node_label(candidate,
                             problem->mv_cache->max_h_domain);
        if (!trie_is_subsumed(result, sig)) {
            trie_remove_subsumed(result, sig);
            trie_add(result, sig, candidate);
        } else {
            safari_node_free(candidate);
        }
        array_free(sig);
    }
terminate:

    rfre_tv_clause_list(alpha);

    diagnoses = array_new(NULL, NULL);
    trie_nodes_visit(result, diagnosis_visitor, diagnoses);
    lydia_qsort(diagnoses->arr,
                diagnoses->sz,
                sizeof(diagnoses->arr[0]),
                cmp_nodes);
    for (ix = 0; ix < diagnoses->sz && !terminate; ix++) {
        node = (safari_node)diagnoses->arr[ix];
        candidate_found(node->cardinality);
        if (is_terminate()) {
            break;
        }
        term = safari_node_to_mv_term(problem, node);
/*
        pp_mv_term(stderr,
                   problem->encoded_variables,
                   problem->domains,
                   term);
        fprintf(stderr, "\n-\n");
*/

        add_diagnosis_from_mv_term(problem,
                                   term,
                                   &terminate);
        rfre_mv_term(term);
    }
    array_free(diagnoses);
    trie_free(result);

    return rc;
}

int safari_diag(FILE *outfile, diagnostic_problem problem, const_tv_term alpha)
{
    int rc = 1;

    diagnostic_problem_reset(problem);

/* Start the timer. */
    start_stopwatch("search time");
    if (safari_enable_observation_assumptions(tms, alpha)) {
        if (problem->encoding == ENCODING_NONE) {
            rc = safari_tv_diagnoses(outfile,
                                     problem,
                                     alpha,
                                     option_tries,
                                     option_greediness,
                                     option_sat,
                                     option_trivial);
        } else {
            rc = safari_mv_diagnoses(problem,
                                     alpha,
                                     option_tries,
                                     option_greediness,
                                     option_sat,
                                     option_trivial);
        }
    }
    safari_retract_observation_assumptions(tms, alpha);

/* Stop the timer. */
    stop_stopwatch("search time");

    return 1;
}

int safari_init(diagnostic_problem problem,
                const unsigned int tries,
                const unsigned int greediness,
                const int sat,
                const int trivial)
{
    register unsigned int ix;

    option_tries = tries;
    option_greediness = greediness;
    option_sat = sat;
    option_trivial = trivial;

    stat_init();

    init_int_counter("dpll_calls", "%d calls", "DPLL");
    init_int_counter("dpll_unsat", "UNSAT: %d", "DPLL");
    init_int_counter("ltms_calls", "%d calls", "LTMS");
    init_int_counter("ltms_unsat", "UNSAT: %d", "LTMS");
    init_stopwatch("search time", "search time: %d s %03d.%03d ms", "dynamics");

    assert(problem->model->tag == TAGtv_cnf_flat_kb ||
           problem->model->tag == TAGmv_cnf_flat_kb);

/* Now go, initialize the LTMS. */
    if (NULL == (tms = ltms_new())) {
        return 0;
    }

    for (ix = 0; ix < problem->u.tv_cnf_sd->variables->sz; ix++) {
        ltms_add_node(tms,
                      ix,
                      is_health(problem->u.tv_cnf_sd->variables->arr[ix]),
                      is_observable(problem->u.tv_cnf_sd->variables->arr[ix]));
    }
    for (ix = 0; ix < problem->u.tv_cnf_sd->clauses->sz; ix++) {
        ltms_add_clause(tms, problem->u.tv_cnf_sd->clauses->arr[ix]);
    }

    return 1;
}

void safari_destroy()
{
    if (NULL != tms) {
        ltms_free(tms);
    }

    stat_destroy();
}

int safari_tv_conflicts(diagnostic_problem problem,
                        const_tv_term obs,
                        const unsigned int option_tries,
                        const unsigned int option_greediness,
                        const int option_sat)
{
    register unsigned int ix, iy, iz;
    register int success;
    unsigned int greediness = option_greediness == 0 ? 
                              problem->tv_cache->assumables : 
                              option_greediness;

    tv_clause_list alpha;
    rand_nr_ctx rnr;
    tv_term term;
    trie result;

    array conflicts;
    array sig;

    unsigned int flip;

    int terminate = 0;

    int rc = 1;

    safari_node candidate;

    alpha = term_to_tv_clause_list(obs);

    result = trie_new((trie_node_destroy_func_t)safari_node_free,
                      (trie_node_clone_func_t)safari_node_copy);

    for (ix = 0; ix < option_tries; ix++) {
        candidate = allhealthy_candidate(problem);
        if (NULL == candidate) {
            break;
        }
        if (safari_enable_health_assumptions(tms,
                                             problem->tv_cache,
                                             candidate->assignments)) {
            rc = 0;
            assert(0);
            abort();
            break;
        }
        do {
            if (problem->tv_cache->assumables == candidate->cardinality) {
                break;
            }
            success = 0;

            rnr = rand_nr_start(problem->tv_cache->assumables - candidate->cardinality);
            for (iy = 0; iy < greediness; iy++) {
                safari_literal lit = NULL;
                if ((unsigned int)-1 == (iz = rand_nr(rnr))) {
                    break;
                }

                flip = tv_improve_conflict(problem, candidate, iz);
                if ((unsigned int)-1 == flip) {
                    break;
                }
                lit = candidate->assignments->arr[flip];

                ltms_retract_assumption(tms,
                                        problem->tv_cache->a_to_v[lit->var]);
                ltms_enable_assumption(tms,
                                       problem->tv_cache->a_to_v[lit->var],
                                       !lit->val);

                if (ltms_has_contradiction(tms)) {
                    if (option_sat != TAGsat_none) {
                        lit->val = !lit->val;
                        candidate->cardinality += 1;

                        term = safari_node_to_tv_term(problem, candidate);
                        if (!safari_is_sat(problem->u.tv_cnf_sd,
                                           alpha,
                                           term,
                                           option_sat == TAGsat_lydia ? 
                                               SAT_SOLVER_LSSS :
                                               SAT_SOLVER_MINISAT)) {
                            success = 1;

                            rfre_tv_term(term);
                    
                            break;
                        }
                        rfre_tv_term(term);

                        lit->val = !lit->val;
                        candidate->cardinality -= 1;
                    } else {
                        success = 1;

                        lit->val = !lit->val;
                        candidate->cardinality += 1;
                    
                        break;
                    }
                }
                ltms_retract_assumption(tms,
                                        problem->tv_cache->a_to_v[lit->var]);
                ltms_enable_assumption(tms,
                                       problem->tv_cache->a_to_v[lit->var],
                                       lit->val);
            }
            rand_nr_end(rnr);
        } while (success);

        safari_retract_health_assumptions(tms,
                                          problem->tv_cache,
                                          candidate->assignments);
        invert_node(candidate);

        sig = get_node_label(candidate, 2);
        if (!trie_is_subsumed(result, sig)) {
            trie_remove_subsumed(result, sig);
            trie_add(result, sig, candidate);
        } else {
            safari_node_free(candidate);
        }
        array_free(sig);
    }

    rfre_tv_clause_list(alpha);

    conflicts = array_new(NULL, NULL);
    trie_nodes_visit(result, diagnosis_visitor, conflicts);
    lydia_qsort(conflicts->arr,
                conflicts->sz,
                sizeof(conflicts->arr[0]),
                cmp_nodes);
    for (ix = 0; ix < conflicts->sz && !terminate; ix++) {
        safari_node node = (safari_node)conflicts->arr[ix];
        candidate_found(node->cardinality);
        if (is_terminate()) {
            break;
        }
        term = safari_node_to_tv_term(problem, node);

        add_diagnosis_from_tv_term(problem,
                                   term,
                                   &terminate,
                                   1);

        rfre_tv_term(term);
    }
    array_free(conflicts);

    trie_free(result);

    return rc;
}

int safari_conflicts(diagnostic_problem problem, const_tv_term alpha)
{
    int rc = 1;


    diagnostic_problem_reset(problem);

/* Start the timer. */
    start_stopwatch("search time");
    if (safari_enable_observation_assumptions(tms, alpha)) {
        if (problem->encoding == ENCODING_NONE) {
            rc = safari_tv_conflicts(problem,
                                     alpha,
                                     option_tries,
                                     option_greediness,
                                     option_sat);
        } else {
            assert(0);
            abort();
        }
    }
    safari_retract_observation_assumptions(tms, alpha);

/* Stop the timer. */
    stop_stopwatch("search time");

    return 1;
}

void safari_set_cones(cones_context context)
{
    cones = context;
}
