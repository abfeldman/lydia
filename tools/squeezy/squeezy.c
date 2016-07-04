#include "transv.h"
#include "config.h"
#include "fprint.h"
#include "strdup.h"
#include "trie.h"
#include "util.h"
#include "pi.h"
#include "tv.h"

#include <assert.h>

static tv_nf squeeze(const_tv_nf input,
                     const signed char tison,
                     const signed char tries,
                     const signed char pionly,
                     const signed char greedy)
{
    int_list_list matrix;

    tv_nf output = tison ?
        (tries ? pi_tison_trie(input) : pi_tison(input)) :
        (pi_brute_force(input));

    if (!pionly) {
        matrix = new_adjacency_matrix(get_literal_sets(input),
                                      get_literal_sets(output));
        if (greedy) {
            rfre_tv_literal_set_list(set_literal_sets(output, greedy_era(matrix, get_literal_sets(output))));
        } else {
            era(matrix, get_literal_sets(output));
        }
        rfre_int_list_list(matrix);
    }

    return output;
}

void squeeze_nf(const_tv_nf input,
                tv_nf *output,
                const signed char tison,
                const signed char tries,
                const signed char pionly,
                const signed char greedy,
                const signed char verbose)
{
    if (input->tag == TAGtv_cnf) {
        if (verbose) {
            fprintf(stderr, "input clauses: %d\n", to_tv_cnf(input)->clauses->sz);
        }
        *output = squeeze(to_tv_nf(input), tison, tries, pionly, greedy);
        if (verbose) {
            fprintf(stderr, "%sprime implicates: %d\n", (pionly ? "" : "essential "), to_tv_cnf(*output)->clauses->sz);
        }
        if (verbose && !pionly && to_tv_cnf(*output)->clauses->sz > 0) {
            fprintf(stderr, "reduction: %.2f%%\n", (double)to_tv_cnf(input)->clauses->sz / (double)to_tv_cnf(*output)->clauses->sz * 100 - 100);
        }
    } else if (input->tag == TAGtv_dnf) {
        if (verbose) {
            fprintf(stderr, "input terms: %d\n", to_tv_dnf(input)->terms->sz);
        }
        *output = squeeze(to_tv_nf(input), tison, tries, pionly, greedy);
        if (verbose) {
            fprintf(stderr, "%sprime implicants: %d\n", (pionly ? "" : "essential "), to_tv_dnf(*output)->terms->sz);
        }
        if (verbose && !pionly && to_tv_dnf(*output)->terms->sz > 0) {
            fprintf(stderr, "reduction: %.2f%%\n", (double)to_tv_dnf(input)->terms->sz / (double)to_tv_dnf(*output)->terms->sz * 100 - 100);
        }
    } else {
        assert(0);
        abort();
    }
}
