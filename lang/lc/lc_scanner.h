#ifndef __LYDIA_SCANNER_H__
#define __LYDIA_SCANNER_H__

#include "ast.h"

extern int yylex(void);
extern void set_lexfile(FILE *infile, const char *infilename, char **ipath, unsigned int ipathsz);
extern void set_lex_debugging(int flag);
extern int yywrap();
extern void parserror(const char *msg);
extern origin make_origin();

#endif
