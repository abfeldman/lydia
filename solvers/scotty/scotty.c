#include "card_sort_terms.h"
#include "sorted_int_list.h"
#include "config.h"
#include "scotty.h"
#include "stat.h"
#include "defs.h"
#include "diag.h"
#include "tv.h"

int scotty_diag(diagnostic_problem problem, const_tv_term alpha)
{
    register unsigned int ix;
    register unsigned int cardinality;

    tv_term term;

    int terminate = 0;

    diagnostic_problem_reset(problem);

    sort_int_list(alpha->neg);
    sort_int_list(alpha->pos);

    for (ix = 0; ix < problem->u.tv_dnf_sd->terms->sz && !terminate; ix++) {
        term = problem->u.tv_dnf_sd->terms->arr[ix];

        cardinality = get_tv_term_cardinality(problem->tv_cache, term);
        candidate_found(cardinality);
        if (is_terminate()) {
            break;
        }

        if (!are_sorted_tv_terms_consistent(alpha, term)) {
            continue;
        }

        add_diagnosis_from_tv_term(problem, term, &terminate, 1);
    }

    return 1;
}

int scotty_init(diagnostic_problem problem)
{
    if (!cardinality_sort_terms(problem->tv_cache,
                                problem->mv_cache,
                                problem->u.tv_dnf_sd->terms,
                                problem->u.tv_dnf_sd->encoding)) {
        return 0;
    }

    return 1;
}

int scotty_destroy()
{
    stat_destroy();

    return 1;
}
