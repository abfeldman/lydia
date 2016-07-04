#include "variable.h"
#include "strdup.h"
#include "config.h"
#include "lsss.h"
#include "util.h"
#include "tv.h"
#include "pp.h"

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define VERSION_MAJOR 1
#define VERSION_MINOR 1

static int findall = 0;

static FILE *infile;
static FILE *outfile;

static char *infilename = NULL;
static char *outfilename = NULL;

static void version(FILE *fp)
{
    fprintf(fp, "dimacs v. %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
}

static void usage( FILE *fp )
{
    fprintf(fp,
            "Usage: dimacs [OPTION] SOURCE DEST\n"
            "  or:  dimacs\n"
            "DIMACS problem solver.\n"
            "\n"
            "  -a                   find all solutions instead of just one\n"
            "  -V                   increase verbosity\n"
            "  -h                   display this help and exit\n"
            "  -v                   output version information and exit\n"
            "\n"
            "With no SOURCE, or when SOURCE is -, read standard input.\n"
            "\n"
            "Report bugs to <lydia-dev@falcon.pds.twi.tudelft.nl>.\n");
}

int trace_solver = 0;

/* Given the number of variables 'n', return variable_list with 'n' variables. */
static variable_list build_dimacs_variable_list(unsigned int n)
{
    unsigned int i;
    variable_list res = setroom_variable_list(new_variable_list(), n);
    for (i = 0; i < n; i++) {
        char name[20];
        bool_variable v;

        sprintf(name,"%u", (i + 1));
        v = new_bool_variable(new_identifier(add_lydia_symbol(name),
                                             int_listNIL,
                                             qualifier_listNIL),
                              variable_attribute_listNIL,
                              0);
        res = append_variable_list(res, to_variable(v));
    }
    return res;
}

/*
 * Quick and dirty: we assume all words are less than WORDSIZE characters
 * long. Given that these words are supposed to represent variable
 * numbers, to not fit in the word, they would have to be really huge...
 */
#define WORDSIZE 500u

/*
 * Given an input stream, read all digits up to the
 * next whitespace characters, and return them as a tmstring.
 */
static void read_word(FILE *f, char firstchar, char *buf, unsigned int bufsz)
{
    unsigned int ix=0;

    buf[ix++] = firstchar;
    for (;;) {
        int c = fgetc(f);

        if(c == EOF) {
            break;
        }
        if(!isdigit(c)) {
            ungetc(c, f);
            break;
        }
        if(ix >= bufsz) {
            buf[bufsz] = '\0';
            fprintf(stderr, "Word too long. I got '%s'\nGiving up\n", buf);
            exit(1);
        }
        buf[ix++] = c;
    }
    buf[ix] = '\0';
}

static int parse_DIMACS_file(FILE *f, tv_clause_list *clauses, variable_list *variables)
{
    char buf[WORDSIZE + 1];
    int lineno = 1;
    int start_of_line = 1;
    tv_clause_list cl = tv_clause_listNIL;
    int varcount;
    int tv_clausecount;
    int_list pos = new_int_list();
    int_list neg = new_int_list();

    for (;;) {
        int c = fgetc(f);
        int new_start_of_line = 0;

        if (c == EOF) {
            /* We're done. */
            break;
        }
        if (c == '\n') {
            lineno++;
            new_start_of_line = 1;
        }
        if (start_of_line && c == 'p') {
            int res;

            /* A parameters line. */
            if (cl != tv_clause_listNIL) {
                fprintf(stderr, "%d: Only one 'p' line allowed\n", lineno);
                return 0;
            }
            if ((res = fscanf(f, " cnf %d %d", &varcount, &tv_clausecount)) != 2) {
                fprintf( stderr, "%d: Malformed 'p' line\n", lineno );
                return 0;
            }

            /* There should only be whitespace at this line. */
            for (;;) {
                c = fgetc(f);

                if (c == '\n') {
                    lineno++;
                    new_start_of_line = 1;
                    break;
                }
            }

            cl = setroom_tv_clause_list(new_tv_clause_list(), (unsigned int)tv_clausecount);
        } else if (start_of_line && c == 'c') {
            /* The start of a comment line. Eat the rest of it. */
            for (;;) {
                c = fgetc( f );

                if (c == EOF || c == '\n') {
                    break;
                }
            }
            lineno++;
            new_start_of_line = 1;
        } else if (isspace(c)) {
            /* Skip. */
        } else if (c == '-' || isdigit(c)) {
            read_word(f, c, buf, WORDSIZE);
            if (buf[0] == '-' && buf[1] == '\0') {
                /* A special case we want to complain about. */
                fprintf( stderr, "%d: Solitary '-' ignored\n", lineno );
            } else {
                if (buf[0] == '-') {
                    int v = atoi(buf + 1);

                    /*
                     * We start counting the variables from '0', so
                     * we need to do 'v-1'.
                     */
                    neg = append_int_list(neg, v - 1);
                } else {
                    int v = atoi(buf);

                    if (v == 0) {
                        /*
                         * This terminates a tv_clause. Put it away, and
                         * start a new one.
                         */
                        if (cl != tv_clause_listNIL) {
                            tv_clause c = new_tv_clause( pos, neg );
                            cl = append_tv_clause_list(cl, c);
                        }
                        pos = new_int_list();
                        neg = new_int_list();
                    } else {
                        /*
                         * We start counting the variables from '0', so
                         * we need to do 'v-1'.
                         */
                        pos = append_int_list(pos, v - 1);
                    }
                }
            }
        } else {
            fprintf( stderr, "%d: Unexpected character '%c' ignored\n", lineno, c );
        }
        start_of_line = new_start_of_line;
    }
    /* If there is a pending tv_clause, put it away. */
    if (pos->sz != 0 || neg->sz != 0) {
        if (cl != tv_clause_listNIL) {
            tv_clause c = new_tv_clause(pos, neg);
            cl = append_tv_clause_list(cl, c);
        }
    } else {
        rfre_int_list(pos);
        rfre_int_list(neg);
    }
    *clauses = cl;
    *variables = build_dimacs_variable_list(varcount);
    return cl == tv_clause_listNIL ? 0 : 1;
}

int main(int argc, char *argv[])
{
    tv_clause_list clauses;
    variable_list variables;
    tv_term solution = NULL;
    tv_dnf result;

    int c;
    extern int optind;

    while ((c = getopt(argc, argv, "Vah?v")) != -1) {
        switch (c) {
            case 0:
                break;
            case 'a':
                findall = 1;
                break;
            case 'V':
                trace_solver = 1;
                break;
            case '?':
            case 'h':
                usage(stdout);
                return 1; /* exit failure */
            case 'v':
                version(stdout);
                return 0; /* exit failure */
            default:
                fprintf(stderr, "Try `cnf2dnf -h' for more information.\n");
                return 1; /* exit failure */
        }
    }

    infile = stdin;
    outfile = stdout;

    if (optind < argc) {
        infilename = strdup(argv[optind++]);
    }
    if (optind < argc) {
        outfilename = strdup(argv[optind++]);
    }

    if (infilename != NULL && 0 != strcmp(infilename, "-")) {
        infile = ckfopen(infilename, "r");
    }
    if (outfilename != NULL) {
        outfile = ckfopen(outfilename, "w");
    }

    init_symtbl();
    if (!parse_DIMACS_file(infile, &clauses, &variables)) {
        fprintf(stderr, "Giving up\n");
        destroy_symtbl();
        return EXIT_FAILURE;
    }

    fprintf(outfile, "Problem has %u variables and %u clauses\n", variables->sz, clauses->sz);

    result = new_tv_dnf(new_values_set_list(),
                        rdup_variable_list(variables),
                        new_variable_list(),
                        new_constant_list(),
                        ENCODING_NONE,
                        new_tv_term_list());
    if (findall) {
        lsss_get_all_solutions(clauses, variables->sz, result->terms);
    } else {
        if (lsss_get_solution(clauses, variables->sz, &solution)) {
            result->terms = append_tv_term_list(result->terms, solution);
        }
    }

    pp_dimacs_dnf(outfile, result);

    rfre_variable_list(variables);
    rfre_tv_clause_list(clauses);
    rfre_tv_dnf(result);
    destroy_symtbl();

    return EXIT_SUCCESS;
}
