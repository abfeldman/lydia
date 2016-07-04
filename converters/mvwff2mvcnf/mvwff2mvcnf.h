/* filename:     mvwff2mvcnf.h
 * description:  functions for converting from mv_wff
 * author:       Tom Janssen (TU Delft)
 */

#ifndef __MVWFF2MVCNF_H__
#define __MVWFF2MVCNF_H__

#include "mv.h"

extern void mvwff2mvcnf_eliminate_implications(mv_wff_expr*);
/* eliminates both equivalences and implications
 * introduces conjunctions, disjunctions and negations
 */

extern void mvwff2mvcnf_reduce_negation_scopes(mv_wff_expr*);
/* pushes the negations inside the expressions
 * introduces conjunstions and disjunctions (using de Morgan's laws)
 */

extern void mvwff2mvcnf_distribute_disjunctions(mv_wff_expr*);
/* creates conjunctions of disjunctions (using distributive laws)
 */

extern void mvwff2mvcnf_eliminate_constants(mv_wff_expr*);
/* eliminates constants in the expressions 
 */

extern serializable mvwff2mvcnf(const_serializable);
/* converts a mv_wff to a mv_cnf
 * converts a mv_wff_hierarchy to a mv_cnf_hierarchy
 */

extern serializable mvwff2mvdnf(const_serializable);


#endif

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
