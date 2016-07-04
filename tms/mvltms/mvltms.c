/* filename:     mvltms.c
 * description:  multivalued Truth Maintenance System
 * author:       Tom Janssen (TU Delft)
 */

#include <stdlib.h>
#include <assert.h>

#include "mv.h"
#include "mvltms.h"
#include "config.h"

/* mvclause2posmvclause function (not sure about keeping it here */
void mvclause2posmvclause(mv_clause cl, variable_list variables, values_set_list domains)
{
    unsigned int ix, iy, iz;
    for(ix=0; ix<cl->neg->sz; ix++) {
	int var = cl->neg->arr[ix]->var;
	int negval = cl->neg->arr[ix]->val;
	unsigned int enumsize = 0;
	if(variables->arr[var]->tag != TAGenum_variable) {
	    fprintf(stderr, "encountered variable type other than enum\n");
	    return;
	}
	enumsize = domains->arr[to_enum_variable(variables->arr[var])->values_set]->entries->sz;
	for(iy=0; iy<enumsize; iy++) {
	    int inpos = 0;
	    for(iz=0; iz<cl->pos->sz; iz++)
		if(cl->pos->arr[iz]->var==var &&
			cl->pos->arr[iz]->val==(int)iy) {
		    inpos = 1;
		    break;
		}
	    if(inpos)
		continue;
	    if((int)iy!=negval)
		append_mv_literal_list(cl->pos, new_mv_literal(var, iy));
	}
    }
    rfre_mv_literal_list(cl->neg);
    cl->neg = new_mv_literal_list();
}

/* Static function prototypes: */
static void mvltms_literal_free(mvltms_literal);

static mvltms_literal mvltms_literal_new(mvltms engine,
				     const int nodeidx,
				     const int value)
{
    mvltms_literal literal = (mvltms_literal)malloc(sizeof(struct str_mvltms_literal));
    if (NULL == literal) {
	return literal;
    }

    literal->proposition = engine->nodes->arr[nodeidx];
    literal->value = value;

    return literal;
}

static mvltms_literal mvltms_literal_copy(mvltms_literal literal)
{
    mvltms_literal result = (mvltms_literal)malloc(sizeof(struct str_mvltms_literal));
    if (NULL == result) {
	return result;
    }

    result->proposition = literal->proposition;
    result->value = literal->value;

    return result;
}

static mvltms_clause mvltms_clause_new()
{
    mvltms_clause clause = (mvltms_clause)malloc(sizeof(struct str_mvltms_clause));
    if (NULL == clause) {
	return clause;
    }
    memset(clause, 0, sizeof(struct str_mvltms_clause));

    if (NULL == (clause->literals = array_new((array_element_destroy_func_t)mvltms_literal_free, NULL))) {
	free(clause);
	return NULL;
    }

    clause->unit_open_index = -1;
    clause->violated_index = -1;

    clause->conflicting_node = NULL;

    return clause;
}

static void mvltms_literal_free(mvltms_literal literal)
{
    free(literal);
}

static void mvltms_node_free(mvltms_node node)
{
    array_free(node->clauses);

    free(node);
}

static void mvltms_clause_free(mvltms_clause clause)
{
    array_free(clause->literals);

    free(clause);
}

/* Traversals of clause literals: */
static mvltms_literal find_unassigned_literal(mvltms_clause clause)
{
    register unsigned int ix;

    for (ix = 0; ix < clause->literals->sz; ix++) {
	mvltms_literal lit = (mvltms_literal)clause->literals->arr[ix];
	if (MVLTMS_NODE_LABEL_UNASSIGNED == lit->proposition->label) {
	    return lit;
	}
    }
/*
 * Never reached. This function should be called on unit open clauses
 * only.
 */
    assert(0);
    abort();
}

static mvltms_node find_satisfying_node(mvltms_clause clause)
{
    register unsigned int ix;

    /* return an assigned node which corresponds with a literal in this clause */
    for (ix = 0; ix < clause->literals->sz; ix++) {
	mvltms_literal lit = (mvltms_literal)clause->literals->arr[ix];
	if (MVLTMS_NODE_LABEL_ASSIGNED == lit->proposition->label &&
		lit->proposition->value == lit->value) {
	    return lit->proposition;
	}
    }
    /* The clause is not satisfied. */
    return NULL;
}

void print_status(mvltms engine)
{
    mvltms_node node;
    mvltms_clause clause;
    unsigned int ix, iy, iz;

    fprintf(stderr, " MVLTMS status:\n");
    fprintf(stderr, " index: %i\n", engine->index);
    fprintf(stderr, " level: %i\n", engine->level);
    for(ix=0; ix<engine->index; ix++)
    {
	for(iy=0; iy<engine->nodes->sz; iy++)
	{
	    if(((mvltms_node)engine->nodes->arr[iy])->assign_index==ix)
	    {
		node = (mvltms_node)engine->nodes->arr[iy];
		fprintf(stderr, " %i: %i l=%i  [", ix, node->index, node->assume_level);
		for(iz=0; iz<node->clauses->sz; iz++)
		{
		    clause = (mvltms_clause)node->clauses->arr[iz];
		    if(clause->supported_node)
    			fprintf(stderr, "%i ", clause->supported_node->index);
		}
		fprintf(stderr, "]\n");
	    }
	}
    }
}

static void set(mvltms engine, mvltms_node node, int value)
{
    register unsigned int ix, iy;

    array clauses;

    assert(MVLTMS_NODE_LABEL_UNASSIGNED == node->label);

    /* assign node with value */
    node->label = MVLTMS_NODE_LABEL_ASSIGNED;
    node->value = value;

    node->assume_level = engine->level;
    node->assign_index = engine->index++;

    clauses = node->clauses;

    /* for each clause in nodes referring clauses list,
     *  substract nr of occurring node literals from nr of unassigned literals
     *  if clause is not yet satisfied and literal has same value as node
     *   then node satisfies clause
     *  if all literals have different values than node and clause is
     *   not yet satisfied and no other literals left, then clause is violated
     *  if all literals in clause are checked and clause is not satisfied and
     *   one literal is left, then clause is unit clause */
    for (ix = 0; ix < clauses->sz; ix++) {
	mvltms_clause clause = (mvltms_clause)clauses->arr[ix];
	/* clause can have more than one literal of the same mv var */
	for (iy = 0; iy < clause->literals->sz; iy++) {
	    mvltms_literal lit = clause->literals->arr[iy];
	    if(lit->proposition == node) {
		clause->unassigned -= 1;
		if(lit->value == node->value) {
		    if(NULL == clause->satisfying_node)
			clause->satisfying_node = node;
		}
	    }
	}
	if(NULL == clause->satisfying_node) {
	    if(0 == clause->unassigned) {
		/* only record first conflict */
		if(engine->violated->sz==0)
		    clause->conflicting_node = node;
		clause->violated_index = engine->violated->sz;
		stack_push(engine->violated, clause);
	    } else if(1 == clause->unassigned) {
		clause->unit_open_index = engine->unit_open->sz;
		stack_push(engine->unit_open, clause);
	    }
	}
    }
}

void remove_from_violated(mvltms engine, mvltms_clause clause)
{
    assert(engine->violated->sz > 0);
    if (clause->violated_index == engine->violated->sz - 1) {
	stack_pop(engine->violated);
    } else {
	mvltms_clause top_clause = stack_pop(engine->violated);
	engine->violated->arr[clause->violated_index] = top_clause;
	top_clause->violated_index = clause->violated_index;
    }
    clause->violated_index = -1;

    clause->conflicting_node = NULL;
}

void remove_from_unit_open(mvltms engine, mvltms_clause clause)
{
    assert(engine->unit_open->sz > 0);
    if (clause->unit_open_index == engine->unit_open->sz - 1) {
	stack_pop(engine->unit_open);
    } else {
	mvltms_clause top_clause = stack_pop(engine->unit_open);
	engine->unit_open->arr[clause->unit_open_index] = top_clause;
	top_clause->unit_open_index = clause->unit_open_index;
    }
    clause->unit_open_index = -1;
}

static void unset(mvltms engine, mvltms_node node)
{
    register unsigned int ix, iy;

    array clauses;

    if (MVLTMS_NODE_LABEL_UNASSIGNED == node->label) {
	return;
    }

    /* label node to be unassigned */
    node->label = MVLTMS_NODE_LABEL_UNASSIGNED;

    node->assume_level = -1;
    node->assign_index = -1;
    engine->index--;

    clauses = node->clauses;

    /* for each clause in nodes referring clauses list,
     *  if clause was violated
     *   then remove it from violated list (because this node could satisfy it)
     *  else if clause was in unit open list
     *   then remove it from the list
     *  else if clause supported node
     *   then put that node on undo stack
     *   and stop supporting it
     *  if clause satisfied this node
     *   then find another node that is assigned and satisfied by clause
     *  increase nr of unassigned literals with nr of occurring node literals
     *  check if unit open now */
    for (ix = 0; ix < clauses->sz; ix++) {
	mvltms_clause clause = (mvltms_clause) clauses->arr[ix];
	if ((unsigned int)-1 != clause->violated_index) {
	    remove_from_violated(engine, clause);
	    clause->unit_open_index = engine->unit_open->sz;
	    stack_push(engine->unit_open, clause);
	} else if ((unsigned int)-1 != clause->unit_open_index) {
	    remove_from_unit_open(engine, clause);
	} else if (NULL != clause->supported_node) {
	    stack_push(engine->undo_nodes, clause->supported_node);
	    assert(clause==clause->supported_node->supporting_clause);
	    clause->supported_node->supporting_clause = NULL;
	    clause->supported_node = NULL;
	}
	if (node == clause->satisfying_node) {
	    clause->satisfying_node = find_satisfying_node(clause);
	}
	for (iy = 0; iy < clause->literals->sz; iy++) {
	    mvltms_literal lit = clause->literals->arr[iy];
	    if(lit->proposition == node) {
		clause->unassigned += 1;
	    }
	}
	if(NULL == clause->satisfying_node) {
	    if(1 == clause->unassigned) {
		clause->unit_open_index = engine->unit_open->sz;
		stack_push(engine->unit_open, clause);
	    }
	}
    }
}

static void bcp(mvltms engine)
{
    /* for each unit clause,
     *  set the proposition node of the unassigned literal
     *  to the literals value */
    while (!stack_empty(engine->unit_open)) {
	mvltms_clause clause = (mvltms_clause)stack_pop(engine->unit_open);
	assert(NULL != clause);

	clause->unit_open_index = -1;
	if (1 == clause->unassigned) {
	    mvltms_literal lit = find_unassigned_literal(clause);
	    assert(NULL != lit);

	    /* if the node is set with the one unassigned literals value
	     * then the node is supported by this clause */
	    assert(clause->supported_node==NULL);
	    clause->supported_node = lit->proposition;
	    lit->proposition->supporting_clause = clause;

	    set(engine, lit->proposition, lit->value);
	}
    }
}

static void undo_propagations(mvltms engine, mvltms_node node)
{
    unset(engine, node);
    while (!stack_empty(engine->undo_nodes)) {
	node = (mvltms_node)stack_pop(engine->undo_nodes);
	assert(NULL != node);
	unset(engine, node);
    }
    bcp(engine);
}

mvltms mvltms_new(variable_list variables, values_set_list domains)
{
    mvltms engine = (mvltms)malloc(sizeof(struct str_mvltms));
    if (NULL == engine) {
	return engine;
    }

    engine->nodes = array_new((array_element_destroy_func_t)mvltms_node_free, NULL);
    engine->clauses = array_new((array_element_destroy_func_t)mvltms_clause_free, NULL);

    engine->unit_open = stack_new(NULL, NULL);
    engine->violated = stack_new(NULL, NULL);
    engine->undo_nodes = stack_new(NULL, NULL);
    engine->time_stamp = 0;
    engine->constructed = 0;

    engine->level = -1;
    engine->index = -1;

    /* should not be in mvltms
     * only here for getting rid of negative literals */
    engine->variables = variables;
    engine->domains = domains;

    return engine;
}

void mvltms_free(mvltms engine)
{
    stack_free(engine->undo_nodes);
    stack_free(engine->violated);
    stack_free(engine->unit_open);

    array_free(engine->clauses);
    array_free(engine->nodes);

    free(engine);
}

int mvltms_add_node(mvltms engine, const int nodeidx)
{
    mvltms_node node = (mvltms_node)malloc(sizeof(struct str_mvltms_node));

    if (NULL == node) {
	return 0;
    }
    memset(node, 0, sizeof(struct str_mvltms_node));

    assert(nodeidx == (int)engine->nodes->sz);

    node->index = nodeidx;
    node->label = MVLTMS_NODE_LABEL_UNASSIGNED;
    node->clauses = array_new(NULL, NULL);
    node->assumption_clause = NULL;

    node->assume_level = -1;
    node->assign_index = -1;

    array_append(engine->nodes, node);

    return 1;
}

int mvltms_add_clause(mvltms engine, mv_clause cl)
{
    register unsigned int ix, iy;

    const_mv_literal_list pos;
    const_mv_literal_list neg;
    
    mvltms_clause clause;
    mvltms_literal lit;

    mvclause2posmvclause(cl, engine->variables, engine->domains);
    pos = cl->pos;
    neg = cl->neg;

    /*TODO: check if engine still must be constructed before adding clauses
     * now that the literal's assumption clause is no longer index based */
    /*assert(0 == engine->constructed);*/

    /* This mvltms only supports positive literals */
    assert(0 == neg->sz);

    if (NULL == (clause = mvltms_clause_new())) {
	return 0;
    }

    clause->unassigned = pos->sz;

    for (ix = 0; ix < pos->sz; ix++) {
	int present;

	lit = mvltms_literal_new(engine, pos->arr[ix]->var, pos->arr[ix]->val);

	array_append(clause->literals, lit);
	
	/* add if not present */
	present=0;
	for(iy=0; iy<lit->proposition->clauses->sz; iy++)
	    if(lit->proposition->clauses->arr[iy]==clause)
		present=1;
	if(!present)
	    array_append(lit->proposition->clauses, clause);

	if (MVLTMS_NODE_LABEL_ASSIGNED == lit->proposition->label) {
	    clause->unassigned -= 1;
	    if (lit->value == lit->proposition->value) {
    		if (NULL == clause->satisfying_node) {
    		    clause->satisfying_node = lit->proposition;
		}
	    }
	    /* TODO: note clause of last conflicting literal, if clause is not satisfied
	     *  OR should we keep it NULL in case of clause adding? */
	} else {
	    assert(MVLTMS_NODE_LABEL_UNASSIGNED == lit->proposition->label);
	}
    }

    array_append(engine->clauses, clause);

    if (NULL == clause->satisfying_node) {
	if (0 == clause->unassigned) {
	    clause->violated_index = engine->violated->sz;
	    stack_push(engine->violated, clause);
	}
	if (1 == clause->unassigned) {
	    clause->unit_open_index = engine->unit_open->sz;
	    stack_push(engine->unit_open, clause);
	    bcp(engine);
	}
    }

    return 1;
}

int mvltms_has_contradiction(mvltms engine)
{
    return !stack_empty(engine->violated);
}

/* We can only assume a node has a value, not the complement */
int mvltms_enable_assumption(mvltms engine,
			   const int nodeidx,
			   const int value,
			   const signed char observation)
{
    mvltms_clause clause;
    mvltms_literal lit;

    /* engine is constructed, no more clauses or nodes may be added by user */
    engine->constructed = 1;

    /* add clause with one literal, setting var with value */
    if (NULL == (clause = mvltms_clause_new())) {
	return 0;
    }
    if (NULL == (lit = mvltms_literal_new(engine, nodeidx, value))) {
	mvltms_clause_free(clause);
	return 0;
    }
    array_append(clause->literals, lit);
    lit->proposition->assumption_clause = clause;
    array_append(engine->clauses, clause);

    /* add this clause to the proposition node */
    array_append(lit->proposition->clauses, clause);

    clause->unassigned = 1;
    clause->assumption = 1;
    clause->observation = observation;

    /* update proposition node
     *  if unassigned, put clause on unit open stack and propagate
     *  else, 
     *    if proposition node value is the same, clause is satisfied by node
     *    else there is a violation, push clause on violation stack */
    if (MVLTMS_NODE_LABEL_UNASSIGNED != lit->proposition->label) {
	clause->unassigned -= 1;
	if (lit->proposition->value == lit->value) {
	    clause->satisfying_node = lit->proposition;
	} else {
	    if(engine->violated->sz==0)
		clause->conflicting_node = lit->proposition;
	    clause->conflicting_node = lit->proposition;
	    clause->violated_index = engine->violated->sz;
	    stack_push(engine->violated, clause);
	}
    } else {
	clause->unit_open_index = engine->unit_open->sz;
	stack_push(engine->unit_open, clause);
	engine->level++;
	bcp(engine);
    }

    return 1;
}

void mvltms_retract_assumption(mvltms engine, const int nodeidx)
{
    register unsigned int iy;

    array clauses;
    mvltms_node node = (mvltms_node)engine->nodes->arr[nodeidx];
    mvltms_clause clause;
    mvltms_literal lit;

    if (MVLTMS_NODE_LABEL_UNASSIGNED == node->label || node->assumption_clause==NULL) {
/* The node's value is unknown or this is not an assumption node. */
	return;
    }

    engine->level--;

    clause = node->assumption_clause;

    assert(1 == clause->literals->sz);
    lit = clause->literals->arr[0];

    if ((unsigned int)-1 != clause->violated_index) {
	remove_from_violated(engine, clause);
    }

    clauses = lit->proposition->clauses;

    assert(clauses->sz > 0);
    for (iy = clauses->sz - 1; iy < clauses->sz; iy--) {
	if (clause == clauses->arr[iy]) {
	    array_delete(clauses, iy);
	}
    }
    assert(engine->clauses->sz > 0);
    
    mvltms_clause_free(clause);
    for(iy = engine->clauses->sz - 1; iy < engine->clauses->sz; iy--)
    {
	if (clause == engine->clauses->arr[iy])
	{
	    engine->clauses->arr[iy] = NULL; /* prevent double freeing */
	    array_delete(engine->clauses, iy);
	}
    }

    node->assumption_clause = NULL;

    undo_propagations(engine, node);
    
}

array mvltms_get_conflict_assumptions(mvltms engine)
{
    register unsigned int ix;

    mvltms_clause clause, support;

    array result = array_new(NULL, NULL);
    stack clauses = stack_new(NULL, NULL);
    mvltms_literal lit;

    assert(engine->violated->sz > 0);

    clause = engine->violated->arr[0];
    if (clause->assumption && !clause->observation) {
	lit = (mvltms_literal)clause->literals->arr[0];
	array_append(result, mvltms_literal_copy(lit));
    }
    engine->time_stamp += 1;
    do {
	for (ix = 0; ix < clause->literals->sz; ix++) {
	    lit = (mvltms_literal)clause->literals->arr[ix];
	    support = lit->proposition->supporting_clause;
	    if (support == clause ||
		engine->time_stamp == support->time_stamp) {
		continue;
	    }
	    support->time_stamp = engine->time_stamp;
	    if (support->assumption && !support->observation) {
		lit = (mvltms_literal)support->literals->arr[0];
		array_append(result, mvltms_literal_copy(lit));
	    } else {
		stack_push(clauses, support);
	    }
	}
    } while (NULL != (clause = stack_pop(clauses)));

    stack_free(clauses);

    return result;
}

mv_clause mvltms_get_conflict_learning_clause(mvltms engine, unsigned int *backjump_level)
{
    /*
     * return a clause which contains the cause of the current conflict
     * set backjump_level to the correct level to backtrack to
     */

    mv_clause result;
    mvltms_node node, resolve_node;
    mvltms_literal lit, litx;
    mvltms_clause clause;
    array resolved_literals;
    int n_falsified, n_resolutions;
    unsigned int ix, iy;

    /* if no conflict, return NULL and set backjump_level to (unsigned) -1 */
    if(engine==NULL || engine->violated->sz==0)
    {
	if(backjump_level)
	    *backjump_level = -1;
	return NULL;
    }

    clause = engine->violated->arr[0];
    if(clause->conflicting_node==NULL)
    {
	if(backjump_level)
	    *backjump_level = -1;
	return NULL;
    }

    resolved_literals = array_new((array_element_destroy_func_t)mvltms_literal_free, NULL);

    /* add violated clause literals to resolved_literals */
    for(ix=0; ix<clause->literals->sz; ix++)
    {
	lit = clause->literals->arr[ix];
	array_append(resolved_literals, mvltms_literal_copy(lit));
    }

    n_resolutions=0;
    while(1)
    {
    	/* find last of the falsified literals at current level 
	 *  in resolved literals 
	 */
    	resolve_node = NULL;
	n_falsified = 0;
    	for(ix=0; ix<resolved_literals->sz; ix++)
    	{
    	    lit = (mvltms_literal) resolved_literals->arr[ix];
    	    node = lit->proposition;
    	    assert(lit->value != node->value);
    	    if(node->assume_level==engine->level)
    	    {
    		n_falsified++;
    		if(resolve_node==NULL || resolve_node->assign_index<node->assign_index)
    		    resolve_node = node;
    	    }
    	}
	/* continue only if there are 2 or more falsified literals 
	 *  at current level in resolved_literals 
	 */
    	if(n_falsified > 1)
    	{
    	    /* clause is the reason why resolve_node was assigned with value */
    	    if(resolve_node->supporting_clause==NULL)
    		clause = resolve_node->assumption_clause;
    	    else
    		clause = resolve_node->supporting_clause;
    	    assert(clause!=NULL);
	    
    	    /* add to resolved_literals the literals of clause, deleting 
	     *  the literals of resolve_node (resolution)
	     */
	    for(ix=resolved_literals->sz-1; ix<resolved_literals->sz; ix--)
    	    {
    		lit = (mvltms_literal) resolved_literals->arr[ix];
    		if(lit->proposition==resolve_node)
    		    array_delete(resolved_literals, ix);
    	    }
    	    for(ix=0; ix<clause->literals->sz; ix++)
    	    {
    		lit = (mvltms_literal) clause->literals->arr[ix];
    		for(iy=0; iy<resolved_literals->sz; iy++)
    		{
    		    litx = (mvltms_literal) resolved_literals->arr[iy];
    		    if(litx->proposition==lit->proposition &&
    			    litx->value==lit->value)
    			break;
    		}
    		if(iy==resolved_literals->sz && lit->proposition!=resolve_node)
    		    array_append(resolved_literals, mvltms_literal_copy(lit));
    	    }
	    n_resolutions++;
    	}
	else
	    break;
    }

    /* find last of the falsified nodes at level < current level in 
     *  result clause 
     */
    *backjump_level = -1;
    resolve_node = NULL;
    n_falsified = 0;
    for(ix=0; ix<resolved_literals->sz; ix++)
    {
	lit = (mvltms_literal) resolved_literals->arr[ix];
	node = lit->proposition;
	assert(lit->value != node->value);
	if(node->assume_level<engine->level)
	{
	    if(resolve_node==NULL || resolve_node->assign_index<node->assign_index)
		resolve_node = node;
	}
	else
	    n_falsified++;
    }

    if(n_falsified!=1)
    {
	/* no falsified literal at current level left 
	 *  so there's no reason left to learn this clause now 
	 */
	if(backjump_level)
	    *backjump_level=-1;
	return NULL;
    }
    /* there's exactly 1 falsified literal in current level
     * set backjump_level to last falsified node from lower levels
     *  making result clause a unit open clause at that level
     */
    if(resolve_node!=NULL)
	*backjump_level = resolve_node->assume_level;

    /*printf("conflict learning clause:\n");*/
    result = new_mv_clause(new_mv_literal_list(), new_mv_literal_list());
    for(ix=0; ix<resolved_literals->sz; ix++)
    {
	lit = (mvltms_literal) resolved_literals->arr[ix];
	append_mv_literal_list(result->pos, new_mv_literal(lit->proposition->index, lit->value));
	/*printf(" %i=%i", lit->proposition->index, lit->value);*/
    }
    /*printf("\n");*/

    array_free(resolved_literals);
    return result;
}

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
