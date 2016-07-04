/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_LC_PARSER_H_INCLUDED
# define YY_YY_LC_PARSER_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ARROW = 258,
     IDENTIFIER = 259,
     POS_FLOAT = 260,
     POS_INT = 261,
     KEY_AS = 262,
     KEY_ATTRIBUTE = 263,
     KEY_BOOL = 264,
     KEY_VARIABLE = 265,
     KEY_COND = 266,
     KEY_CONST = 267,
     KEY_DEFAULT = 268,
     KEY_ELSE = 269,
     KEY_ENUM = 270,
     KEY_EXISTS = 271,
     KEY_FALSE = 272,
     KEY_FLOAT = 273,
     KEY_FORALL = 274,
     KEY_FUNCTION = 275,
     KEY_IF = 276,
     KEY_IN = 277,
     KEY_INT = 278,
     KEY_MOD = 279,
     KEY_STRUCT = 280,
     KEY_SWITCH = 281,
     KEY_SYSTEM = 282,
     KEY_TRUE = 283,
     KEY_TO = 284,
     KEY_TYPE = 285,
     GE = 286,
     LE = 287,
     GT = 288,
     LT = 289,
     NE = 290,
     EQ = 291,
     IMPLY = 292,
     OR = 293,
     AND = 294,
     NOT = 295
   };
#endif
/* Tokens.  */
#define ARROW 258
#define IDENTIFIER 259
#define POS_FLOAT 260
#define POS_INT 261
#define KEY_AS 262
#define KEY_ATTRIBUTE 263
#define KEY_BOOL 264
#define KEY_VARIABLE 265
#define KEY_COND 266
#define KEY_CONST 267
#define KEY_DEFAULT 268
#define KEY_ELSE 269
#define KEY_ENUM 270
#define KEY_EXISTS 271
#define KEY_FALSE 272
#define KEY_FLOAT 273
#define KEY_FORALL 274
#define KEY_FUNCTION 275
#define KEY_IF 276
#define KEY_IN 277
#define KEY_INT 278
#define KEY_MOD 279
#define KEY_STRUCT 280
#define KEY_SWITCH 281
#define KEY_SYSTEM 282
#define KEY_TRUE 283
#define KEY_TO 284
#define KEY_TYPE 285
#define GE 286
#define LE 287
#define GT 288
#define LT 289
#define NE 290
#define EQ 291
#define IMPLY 292
#define OR 293
#define AND 294
#define NOT 295



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 24 "lc_parser.y"

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


/* Line 2058 of yacc.c  */
#line 183 "lc_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_LC_PARSER_H_INCLUDED  */
