#ifndef ERROR_H
#define ERROR_H

#include "ast.h"

#define LEH_MAXNAMELEN                            512
#define LEH_ERRARGLEN                             256

typedef struct str_error_message *error_message;

struct str_error_message
{
    int type;
    int id;
    const char *msg;
    const char *legend;
    int cnt;
};

#define ERR_UNDEFINED_TYPE                          1
#define ERR_UNDEFINED_IDENTIFIER                    2
#define ERR_REDEFINED_IDENTIFIER                    3
#define ERR_REDEFINED_ATTRIBUTE                     4
#define ERR_TOO_MANY_ARGUMENTS_FUNCTION             5
#define ERR_TOO_MANY_ARGUMENTS_SYSTEM               6
#define ERR_TOO_FEW_ARGUMENTS_FUNCTION              7
#define ERR_TOO_FEW_ARGUMENTS_SYSTEM                8
#define ERR_INCOMPATIBLE_ARGUMENT_TYPE_FUNCTION     9
#define ERR_INCOMPATIBLE_ARGUMENT_TYPE_SYSTEM      10
#define ERR_INCOMPATIBLE_RETURN_TYPE               11
#define ERR_EVALUATE_ATTRIBUTE_ERROR               12
#define ERR_EVALUATE_ATTRIBUTE_ENUM_ERROR          13
#define ERR_INCOMPATIBLE_INITIALIZER_TYPE          14
#define ERR_TYPE_ARITH_IF_COND                     15
#define ERR_TYPE_ARITH_IF_VALS                     16
#define ERR_TYPE_BINOP                             17
#define ERR_TYPE_UNOP                              18
#define ERR_TYPE_ENUM_NOSEL                        19
#define ERR_TYPE_ENUM_BADSEL                       20
#define ERR_TYPE_BADSEL                            21
#define ERR_MISSING_DEFAULT                        22
#define ERR_TYPE_COND                              23
#define ERR_TYPE_CAST                              28
#define ERR_ALREADY_INSTANTIATED                   29
#define ERR_TYPE_ATTR                              30
#define ERR_TYPE_CIRCULAR                          31
#define ERR_TYPE_CIRCULAR_REPORT                   32
#define ERR_REDEFINED_INT_ATTR                     33
#define ERR_TYPE_ARRAY_DIMS                        34
#define ERR_TYPE_SUBSCRIPT                         35
#define ERR_TYPE_OUT_OF_RANGE                      36
#define ERR_REDEFINED_STRUCT_VALUE                 37
#define ERR_TYPE_NOT_STRUCT                        38
#define ERR_TYPE_STRUCT_NO_ENTRY                   39
#define ERR_ALREADY_INSTANTIATED_ARRAY             40
#define ERR_TYPE_EXTENT_SIZE                       41
#define ERR_TYPE_ARRAY_DIMS_INSTANTIATION          42
#define ERR_TYPE_SEQUENCE                          43
#define ERR_EMPTY_SEQUENCE                         44
#define ERR_MISSING_PROBABILITIES                  45
#define ERR_TYPE_NOT_ARRAY                         46
#define ERR_TYPE_ARRAY_DIMS_DECLARATION            47
#define ERR_TYPE_BINOP_SIZES                       48
#define ERR_BAD_ARGUMENT_SIZE_FUNCTION             49
#define ERR_BAD_ARGUMENT_SIZE_SYSTEM               50
#define ERR_SELF_REFERENCE                         51
#define ERR_TYPE_PREDICATE                         52
#define ERR_TYPE_IF_COND                           53

#define WARN_UNUSED_VARIABLE                     1001
#define WARN_UNUSED_SYSTEM                       1002
#define WARN_REDEFINED_ENUM_VALUE                1003
#define WARN_CONST_HEALTH                        1004
#define WARN_NON_CONST_OBSERVABLE                1005
#define WARN_BAD_PROBABILITY                     1006
#define WARN_BAD_PROBABILITY_ENTRY               1007
#define WARN_UNUSED_SYSTEM_ARRAY                 1008
#define WARN_UNUSED_SYSTEM_ARRAY_ALL             1009
#define WARN_UNUSED_VARIABLE_ARRAY               1010
#define WARN_UNUSED_VARIABLE_ARRAY_ELEMENT       1011
#define WARN_UNUSED_VARIABLE_STRUCTURE           1012
#define WARN_UNUSED_VARIABLE_STRUCTURE_MEMBER    1013

#define INTERNAL_UNSUPPORTED_EXPR                2001
#define INTERNAL_BRACKET_LEVEL                   2002

#define COMPILER_OPEN_ERROR                      3001
#define COMPILER_INCLUDE_ERROR                   3002

#define LEH_LOCATION_GLOBAL                         2
#define LEH_LOCATION_FUNCTION                       1
#define LEH_LOCATION_SYSTEM                         0

#define LEH_ERR                                     0
#define LEH_WARN                                    1
#define LEH_INTERNAL                                2
#define LEH_COMPILER                                3

/* Function prototypes: */
extern void leh_error(int, int, const_origin, ...);
extern void lc_parser_error(const char *, int, const char *, ...);

#endif
