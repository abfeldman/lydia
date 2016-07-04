#ifndef OC_SCANNER_H
#define OC_SCANNER_H

#include "oc.h"

extern int yylex(void);
extern void set_obs_lexfile(FILE *infile, const char *infilename, char **ipath, unsigned int ipathsz);
extern void set_obs_lex_debugging(int flag);
extern int yywrap();
extern void obs_parserror(const char *msg);
extern obs_orig make_orig();

#endif
