%{
#include "oc_scanner.h"
#include "config.h"
#include "array.h"

#include <string.h>

/* Generate the code to allow yacc tracing. */
#define YYDEBUG 1

static obs_dump result;

static void yyerror(const char *msg)
{
/* Delegate error reporting to the lexer. */
    obs_parserror(msg);
    exit(1);
}
%}

%union
{
    lydia_symbol                  _identifier;
    int                           _int;
    obs_choice                    _choice;
    obs_choice_list               _choicelist;
    obs_expr                      _expr;
    obs_expr_list                 _exprlist;
    obs_expr_variable             _exprvariable;
    obs_instance                  _instance;
    obs_instance_list             _instancelist;
    obs_orig                      _origin;
    obs_orig_symbol               _origsymbol;
    obs_variable_identifier       _variableidentifier;
    obs_extent_list               _extentlist;
    obs_variable_qualifier        _variablequalifier;
    obs_variable_qualifier_list   _variablequalifierlist;
    obs_extent                    _extent;
}

%start dump

%token                            ARROW
%token <_identifier>              IDENTIFIER
%token <_int>                     POS_INT

%token                            KEY_COND
%token                            KEY_DEFAULT
%token                            KEY_FALSE
%token                            KEY_OBSERVATION
%token                            KEY_TRUE

%nonassoc                         EQ NE LT GT LE GE
%right                            '?' ':'
%nonassoc                         IMPLY
%left                             OR
%left                             AND
%left                             NOT

%type    <_choice>                choice
%type    <_choicelist>            choice_list
%type    <_expr>                  constraint
%type    <_exprlist>              constraint_list
%type    <_expr>                  expr
%type    <_expr>                  expr_binary
%type    <_expr>                  expr_literal
%type    <_expr>                  expr_unary
%type    <_exprlist>              expr_list
%type    <_exprvariable>          expr_variable
%type    <_extent>                array_selector
%type    <_extentlist>            array_selector_list
%type    <_instance>              instance
%type    <_instancelist>          instance_list
%type    <_origin>                origin
%type    <_origsymbol>            origin_identifier
%type    <_variableidentifier>    variable_identifier
%type    <_variablequalifier>     variable_qualifier
%type    <_variablequalifierlist> variable_qualifier_list

%%

dump:
    /* empty */
    {
        result = new_obs_dump(new_obs_instance_list());
    } |
    instance_list
    {
        result = new_obs_dump($1);
    };

instance_list:
    instance
    {
        $$ = new_obs_instance_list();
        if ($1 != obs_instanceNIL) {
            $$ = append_obs_instance_list($$, $1);
        }
    } |
    instance_list instance
    {
        if ($2 == obs_instanceNIL) {
            $$ = $1;
        } else {
            $$ = append_obs_instance_list($1, $2);
        }
    };

instance:
    KEY_OBSERVATION origin_identifier '{' constraint_list '}'
    {
        $$ = new_obs_instance(to_obs_type(new_obs_bool_type()), $2, $4);
    };

constraint_list:
    /* empty */
    {
        $$ = new_obs_expr_list();
    } |
    constraint_list origin constraint
    {
        if ($3->org) {
            rfre_obs_orig($3->org);
        }
        $3->org = $2;
 
        $$ = append_obs_expr_list($1, $3);
    };

constraint:
    expr ';'
    {
        $$ = $1;
    };

expr:
    expr_unary
    {
        $$ = $1;
    } |
    expr_binary
    {
        $$ = $1;
    } |
    expr_literal
    {
        $$ = $1;
    } |
    expr_variable
    {
        $$ = to_obs_expr($1);
    } |
    '(' expr ')'
    {
        $$ = $2;
    } |
    expr '?' expr ':' expr
    {
        $$ = to_obs_expr(new_obs_expr_if_else(obs_origNIL, obs_typeNIL, $1, $3, $5));
    } |
    KEY_COND '(' expr ')' '(' choice_list ';' KEY_DEFAULT ARROW expr optional_separator ')'
    {
        $$ = to_obs_expr(new_obs_expr_cond(obs_origNIL, obs_typeNIL, $3, $6, $10));
    } |
    KEY_COND '(' expr ')' '(' choice_list optional_separator ')'
    {
        $$ = to_obs_expr(new_obs_expr_cond(obs_origNIL, obs_typeNIL, $3, $6, obs_exprNIL));
    } |
    '[' expr_list ']'
    {
        $$ = to_obs_expr(new_obs_expr_concatenation(obs_origNIL, obs_typeNIL, $2));
    };

expr_unary:
    NOT expr
    {
        $$ = to_obs_expr(new_obs_expr_not(obs_origNIL, obs_typeNIL, $2));
    };

expr_binary:
    expr AND expr
    {
        $$ = to_obs_expr(new_obs_expr_and(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr OR expr
    {
        $$ = to_obs_expr(new_obs_expr_or(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr IMPLY expr
    {
        $$ = to_obs_expr(new_obs_expr_imply(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr LT expr
    {
        $$ = to_obs_expr(new_obs_expr_lt(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr GT expr
    {
        $$ = to_obs_expr(new_obs_expr_gt(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr LE expr
    {
        $$ = to_obs_expr(new_obs_expr_le(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr GE expr
    {
        $$ = to_obs_expr(new_obs_expr_ge(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr EQ expr
    {
        $$ = to_obs_expr(new_obs_expr_eq(obs_origNIL, obs_typeNIL, $1, $3));
    } |
    expr NE expr
    {
        $$ = to_obs_expr(new_obs_expr_ne(obs_origNIL, obs_typeNIL, $1, $3));
    };

expr_literal:
    origin KEY_TRUE
    {
        $$ = to_obs_expr(new_obs_expr_bool($1, obs_typeNIL, LYDIA_TRUE));
    } |
    origin KEY_FALSE
    {
        $$ = to_obs_expr(new_obs_expr_bool($1, obs_typeNIL, LYDIA_FALSE));
    } |
    origin POS_INT
    {
        $$ = to_obs_expr(new_obs_expr_int($1, obs_typeNIL, $2));
    };

expr_variable:
    origin variable_identifier
    {
        $$ = new_obs_expr_variable($1, obs_typeNIL, $2);
    };

expr_list:
    /* empty */
    {
        $$ = new_obs_expr_list();
    } |
    expr
    {
        $$ = append_obs_expr_list(new_obs_expr_list(), $1);
    } |
    expr_list ',' expr
    {
        $$ = append_obs_expr_list($1, $3);
    };

choice_list:
    choice
    {
        $$ = append_obs_choice_list(new_obs_choice_list(), $1);
    } |
    choice_list ';' choice
    {
        $$ = append_obs_choice_list($1, $3);
    };

choice:
    expr ARROW origin expr
    {
        $$ = new_obs_choice($3, $1, $4);
    };

optional_separator:
    /* empty */
    |
    ';';

variable_identifier:
    origin_identifier
    {
        $$ = new_obs_variable_identifier($1, obs_extent_listNIL, obs_variable_qualifier_listNIL);
    } |
    origin_identifier array_selector_list
    {
        $$ = new_obs_variable_identifier($1, $2, obs_variable_qualifier_listNIL);
    } |
    variable_qualifier_list '.' origin_identifier
    {
        $$ = new_obs_variable_identifier($3, obs_extent_listNIL, $1);
    } |
    variable_qualifier_list '.' origin_identifier array_selector_list
    {
        $$ = new_obs_variable_identifier($3, $4, $1);
    };

variable_qualifier_list:
    variable_qualifier
    {
        $$ = append_obs_variable_qualifier_list(new_obs_variable_qualifier_list(), $1);
    } |
    variable_qualifier_list '.' variable_qualifier
    {
        $$ = append_obs_variable_qualifier_list($1, $3);
    };

variable_qualifier:
    origin_identifier array_selector_list
    {
        $$ = new_obs_variable_qualifier($1, $2);
    } |
    origin_identifier
    {
        $$ = new_obs_variable_qualifier($1, obs_extent_listNIL);
    };

array_selector:
    '[' expr ']'
    {
        $$ = new_obs_extent($2, rdup_obs_expr($2));
    } |
    '[' expr ':' expr ']'
    {
    $$ = new_obs_extent($2, $4);
    } |
    '[' ':' origin expr ']'
    {
        $$ = new_obs_extent(to_obs_expr(new_obs_expr_int($3, obs_typeNIL, 0)), $4);
    } |
    '[' expr ':' ']'
    {
        $$ = new_obs_extent($2, obs_exprNIL);
    };

array_selector_list:
    array_selector
    {
        $$ = append_obs_extent_list(new_obs_extent_list(), $1);
    } |
    array_selector_list array_selector
    {
        $$ = append_obs_extent_list($1, $2);
    };

origin_identifier:
    IDENTIFIER origin
    {
        $$ = new_obs_orig_symbol($1, $2);
    };

origin:
    /* empty */
    {
        $$ = make_orig();
    };

%%

obs_dump obs_parse(FILE *infile,
                   const char *infilename,
                   char **ipath,
                   unsigned int ipathsz,
                   int trace_yacc,
                   int trace_lex)
{
    int r;

    yydebug = trace_yacc;
    set_obs_lexfile(infile, infilename, ipath, ipathsz);
    set_obs_lex_debugging(trace_lex);

    r = yyparse();

    if (r != 0) {
        return obs_dumpNIL;
    }

    return result;
}
