#include "variable.h"
#include "array.h"
#include "stack.h"
#include "qsort.h"
#include "defs.h"
#include "diag.h"
#include "stat.h"
#include "dec.h"
#include "obs.h"

#include <assert.h>
#include <math.h>
#ifndef WIN32
# include <sys/time.h>
#endif

static hash_table *diagnostic_problems = NULL;
static array diagnostic_problem_names = NULL;

static signed char option_all_diagnoses = 0;

static double max_time = -1;

static double start_time;

static double get_time()
{
#ifdef WIN32
    return 0.0; /* To Do: Implement this. */
#else
    struct timeval tv;
    int result = gettimeofday(&tv, 0);
    assert(result == 0);

    return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}

void diagnostic_problems_list(FILE *outfile,
                              list_item_printer_func_t pp_list_item)
{
    register unsigned int ix;

    if (NULL == diagnostic_problem_names) {
        return;
    }

    for (ix = 0; ix < diagnostic_problem_names->sz; ix++) {
        pp_list_item(outfile,
                     ((lydia_symbol)diagnostic_problem_names->arr[ix])->name);
    }
}

void observations_list(FILE *outfile,
                       const diagnostic_problem dp,
                       list_item_printer_func_t pp_list_item)
{
    register unsigned int ix;

    for (ix = 0; ix < dp->alphas->names->sz; ix++) {
        pp_list_item(outfile,
                     ((lydia_symbol)dp->alphas->names->arr[ix])->name);
    }
}

void diagnostic_problems_free()
{
    if (NULL != diagnostic_problems) {
        hash_destroy(diagnostic_problems);
        free(diagnostic_problems);
    }

    if (NULL != diagnostic_problem_names) {
        array_free(diagnostic_problem_names);
    }
}

static void free_diagnostic_problem(void *bucket)
{
    diagnostic_problem dp = *(diagnostic_problem *)bucket;

    if (NULL != dp->alphas) {
        observations_free(dp->alphas);
    }

    diagnostic_problem_free(dp);
}

static unsigned long name_hash_function(const char *key,
                                        unsigned int UNUSED(key_length))
{
	unsigned long h = 5381;

    const char *start = key;

	for (; *start != '\0'; start++) {
		h += (h << 5);
		h ^= (unsigned long)*start;
	}

	return h;
}

int diagnostic_problem_add(const char *name,
                           diagnostic_problem dp,
                           observations alphas)
{
    assert(NULL != dp);

    dp->alphas = alphas;

    if (NULL == diagnostic_problems) {
        diagnostic_problems = (hash_table *)malloc(sizeof(hash_table));
        if (NULL == diagnostic_problems) {
            return 0;
        }

        hash_init(diagnostic_problems, 2, name_hash_function, free_diagnostic_problem);

        diagnostic_problem_names = array_new(NULL, NULL);
    }

    hash_add(diagnostic_problems,
             name,
             strlen(name),
             (char *)&dp,
             sizeof(diagnostic_problem),
             NULL);

    array_append(diagnostic_problem_names, add_lydia_symbol(name));

    return 1;
}

diagnostic_problem diagnostic_problem_get(const char *name)
{
    void *pdata;

    if (NULL == diagnostic_problems ||
        hash_find(diagnostic_problems,
                  name,
                  strlen(name),
                  &pdata)) {
        return NULL;
    }

    return *(void **)pdata;
}

static array get_faultmode_label(const_faultmode fault,
                                 const_variable_list variables,
                                 unsigned int max_domain)
{
    register unsigned int ix;
    register unsigned int iy;
    register unsigned int var;
    register unsigned int val;

    variable_assignment assignment;

    array result;

    result = array_new(NULL, NULL);
    if (NULL == result) {
        return result;
    }

    for (ix = 0; ix < fault->assignments->sz; ix++) {
        assignment = fault->assignments->arr[ix];
        var = assignment->var;
        assert(is_health(variables->arr[var]));
        if (assignment->tag == TAGenum_variable_assignment) {
            assert(to_enum_variable_assignment(assignment)->value < (int)max_domain);
            val = to_enum_variable_assignment(assignment)->value;
            if (get_health_bool(variables->arr[var], val)) {
                continue;
            }
            iy = var * max_domain + val;
        } else if (assignment->tag == TAGbool_variable_assignment) {
            val = to_bool_variable_assignment(assignment)->value;
            if (get_health_bool(variables->arr[var], val)) {
                continue;
            }
            iy = var * max_domain + val;
        } else {
/* No health variable should be of infinite domain type. */
            assert(0);
            abort();
        }
        array_append(result, ui2p(iy));
    }

    return array_int_sort(result);
}

static const_variable_list variables = NULL;

static int cmp_assignment(const void *a, const void *b)
{
    const_variable_assignment sa = *(const variable_assignment *)a;
    const_variable_assignment sb = *(const variable_assignment *)b;

    return cmp_identifier(variables->arr[sa->var]->name,
                          variables->arr[sb->var]->name);
}

static void sort_assignments(variable_assignment_list al)
{
    if (al == variable_assignment_listNIL || al->sz < 2) {
        return;
    }
    lydia_qsort(al->arr, al->sz, sizeof(al->arr[0]), cmp_assignment);
}

faultmode tv_term_to_faultmode(const_tv_variables_cache tv_cache,
                               const_tv_term term,
                               unsigned int *unassigned,
                               const signed char sort)
{
    register unsigned int ix;

    variable_assignment assignment;

    faultmode result = new_faultmode(1, 0, new_variable_assignment_list());

    const_int_list pos = term->pos;
    const_int_list neg = term->neg;

    signed char *buffer;

    buffer = (signed char *)malloc(sizeof(signed char) * tv_cache->variables);
    if (NULL == buffer) {
        rfre_faultmode(result);
        return faultmodeNIL;
    }
    memset(buffer, 0, sizeof(signed char) * tv_cache->variables);

    for (ix = 0; ix < pos->sz; ix++) {
        if (!tv_cache->is_assumable[pos->arr[ix]]) {
            continue;
        }
        assignment = to_variable_assignment(new_bool_variable_assignment(pos->arr[ix], LYDIA_TRUE));
        append_variable_assignment_list(result->assignments, assignment);
        buffer[pos->arr[ix]] = 1;
    }
    for (ix = 0; ix < neg->sz; ix++) {
        if (!tv_cache->is_assumable[neg->arr[ix]]) {
            continue;
        }
        assignment = to_variable_assignment(new_bool_variable_assignment(neg->arr[ix], LYDIA_FALSE));
        append_variable_assignment_list(result->assignments, assignment);
        buffer[neg->arr[ix]] = -1;
    }
    if (NULL != unassigned) {
        *unassigned = 0;
    }
    for (ix = 0; ix < tv_cache->variables; ix++) {
        if (!tv_cache->is_assumable[ix]) {
            continue;
        }
        if (buffer[ix] == 1) {
            result->probability *= tv_cache->p_true[tv_cache->v_to_a[ix]];
            if (!(tv_cache->health_states[tv_cache->v_to_a[ix]] & 2)) {
                result->cardinality += 1;
            }
        } else if (buffer[ix] == -1) {
            result->probability *= tv_cache->p_false[tv_cache->v_to_a[ix]];
            if (!(tv_cache->health_states[tv_cache->v_to_a[ix]] & 1)) {
                result->cardinality += 1;
            }
        } else {
            assert(buffer[ix] == 0);

            result->probability *= tv_cache->p_max[tv_cache->v_to_a[ix]];

            if (NULL != unassigned) {
                *unassigned += 1;
            }
        }
    }

    free(buffer);

    variables = tv_cache->list;

    if (sort) {
        sort_assignments(result->assignments);
    }
    return result;
}

faultmode mv_term_to_faultmode(diagnostic_problem context, const_mv_term term)
{
    register unsigned int ix;

    faultmode result = new_faultmode(1, 0, new_variable_assignment_list());

    const_mv_literal_list neg = term->neg;
    const_mv_literal_list pos = term->pos;

    result->cardinality = 0;

    assert(neg->sz == 0);
    for (ix = 0; ix < pos->sz; ix++) {
        mv_literal lit = pos->arr[ix];

        if (!context->mv_cache->is_assumable[lit->var]) {
            continue;
        }
                        
        append_variable_assignment_list(result->assignments,
                                        to_variable_assignment(new_enum_variable_assignment(lit->var, lit->val)));
                        
        if (!context->mv_cache->health_states[context->mv_cache->v_to_a[lit->var]][lit->val]) {
            result->cardinality += 1;
            result->probability *= context->mv_cache->p[context->mv_cache->v_to_a[lit->var]][lit->val];
        }
    }
                
    return result;
}

int add_diagnosis(diagnostic_problem context,
                  const_faultmode fault,
                  int *terminate)
{
    array sig;

    if (context->encoding == ENCODING_NONE) {
        sig = get_faultmode_label(fault, context->variables, 2);

        if (NULL == sig) {
            return 0;
        }
    } else {
        sig = get_faultmode_label(fault,
                                  context->encoded_variables,
                                  context->mv_cache->max_h_domain);
        if (NULL == sig) {
            return 0;
        }
    }

    *terminate = 0;
    increase_int_counter("diagnoses");
    if (option_all_diagnoses ||
        !trie_is_subsumed(context->all_diagnoses, sig)) {

        if (!trie_add(context->all_diagnoses, sig, NULL)) {

            array_free(sig);

            return 0;
        }
        increase_int_counter("minimal_diagnoses");
        array_append(context->diagnoses, rdup_faultmode(fault));

        diagnosis_found(fault->cardinality);

        if (is_terminate()) {
            *terminate = 1;

            array_free(sig);

            return 1;
        }
    }
    array_free(sig);

    return 1;
}

int add_diagnosis_from_tv_term(diagnostic_problem context,
                               const_tv_term diagnosis,
                               int *terminate,
                               const signed char sort)
{
    mv_term decoded_term;

    faultmode fault;

    array sig;

    unsigned int unassigned;

    if (context->encoding == ENCODING_NONE) {
        fault = tv_term_to_faultmode(context->tv_cache,
                                     diagnosis,
                                     &unassigned,
                                     sort);
        sig = get_faultmode_label(fault, context->variables, 2);

        if (NULL == sig) {
            rfre_faultmode(fault);

            return 0;
        }
    } else {
        decoded_term = decode_term(diagnosis,
                                   context->variables,
                                   context->encoded_variables,
                                   context->domains,
                                   context->encoding);
        if (NULL == decoded_term) {
            return 0;
        }
        fault = mv_term_to_faultmode(context, decoded_term);
        rfre_mv_term(decoded_term);

        sig = get_faultmode_label(fault,
                                  context->encoded_variables,
                                  context->mv_cache->max_h_domain);
        if (NULL == sig) {
            rfre_faultmode(fault);

            return 0;
        }
    }

    *terminate = 0;
    increase_int_counter("diagnoses");
    if (option_all_diagnoses ||
        !trie_is_subsumed(context->all_diagnoses, sig)) {

        if (!trie_add(context->all_diagnoses, sig, NULL)) {

            array_free(sig);

            return 0;
        }
        increase_int_counter("minimal_diagnoses");
        array_append(context->diagnoses, fault);

        diagnosis_found(fault->cardinality);

        if (is_terminate()) {
            *terminate = 1;

            array_free(sig);

            return 1;
        }
    } else {
        rfre_faultmode(fault);
    }
    array_free(sig);

    return 1;
}

int add_diagnosis_from_mv_term(diagnostic_problem context,
                               const_mv_term diagnosis,
                               int *terminate)
{
    faultmode fault;

    array sig;

    fault = mv_term_to_faultmode(context, diagnosis);
    sig = get_faultmode_label(fault,
                              context->encoded_variables,
                              context->mv_cache->max_h_domain);
    if (NULL == sig) {
        rfre_faultmode(fault);
        return 0;
    }

    *terminate = 0;
    increase_int_counter("diagnoses");
    if (option_all_diagnoses ||
        !trie_is_subsumed(context->all_diagnoses, sig)) {

        if (!trie_add(context->all_diagnoses, sig, NULL)) {

            rfre_faultmode(fault);
            array_free(sig);

            return 0;
        }
        increase_int_counter("minimal_diagnoses");
        array_append(context->diagnoses, fault);

        diagnosis_found(fault->cardinality);
        if (is_terminate()) {
            *terminate = 1;

            array_free(sig);

            return 1;
        }
    } else {
        rfre_faultmode(fault);
    }
    array_free(sig);

    return 1;
}

int diagnostic_problem_reset(diagnostic_problem context)
{
    trie_free(context->all_diagnoses);
    array_free(context->diagnoses);

    if (NULL == (context->diagnoses = array_new((array_element_destroy_func_t)rfre_faultmode, NULL))) {
        free(context);
        return 0;
    }
    if (NULL == (context->all_diagnoses = trie_new(NULL, NULL))) {
        array_free(context->diagnoses);
        free(context);
        return 0;
    }

    return 1;
}

void diagnostic_problem_free(diagnostic_problem context)
{
    trie_free(context->all_diagnoses);
    array_free(context->diagnoses);

    if (NULL != context->tv_cache) {
        destroy_tv_variables_cache(context->tv_cache);
    }
    if (NULL != context->mv_cache) {
        destroy_mv_variables_cache(context->mv_cache);
    }

    free(context);
}

static int fm(FILE *outfile,
              diagnostic_problem context,
              fm_printer_func_t fm_printer)
{
    register unsigned int ix;

    if (NULL == fm_printer) {
        return 1;
    }

    for (ix = 0; ix < context->diagnoses->sz; ix++) {
        fm_printer(outfile, 
                   to_faultmode(context->diagnoses->arr[ix]),
                   ix + 1,
                   context->variables,
                   context->encoded_variables,
                   context->domains,
                   context->encoding);
    }

    return 1;
}

static int probs(FILE *outfile,
                 diagnostic_problem context,
                 prob_printer_func_t prob_printer)
{
    register unsigned int ix;

    double sum = 0;

    if (NULL == prob_printer) {
        return 1;
    }

    for (ix = 0; ix < context->diagnoses->sz; ix++) {
        sum += to_faultmode(context->diagnoses->arr[ix])->probability;
    }

    for (ix = 0; ix < context->diagnoses->sz; ix++) {
        to_faultmode(context->diagnoses->arr[ix])->probability /= sum;
        prob_printer(outfile, context->diagnoses->arr[ix], ix + 1);
    }

    return 1;
}

static int cards(FILE *outfile,
                 diagnostic_problem context,
                 card_printer_func_t card_printer)
{
    register unsigned int ix;

    if (NULL == card_printer) {
        return 1;
    }

    for (ix = 0; ix < context->diagnoses->sz; ix++) {
        card_printer(outfile, context->diagnoses->arr[ix], ix + 1);
    }

    return 1;
}

static int entropy(FILE *outfile,
                   diagnostic_problem context,
                   entropy_printer_func_t entropy_printer)
{
    register unsigned int ix;

    double sum = 0;
    double entropy = 0;
    double p;

    if (NULL == entropy_printer) {
        return 1;
    }

    for (ix = 0; ix < context->diagnoses->sz; ix++) {
        sum += to_faultmode(context->diagnoses->arr[ix])->probability;
    }

    for (ix = 0; ix < context->diagnoses->sz; ix++) {
        to_faultmode(context->diagnoses->arr[ix])->probability /= sum;
        p = to_faultmode(context->diagnoses->arr[ix])->probability;
        entropy += (p * (log(p) /* / log(2) */ ));
    }

    entropy_printer(outfile, entropy);

    return 1;
}

static int count(FILE *outfile,
                 diagnostic_problem context,
                 count_printer_func_t count_printer)
{
    if (NULL == count_printer) {
        return 1;
    }

    count_printer(outfile, context->diagnoses->sz);

    return 1;
}

diagnostic_problem diagnostic_problem_new(const_serializable model)
{
    diagnostic_problem context = malloc(sizeof(struct str_diagnostic_problem));
    if (NULL == context) {
        return NULL;
    }

    memset(context, 0, sizeof(struct str_diagnostic_problem));

    stat_init();

    init_int_counter("minimal_diagnoses", "minimal diagnoses: %d", "search tree");
    init_int_counter("diagnoses", "diagnoses: %d", "search tree");

    context->model = model;
    switch (model->tag) {
        case TAGhorn_flat_kb:
            assert(TAGhorn == to_horn_flat_kb(model)->constraints->tag);
            context->u.horn_sd = to_horn(to_horn_flat_kb(model)->constraints);

            context->variables = context->u.horn_sd->variables;
            context->encoded_variables = context->u.horn_sd->encoded_variables;
            context->domains = context->u.horn_sd->domains;
            context->encoding = context->u.horn_sd->encoding;

            context->name = to_horn_flat_kb(model)->name;
            break;
        case TAGtv_cnf_flat_kb:
            assert(TAGtv_cnf == to_tv_cnf_flat_kb(model)->constraints->tag);
            context->u.tv_cnf_sd = to_tv_cnf(to_tv_cnf_flat_kb(model)->constraints);

            context->variables = context->u.tv_cnf_sd->variables;
            context->encoded_variables = context->u.tv_cnf_sd->encoded_variables;
            context->domains = context->u.tv_cnf_sd->domains;
            context->encoding = context->u.tv_cnf_sd->encoding;

            context->name = to_tv_cnf_flat_kb(model)->name;
            break;
        case TAGtv_dnf_flat_kb:
            assert(TAGtv_dnf == to_tv_dnf_flat_kb(model)->constraints->tag);
            context->u.tv_dnf_sd = to_tv_dnf(to_tv_dnf_flat_kb(model)->constraints);

            context->variables = context->u.tv_dnf_sd->variables;
            context->encoded_variables = context->u.tv_dnf_sd->encoded_variables;
            context->domains = context->u.tv_dnf_sd->domains;
            context->encoding = context->u.tv_dnf_sd->encoding;

            context->name = to_tv_dnf_flat_kb(model)->name;
            break;
        case TAGtv_dnf_hierarchy:
            if (nodeNIL != (context->u.tv_tdnf_sd.root = find_root_node(to_hierarchy(model)))) {
                context->name = context->u.tv_tdnf_sd.root->type;
            }
            break;
        default:
            assert(0);
            abort();
    }
    switch (model->tag) {
        case TAGhorn_flat_kb:
        case TAGtv_cnf_flat_kb:
        case TAGtv_dnf_flat_kb:
            context->tv_cache = initialize_tv_variables_cache(context->variables);
            context->mv_cache = initialize_mv_variables_cache(context->encoded_variables,
                                                              context->domains);
            break;
        case TAGtv_dnf_hierarchy:
            break;
        default:
            assert(0);
            abort();
    }

    context->fm = fm;
    context->probs = probs;
    context->cards = cards;
    context->entropy = entropy;
    context->count = count;

    if (NULL == (context->diagnoses = array_new((array_element_destroy_func_t)rfre_faultmode, NULL))) {
        free(context);
        return 0;
    }
    if (NULL == (context->all_diagnoses = trie_new(NULL, NULL))) {
        array_free(context->diagnoses);
        free(context);
        return 0;
    }

    return context;
}

/* Termination: */
static unsigned int diagnoses = 0;
static unsigned int candidate_cardinality = (unsigned int)-1; /* unknown */
static unsigned int diagnosis_cardinality = (unsigned int)-1; /* unknown */
static unsigned int max_diagnoses = (unsigned int)-1;         /* unlimited */
static unsigned int max_cardinality = (unsigned int)-1;       /* unlimited */
static signed char option_terminate_mc = 0;

static terminate_func_t io_terminate = NULL;

void set_max_diagnoses(const int d)
{
    max_diagnoses = d;
}

void set_max_time(const double d)
{
    max_time = d;
}

void set_max_cardinality(const int c)
{
    max_cardinality = c;
}

int  get_max_diagnoses()
{
	return max_diagnoses;
}

void set_option_terminate_mc(const signed char fg)
{
    option_terminate_mc = fg;
}

void set_terminate(const terminate_func_t f)
{
    io_terminate = f;
}

void diagnosis_reset()
{
    diagnoses = 0;
    candidate_cardinality = (unsigned int)-1; /* unknown */
    diagnosis_cardinality = (unsigned int)-1;
    start_time = get_time();
}

void diagnosis_found(const int c)
{
    candidate_cardinality = c; /* diagnoses are also candidates */
    diagnosis_cardinality = c;

    diagnoses += 1;
}

void candidate_found(const int c)
{
    candidate_cardinality = c;
}

int is_terminate()
{
    if ((candidate_cardinality != (unsigned int)-1) &&
        (max_cardinality != (unsigned int)-1) &&
        (candidate_cardinality > max_cardinality)) {
        return 1;
    }

    if ((max_diagnoses != (unsigned int)-1) && (diagnoses >= max_diagnoses)) {
        return 1;
    }

    if (option_terminate_mc &&
        diagnosis_cardinality != (unsigned int)-1 &&
        candidate_cardinality > diagnosis_cardinality) {
        return 1;
    }

    if (io_terminate != NULL && io_terminate()) {
        return 1;
    }

    if ((max_time >= 0.0) && (get_time() >= start_time + max_time)) {
        return 1;
    }

    return 0;
}

int is_timeout()
{
    if ((max_time >= 0.0) && (get_time() >= start_time + max_time)) {
        return 1;
    }

    return 0;
}

int stop_diagnoses()
{
    if ((candidate_cardinality != (unsigned int)-1) &&
        (max_cardinality != (unsigned int)-1) &&
        (candidate_cardinality > max_cardinality)) {
        return 1;
    }

    if ((max_diagnoses != (unsigned int)-1) && (diagnoses >= max_diagnoses)) {
        return 1;
    }

    if (option_terminate_mc &&
        diagnosis_cardinality != (unsigned int)-1 &&
        candidate_cardinality > diagnosis_cardinality) {
        return 1;
    }

    return 0;
}
