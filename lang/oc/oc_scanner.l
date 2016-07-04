%x incl comment
%option debug
%option prefix="obs"
%option outfile="lex.yy.c"
%{
#include "oc_scanner.h"
#include "oc_parser.h"
#include "findbin.h"
#include "config.h"
#include "strdup.h"
#include "error.h"
#include "oc.h"

#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef HAVE_DECL_GETOPT_FILENO
extern int fileno(FILE *stream);
#endif

static char **include_path;
static unsigned int include_path_size;

#define MAX_INCLUDE_DEPTH 10
static YY_BUFFER_STATE  include_stack[MAX_INCLUDE_DEPTH];
static unsigned int     include_lexline[MAX_INCLUDE_DEPTH];
static char             *include_lexfile[MAX_INCLUDE_DEPTH];
static int              include_stack_ptr = 0;
 
static char             *lexfile;
static unsigned int     lexline = 1;

/* Given a filename 'fnm', set that as the new lex input stream. */
void set_obs_lexfile(FILE *infile,
                     const char *infilename,
                     char **ipath,
                     unsigned int ipathsz)
{
    lexfile = strdup(infilename);
    yyin = infile;
    include_path = ipath;
    include_path_size = ipathsz;
}

obs_orig make_orig()
{
    return new_obs_orig(add_lydia_symbol(lexfile), lexline);
}

void set_obs_lex_debugging(int flag)
{
    yy_flex_debug = flag;
}

/* Flex 2.5.31 generates dodgy code if the '%noyywrap'
   option is used. To avoid problems with picky gcc compilers
   (such as e.g. happens on Mac OS 10.2), we include our own
   dummy empty yywrap() function. */
int yywrap(void)
{
    return 1;
}

/* Report an error from the yacc parser. */
void obs_parserror(const char *msg)
{
    /*
     * Sigh. Some versions of the parser say `syntax error', some
     * say `parse error'. The testsuite doesn't like this difference.
     * So we map the string `syntax error' to `parse error'.
     *
     * Sometimes a career in fruit retailing looks more attractive...
     */
    if (strcmp(msg, "syntax error" ) == 0) {
        msg = "parse error";
    }
    if (yytext[0] == '\n') {
        oc_parser_error(lexfile, lexline, "%s at NEWLINE", msg);
    } else {
        oc_parser_error(lexfile, lexline, "%s at `%s'", msg, yytext);
    }
}

static char *find_include(char *file)
{
    static char result[4096]; /* To Do: Fix this. */
    register unsigned int ix;

#ifndef WIN32
    struct stat st;
#else
    struct _stat st;
#endif
    for (ix = 0; ix < include_path_size; ix++) {
        strcpy(result, include_path[ix]);
        strcat(result, "/");
        strcat(result, file);
        
        if (0 == stat(result, &st)) {
            return result;
        }
    }
    return file;
}

/**
 * To Do: newline handling is currently Unix-centric (but will work
 * with windows). Do this right.
 */
%}

%option input
%option stack noyy_top_state
%option nounput

white_space     [ \t\f\r]
new_line        \r?\n

and             and
default         default
false           false
include         include
cond            cond
not             not
or              or
observation     observation
true            true

digit           [0-9]
base            ({digit}+)|(({digit}*)\.{digit}+)
exponent        [eE](({digit}+)|([+-]{digit}+))
floatconst      ({base})|({base}{exponent})
intconst        {digit}+
letter          [A-Za-z_]
identifier      {letter}({letter}|{digit})*

%%

{white_space}
{new_line}      { lexline++; }
    
"/*"            { yy_push_state(comment); }

<comment>{
    "*/"                yy_pop_state();
    "*"                 /* eat up C-comments */
    [^*\n]+             /* eat up C-comments */
    [^*\n]*{new_line}   ++lexline;
}

"//".*          { /* eat up C++-like comments.*/ }
    

"#"include      { BEGIN(incl); }
<incl>[ \t]*      /* eat the whitespace */
<incl>[^ \t\n]+ {
    /* got the include file name */
    if (include_stack_ptr >= MAX_INCLUDE_DEPTH) {
        fprintf(stderr,
                "oc: file %s, line %d: includes nested too deeply\n",
                lexfile,
                lexline);
        exit(EXIT_FAILURE);
    }
 
    if (yytext[0] != '"' || yytext[strlen(yytext) - 1] != '"') {
        obs_orig org = new_obs_orig(add_lydia_symbol(lexfile), lexline);
        loeh_error(COMPILER_INCLUDE_ERROR,
                   LOEH_LOCATION_GLOBAL,
                   org,
                   yytext);
        rfre_obs_orig(org);
        exit(EXIT_FAILURE);
    }
    yytext++;
    yytext[strlen(yytext) - 1] = '\0';
    if (NULL == (yyin = fopen(find_include(yytext), "r"))) {
        obs_orig org = new_obs_orig(add_lydia_symbol(lexfile), lexline);
        loeh_error(COMPILER_OPEN_ERROR,
                   LOEH_LOCATION_GLOBAL,
                   org,
                   yytext,
                   strerror(errno));
        rfre_obs_orig(org);
        exit(EXIT_FAILURE);
    }

    include_stack[include_stack_ptr] = YY_CURRENT_BUFFER;
    include_lexfile[include_stack_ptr] = lexfile;
    include_lexline[include_stack_ptr] = lexline;
    include_stack_ptr++;
    lexline = 1;
    lexfile = strdup(yytext);
 
    yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
 
    BEGIN(INITIAL);
}

"!"             { return NOT; } 
"!="            { return NE; }
"->"            { return ARROW; } 
"="             { return EQ; }
"=="            { return EQ; }
"<"             { return LT; }
">"             { return GT; }
"<="            { return LE; }
">="            { return GE; }
"=>"            { return IMPLY; }
and             { return AND; }
"&&"            { return AND; }
default         { return KEY_DEFAULT; }
false           { return KEY_FALSE; }
cond            { return KEY_COND; }
not             { return NOT; }
or              { return OR; }
"||"            { return OR; }
true            { return KEY_TRUE; }
observation     { return KEY_OBSERVATION; }

{intconst}	{
    obslval._int = atoi(yytext);
    return POS_INT;
}

{identifier}    {
    obslval._identifier = add_lydia_symbol(yytext);

    return IDENTIFIER;
}

<<EOF>>         {
    free(lexfile);
    if (--include_stack_ptr < 0) {
        yyterminate();
    } else {
        yy_delete_buffer(YY_CURRENT_BUFFER);
        yy_switch_to_buffer(include_stack[include_stack_ptr]);
        lexline = include_lexline[include_stack_ptr];
        lexfile = include_lexfile[include_stack_ptr];
    }
}

.               { return *yytext; }

%%
