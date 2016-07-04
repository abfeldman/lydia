#include "ast.h"
#include "util.h"
#include "defs.h"
#include "dump.h"
#include "attr.h"
#include "error.h"
#include "config.h"
#include "symtbl.h"
#include "rewrite.h"
#include "typetbl.h"
#include "typeinfer.h"

static int trace_lex = 0;
static int trace_yacc = 0;

extern model parse(FILE *, const char *, char **, unsigned int, int, int);

int lc(FILE *input,
       char *infilename,
       char **ipath,
       unsigned int ipathsz,
       const int option_unused_systems,
       csp_hierarchy *output)
{
    index_entry_list index_table;

    user_type_entry_list user_type_table;
    attribute_entry_list attribute_table;

    int rc = 1;

    model result = parse(input,
                         infilename,
                         ipath,
                         ipathsz,
                         trace_yacc,
                         trace_lex);
    if (result == modelNIL) {
        return 0;
    }

    user_type_table = new_user_type_entry_list();
    attribute_table = new_attribute_entry_list();

    rc &= build_user_type_table(result, user_type_table);

    dereference_aliases(user_type_table);
/**
 * We have a small bootstrapping problem here. First we want to
 * initialize the array counters at the same time in which we are
 * doing type inference, but we also want to allow expressions in the
 * array declarations. A solution is to allow limited number of
 * expressions - integers, constants and operations.
 */
    rc &= compute_array_sizes(result, user_type_table);

    rc &= build_attribute_table(result, user_type_table, attribute_table);
    rc &= build_symbol_tables(result, user_type_table, attribute_table);

    rc &= infer_types_model(result, user_type_table, attribute_table);
    if (!rc) {
        rfre_model(result);
        rfre_user_type_entry_list(user_type_table);
        rfre_attribute_entry_list(attribute_table);
        return 0;
    }

    index_table = new_index_entry_list();
    flatten_predicates(result, attribute_table, index_table, user_type_table);
    rfre_index_entry_list(index_table);
/* Only simple predicates in each system definition at this point. */

    rewrite_systems(result, user_type_table);

/*
 * Function calls and constants (they are the same thing, aren't they)
 * are inlined here.
 */
    *output = dump_model(result,
                         user_type_table,
                         attribute_table,
                         option_unused_systems,
                         &rc);

    rfre_user_type_entry_list(user_type_table);
    rfre_attribute_entry_list(attribute_table);
    rfre_model(result);

    return rc;
}
