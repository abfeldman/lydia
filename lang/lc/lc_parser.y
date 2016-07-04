%{
#include "lc_scanner.h"
#include "config.h"
#include "array.h"
#include "error.h"
#include "ast.h"

#include <string.h>

/* Generate the code to allow yacc tracing. */
#define YYDEBUG 1

static model result;

static void yyerror(const char *msg)
{
/* Delegate error reporting to the lexer. */
    parserror(msg);
    exit(1);
}
%}

%union
{
    attribute_declaration        _attributedeclaration;
    attribute_instantiation      _attributeinstantiation;
    attribute_instantiation_list _attributeinstantiationlist;
    choice                       _choice;
    choice_list                  _choicelist;
    compound_predicate           _compoundpredicate;
    definition                   _definition;
    definition_list              _definitionlist;
    double                       _float;
    exists_predicate             _existspredicate;
    expr                         _expr;
    expr_variable                _exprvariable;
    expr_list                    _exprlist;
    extent                       _extent;
    extent_list                  _extentlist;
    expr_variable_list           _variableexprlist;
    forall_predicate             _forallpredicate;
    formal                       _formal;
    formal_list                  _formallist;
    if_predicate                 _ifpredicate;
    int                          _int;
    model                        _model;
    orig_symbol                  _origsymbol;
    orig_symbol_list             _origsymbollist;
    origin                       _origin;
    predicate                    _predicate;
    predicate_list               _predicatelist;
    struct_entry_list            _structlist;
    switch_choice                _switchchoice;
    switch_choice_list           _switchchoicelist;
    switch_predicate             _switchpredicate;
    system_declaration           _systemdeclaration;
    system_instantiation         _systeminstantiation;
    system_instantiation_list    _systeminstantiationlist;
    lydia_symbol                 _identifier;
    type                         _type;
    variable_declaration         _variabledeclaration;
    variable_instantiation       _variableinstantiation;
    variable_instantiation_list  _variableinstantiationlist;
    variable_identifier          _variableidentifier;
    variable_qualifier_list      _variablequalifierlist;
    variable_qualifier           _variablequalifier;
}

%start model

%token                          ARROW
%token <_identifier>            IDENTIFIER
%token <_float>                 POS_FLOAT
%token <_int>                   POS_INT

%token                          KEY_AS
%token                          KEY_ATTRIBUTE
%token                          KEY_BOOL
%token                          KEY_VARIABLE
%token                          KEY_COND
%token                          KEY_CONST
%token                          KEY_DEFAULT
%token                          KEY_ELSE
%token                          KEY_ENUM
%token                          KEY_EXISTS
%token                          KEY_FALSE
%token                          KEY_FLOAT
%token                          KEY_FORALL
%token                          KEY_FUNCTION
%token                          KEY_IF
%token                          KEY_IN
%token                          KEY_INT
%token                          KEY_MOD
%token                          KEY_STRUCT
%token                          KEY_SWITCH
%token                          KEY_SYSTEM
%token                          KEY_TRUE
%token                          KEY_TO
%token                          KEY_TYPE

%nonassoc                       EQ NE LT GT LE GE
%right                          '?' ':'
%right                          '}' KEY_DEFAULT
%nonassoc                       IMPLY
%left                           '+' '-' OR
%left                           '*' '/' AND KEY_MOD
%left                           NOT
%left                           KEY_AS
%right                          ')' KEY_IF KEY_ELSE

/* use sort -b +2 */

%type    <_attributedeclaration>       attribute_declaration
%type    <_attributeinstantiation>     attribute_instantiation
%type    <_attributeinstantiationlist> attribute_instantiation_list
%type    <_choice>                     choice
%type    <_choicelist>                 choices
%type    <_compoundpredicate>          compound_predicate
%type    <_compoundpredicate>          system_body
%type    <_definition>                 definition
%type    <_definitionlist>             definition_list
%type    <_existspredicate>            exists_predicate
%type    <_expr>                       binary_expr
%type    <_expr>                       expr
%type    <_expr>                       literal_expr
%type    <_expr>                       unary_expr
%type    <_exprvariable>               variable_expr
%type    <_exprlist>                   actual_parameters
%type    <_exprlist>                   expr_list
%type    <_extent>                     array_declaration
%type    <_extent>                     array_selector
%type    <_extentlist>                 array_declaration_list
%type    <_extentlist>                 array_selector_list
%type    <_forallpredicate>            forall_predicate
%type    <_formal>                     formal
%type    <_formal>                     typed_formal
%type    <_formallist>                 formal_parameter_list
%type    <_formallist>                 formal_parameters
%type    <_ifpredicate>                if_else_predicate
%type    <_model>                      model
%type    <_origin>                     origin
%type    <_origsymbol>                 origin_identifier
%type    <_origsymbollist>             enum_list
%type    <_predicate>                  predicate
%type    <_predicatelist>              predicate_list
%type    <_structlist>                 struct_list
%type    <_switchchoice>               switch_choice
%type    <_switchchoicelist>           switch_choices
%type    <_switchpredicate>            switch_predicate
%type    <_systemdeclaration>          system_declaration
%type    <_systeminstantiation>        system_instantiation
%type    <_systeminstantiationlist>    system_instantiation_list
%type    <_type>                       atomic_type
%type    <_type>                       type
%type    <_type>                       user_type
%type    <_variabledeclaration>        variable_declaration
%type    <_variableexprlist>           variable_expr_list
%type    <_variableidentifier>         variable_identifier
%type    <_variableinstantiation>      variable_instantiation
%type    <_variableinstantiationlist>  variable_instantiation_list
%type    <_variablequalifier>          variable_qualifier
%type    <_variablequalifierlist>      variable_qualifier_list

%%

model:
    /* empty */
    {
        result = new_model(new_definition_list());
    } |
    definition_list
    {
        result = new_model($1);
    };

definition:
    KEY_ATTRIBUTE type origin_identifier ';'
    {
        $$ = to_definition(new_attribute_definition($3, $2));
    } |
    KEY_CONST type origin_identifier EQ expr ';'
    {
        $$ = to_definition(new_constant_definition($3, $2, $5));
    } |
    KEY_TYPE origin_identifier EQ type ';'
    {
        $$ = to_definition(new_type_definition($2, $4));
    } |
    KEY_TYPE origin_identifier EQ KEY_ENUM '{' enum_list '}' ';'
    {
        $$ = to_definition(new_enum_definition($2, $6));
    } |
    KEY_TYPE origin_identifier EQ KEY_STRUCT '{' struct_list '}' ';'
    {
        $$ = to_definition(new_struct_definition($2, $6));
    } |
    type KEY_FUNCTION origin_identifier formal_parameters '{' expr '}'
    {
        unsigned int ix;
        $$ = to_definition(new_function_definition($3, $1, $4, $6));
        for (ix = 0; ix < $4->sz; ix++) {
            $4->arr[ix]->offset = ix;
        }

    } |
    KEY_SYSTEM origin_identifier formal_parameters system_body
    {
        $$ = to_definition(new_system_definition($2, to_type(new_bool_type()), $3, new_local_list(), new_reference_list(), new_attribute_list(), $4));
    };

definition_list:
    definition
    {
        $$ = new_definition_list();
        if ($1 != definitionNIL) {
            $$ = append_definition_list($$, $1);
        }
    } |
    definition_list definition
    {
        if ($2 == definitionNIL) {
            $$ = $1;
        } else {
            $$ = append_definition_list($1, $2);
        }
    };

enum_list:
    /* empty */
    {
        $$ = new_orig_symbol_list();
    } |
    origin_identifier
    {
        $$ = append_orig_symbol_list(new_orig_symbol_list(), $1);
    } |
    enum_list ',' origin_identifier
    {
        $$ = append_orig_symbol_list($1, $3);
    };

struct_list:
    /* empty */
    {
        $$ = new_struct_entry_list();
    } |
    type origin_identifier
    {
        $$ = append_struct_entry_list(new_struct_entry_list(), new_struct_entry($1, $2, extent_listNIL));
    } |
    type origin_identifier array_declaration_list
    {
        $$ = append_struct_entry_list(new_struct_entry_list(), new_struct_entry($1, $2, $3));
    } |
    struct_list ',' type origin_identifier
    {
        $$ = append_struct_entry_list($1, new_struct_entry($3, $4, extent_listNIL));
    } |
    struct_list ',' type origin_identifier array_declaration_list
    {
        $$ = append_struct_entry_list($1, new_struct_entry($3, $4, $5));
    };

system_body:
    compound_predicate
    {
        $$ = $1;
    };

type:
    atomic_type
    {
        $$ = $1;
    } |
    user_type
    {
        $$ = $1;
    };

atomic_type:
    KEY_BOOL
    {
        $$ = to_type(new_bool_type());
    } |
    KEY_FLOAT
    {
        $$ = to_type(new_float_type());
    } |
    KEY_INT
    {
        $$ = to_type(new_int_type());
    } |
    KEY_VARIABLE
    {
        $$ = to_type(new_variable_type());
    };

user_type:
    origin_identifier
    {
        $$ = to_type(new_user_type($1));
    };

formal_parameters:
    /* empty */
    {
        $$ = new_formal_list();
    } |
    '(' formal_parameter_list ')'
    {  
        $$ = $2;
    };

formal:
    origin_identifier
    {
        $$ = new_formal(new_variable_identifier($1, extent_listNIL, variable_qualifier_listNIL), typeNIL, 0, int_listNIL);
    } |
    origin_identifier array_declaration_list
    {
        $$ = new_formal(new_variable_identifier($1, $2, variable_qualifier_listNIL), typeNIL, 0, int_listNIL);
    } |
    typed_formal
    {
        $$ = $1;
    };

typed_formal:
    type origin_identifier
    {
        $$ = new_formal(new_variable_identifier($2, extent_listNIL, variable_qualifier_listNIL), $1, 0, int_listNIL);
    } |
    type origin_identifier array_declaration_list
    {
        $$ = new_formal(new_variable_identifier($2, $3, variable_qualifier_listNIL), $1, 0, int_listNIL);
    };

formal_parameter_list:
    /* empty */
    {
        $$ = new_formal_list();
    } |
    typed_formal
    {
        $$ = append_formal_list(new_formal_list(), $1);
    } |
    formal_parameter_list ',' formal
    {
        $$ = append_formal_list($1, $3);
    };

predicate:
    expr origin ';'
    {
        $$ = to_predicate(new_simple_predicate($2, $1));
    } |
    system_declaration ';'
    {
        $$ = to_predicate($1);
    } |
    variable_declaration ';'
    {
        $$ = to_predicate($1);
    } |
    attribute_declaration ';'
    {
        $$ = to_predicate($1);
    } |
    compound_predicate
    {
        $$ = to_predicate($1);
    } |
    if_else_predicate
    {
        $$ = to_predicate($1);
    } |
    switch_predicate
    {
        $$ = to_predicate($1);
    } |
    forall_predicate
    {
        $$ = to_predicate($1);
    } |
    exists_predicate
    {
        $$ = to_predicate($1);
    };

predicate_list:
    /* empty */
    {
        $$ = new_predicate_list();
    } |
    predicate_list origin predicate
    {
        if ($3->org) {
            rfre_origin($3->org);
        }
        $3->org = $2;
 
        $$ = append_predicate_list($1, $3);
    };

system_declaration:
    KEY_SYSTEM origin_identifier system_instantiation_list
    {
        $$ = new_system_declaration(rdup_origin($2->org), $2, $3);
    };

system_instantiation_list:
    system_instantiation
    {
        $$ = append_system_instantiation_list(new_system_instantiation_list(), $1);
    } |
    system_instantiation_list ',' system_instantiation
    {
        $$ = append_system_instantiation_list($1, $3);
    };

system_instantiation:
    origin_identifier
    {
        $$ = new_system_instantiation($1, extent_listNIL, expr_listNIL);
    } |
    origin_identifier actual_parameters
    {
        $$ = new_system_instantiation($1, extent_listNIL, $2);
    } |
    origin_identifier array_declaration_list
    {
        $$ = new_system_instantiation($1, $2, expr_listNIL);
    } |
    origin_identifier array_declaration_list actual_parameters
    {
        $$ = new_system_instantiation($1, $2, $3);
    };

attribute_declaration:
    KEY_ATTRIBUTE origin attribute_instantiation_list
    {
        $$ = new_attribute_declaration($2, $3);
    };

attribute_instantiation_list:
    attribute_instantiation
    {
        $$ = append_attribute_instantiation_list(new_attribute_instantiation_list(), $1);
    } |
    attribute_instantiation_list ',' attribute_instantiation
    {
        $$ = append_attribute_instantiation_list($1, $3);
    };

attribute_instantiation:
    type '(' variable_expr_list ')' EQ expr
    {
        $$ = new_attribute_instantiation($1, $3, orig_symbolNIL, $6);
    } |
    type '(' variable_expr_list ')' EQ '\\' origin_identifier expr
    {
        $$ = new_attribute_instantiation($1, $3, $7, $8);
    };

variable_expr_list:
    variable_expr
    {
        $$ = append_expr_variable_list(new_expr_variable_list(), $1);
    } |
    variable_expr_list ',' variable_expr
    {
        $$ = append_expr_variable_list($1, $3);
    };

variable_identifier:
    origin_identifier
    {
        $$ = new_variable_identifier($1, extent_listNIL, variable_qualifier_listNIL);
    } |
    origin_identifier array_selector_list
    {
        $$ = new_variable_identifier($1, $2, variable_qualifier_listNIL);
    } |
    variable_qualifier_list '.' origin_identifier
    {
        $$ = new_variable_identifier($3, extent_listNIL, $1);
    } |
    variable_qualifier_list '.' origin_identifier array_selector_list
    {
        $$ = new_variable_identifier($3, $4, $1);
    };

variable_qualifier_list:
    variable_qualifier
    {
        $$ = append_variable_qualifier_list(new_variable_qualifier_list(), $1);
    } |
    variable_qualifier_list '.' variable_qualifier
    {
        $$ = append_variable_qualifier_list($1, $3);
    };

variable_qualifier:
    origin_identifier array_selector_list
    {
        $$ = new_variable_qualifier($1, $2);
    } |
    origin_identifier
    {
        $$ = new_variable_qualifier($1, extent_listNIL);
    };

variable_declaration:
    type origin variable_instantiation_list
    {
        $$ = new_variable_declaration($2, $1, $3);
    };

variable_instantiation_list:
    variable_instantiation
    {
        $$ = append_variable_instantiation_list(new_variable_instantiation_list(), $1);
    } |
    variable_instantiation_list ',' variable_instantiation
    {
        $$ = append_variable_instantiation_list($1, $3);
    };

variable_instantiation:
    origin_identifier
    {
        $$ = new_variable_instantiation(typeNIL, $1, extent_listNIL, exprNIL);
    } |
    origin_identifier array_declaration_list
    {
        $$ = new_variable_instantiation(typeNIL, $1, $2, exprNIL);
    } |
    origin_identifier EQ expr
    {
        $$ = new_variable_instantiation(typeNIL, $1, extent_listNIL, $3);
    };

compound_predicate:
    '{' origin predicate_list '}'
    {
        $$ = new_compound_predicate($2, new_orig_symbol(lydia_symbolNIL, rdup_origin($2)), $3, new_local_list());
    };

if_else_predicate:
    KEY_IF origin '(' expr ')' compound_predicate origin
    {
        $$ = new_if_predicate($2,
                              $4,
                              $6,
                              new_compound_predicate($7,
                                                     new_orig_symbol(lydia_symbolNIL, rdup_origin($7)),
                                                     new_predicate_list(),
                                                     new_local_list()));
    } |
    KEY_IF origin '(' expr ')' compound_predicate KEY_ELSE compound_predicate
    {
        $$ = new_if_predicate($2, $4, $6, $8);
    } |
    KEY_IF origin '(' expr ')' compound_predicate KEY_ELSE origin if_else_predicate
    {
        compound_predicate c = new_compound_predicate($8,
                                                      new_orig_symbol(lydia_symbolNIL, rdup_origin($8)),
                                                      append_predicate_list(new_predicate_list(), to_predicate($9)),
                                                      new_local_list());
        $$ = new_if_predicate($2, $4, $6, c);
    };

switch_predicate:
    KEY_SWITCH origin '(' expr ')' '{' switch_choices '}'
    {
        $$ = new_switch_predicate($2, $4, $7, compound_predicateNIL);
    } |
    KEY_SWITCH origin '(' expr ')' '{' switch_choices KEY_DEFAULT ARROW compound_predicate '}'
    {
        $$ = new_switch_predicate($2, $4, $7, $10);
    };

forall_predicate:
    KEY_FORALL origin '(' origin_identifier KEY_IN expr KEY_TO expr ')' compound_predicate
    {
        $$ = new_forall_predicate($2, $4, new_extent($6, $8), $10);
    };

exists_predicate:
    KEY_EXISTS origin '(' origin_identifier KEY_IN expr KEY_TO expr ')' compound_predicate
    {
        $$ = new_exists_predicate($2, $4, new_extent($6, $8), $10);
    };

switch_choices:
    /* empty */
    {
        $$ = new_switch_choice_list();
    } |
    switch_choices switch_choice
    {
        $$ = append_switch_choice_list($1, $2);
    };

switch_choice:
    expr ARROW origin compound_predicate
    {
        $$ = new_switch_choice($1, $4, $3);
    };

expr:
    unary_expr
    {
        $$ = $1;
    } |
    binary_expr
    {
        $$ = $1;
    } |
    literal_expr
    {
        $$ = $1;
    } |
    variable_expr
    {
        $$ = to_expr($1);
    } |
    '(' expr ')'
    {
        $$ = $2;
    } |
    expr '?' expr ':' expr
    {
        $$ = to_expr(new_expr_if_else(typeNIL, $1, $3, $5));
    } |
    KEY_COND '(' expr ')' '(' choices ';' KEY_DEFAULT ARROW expr optional_separator ')'
    {
        $$ = to_expr(new_expr_cond(typeNIL, $3, $6, $10));
    } |
    KEY_COND '(' expr ')' '(' choices optional_separator ')'
    {
        $$ = to_expr(new_expr_cond(typeNIL, $3, $6, exprNIL));
    } |
    expr KEY_AS type
    {
        $$ = to_expr(new_expr_cast(typeNIL, $1, $3));
    } |
    origin_identifier actual_parameters
    {
        $$ = to_expr(new_expr_apply(typeNIL, $1, extent_listNIL, $2));
    } |
    origin_identifier array_selector_list actual_parameters
    {
        $$ = to_expr(new_expr_apply(typeNIL, $1, $2, $3));
    } |
    '[' expr_list ']'
    {
        $$ = to_expr(new_expr_concatenation(typeNIL, $2));
    };

optional_separator:
    /* empty */
    |
    ';';

unary_expr:
    NOT expr
    {
        $$ = to_expr(new_expr_not(typeNIL, $2));
    } |
    '-' expr
    {
        $$ = to_expr(new_expr_negate(typeNIL, $2));
    };

binary_expr:
    expr '+' expr
    {
        $$ = to_expr(new_expr_add(typeNIL, $1, $3));
    } |
    expr '-' expr
    {
        $$ = to_expr(new_expr_sub(typeNIL, $1, $3));
    } |
    expr '*' expr
    {
        $$ = to_expr(new_expr_mult(typeNIL, $1, $3));
    } |
    expr '/' expr
    {
        $$ = to_expr(new_expr_div(typeNIL, $1, $3));
    } |
    expr KEY_MOD expr
    {
        $$ = to_expr(new_expr_mod(typeNIL, $1, $3));
    } |
    expr AND expr
    {
        $$ = to_expr(new_expr_and(typeNIL, $1, $3));
    } |
    expr OR expr
    {
        $$ = to_expr(new_expr_or(typeNIL, $1, $3));
    } |
    expr IMPLY expr
    {
        $$ = to_expr(new_expr_imply(typeNIL, $1, $3));
    } |
    expr LT expr
    {
        $$ = to_expr(new_expr_lt(typeNIL, $1, $3));
    } |
    expr GT expr
    {
        $$ = to_expr(new_expr_gt(typeNIL, $1, $3));
    } |
    expr LE expr
    {
        $$ = to_expr(new_expr_le(typeNIL, $1, $3));
    } |
    expr GE expr
    {
        $$ = to_expr(new_expr_ge(typeNIL, $1, $3));
    } |
    expr EQ expr
    {
        $$ = to_expr(new_expr_eq(typeNIL, $1, $3));
    } |
    expr NE expr
    {
        $$ = to_expr(new_expr_ne(typeNIL, $1, $3));
    };

literal_expr:
    KEY_TRUE
    {
        $$ = to_expr(new_expr_bool(typeNIL, LYDIA_TRUE));
    } |
    KEY_FALSE
    {
        $$ = to_expr(new_expr_bool(typeNIL, LYDIA_FALSE));
    } |
    POS_INT
    {
        $$ = to_expr(new_expr_int(typeNIL, $1));
    } |
    POS_FLOAT
    {
        $$ = to_expr(new_expr_float(typeNIL, $1));
    };

variable_expr:
    variable_identifier
    {
        $$ = new_expr_variable(typeNIL, $1);
    };

expr_list:
    /* empty */
    {
        $$ = new_expr_list();
    } |
    expr
    {
        $$ = append_expr_list(new_expr_list(), $1);
    } |
    expr_list ',' expr
    {
        $$ = append_expr_list($1, $3);
    };

choices:
    choice
    {
        $$ = append_choice_list(new_choice_list(), $1);
    } |
    choices ';' choice
    {
        $$ = append_choice_list($1, $3);
    };

choice:
    expr ARROW origin expr
    {
        $$ = new_choice($3, $1, $4);
    };

actual_parameters:
    '(' expr_list ')'
    {
        $$ = $2;
    };

origin:
    /* empty */
    {
        $$ = make_origin();
    };

array_declaration_list:
    array_declaration
    {
        $$ = append_extent_list(new_extent_list(), $1);
    } |
    array_declaration_list array_declaration
    {
        $$ = append_extent_list($1, $2);
    };

array_selector_list:
    array_selector
    {
        $$ = append_extent_list(new_extent_list(), $1);
    } |
    array_selector_list array_selector
    {
        $$ = append_extent_list($1, $2);
    };

array_declaration:
    '[' expr ']'
    {
        $$ = new_extent(exprNIL, $2);
    } |
    '[' expr ':' expr ']'
    {
        $$ = new_extent($2, $4);
    };

array_selector:
    '[' expr ']'
    {
        $$ = new_extent($2, rdup_expr($2));
    } |
    '[' expr ':' expr ']'
    {
        $$ = new_extent($2, $4);
    } |
    '[' ':' expr ']'
    {
        $$ = new_extent(to_expr(new_expr_int(typeNIL, 0)), $3);
    } |
    '[' expr ':' ']'
    {
        $$ = new_extent($2, exprNIL);
    };

origin_identifier:
    IDENTIFIER origin
    {
        $$ = new_orig_symbol($1, $2);
    };

%%

model parse(FILE *infile,
            const char *infilename,
            char **ipath,
            unsigned int ipathsz,
            int trace_yacc,
            int trace_lex)
{
    int r;

    yydebug = trace_yacc;
    set_lexfile(infile, infilename, ipath, ipathsz);
    set_lex_debugging(trace_lex);

    r = yyparse();

    if (r != 0) {
        return modelNIL;
    }

    return result;
}
