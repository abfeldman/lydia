#include <stdlib.h>
#include <assert.h>

#include "config.h"
#include "stat.h"
#include "ltms.h"
#include "defs.h"
#include "tv.h"

/* Static function prototypes: */
static void ltms_literal_free(ltms_literal);

static ltms_literal ltms_literal_new(ltms engine,
                                     const int nodeidx,
                                     const signed char sign)
{
    ltms_literal literal = (ltms_literal)malloc(sizeof(struct str_ltms_literal));
    if (NULL == literal) {
        return literal;
    }

    literal->proposition = engine->nodes->arr[nodeidx];
    literal->sign = sign;

    return literal;
}

static ltms_literal ltms_literal_copy(ltms_literal literal)
{
    ltms_literal result = (ltms_literal)malloc(sizeof(struct str_ltms_literal));
    if (NULL == result) {
        return result;
    }

    result->proposition = literal->proposition;
    result->sign = literal->sign;

    return result;
}

static ltms_clause ltms_clause_new()
{
    ltms_clause clause = (ltms_clause)malloc(sizeof(struct str_ltms_clause));
    if (NULL == clause) {
        return NULL;
    }
    memset(clause, 0, sizeof(struct str_ltms_clause));

    if (NULL == (clause->literals = array_new((array_element_destroy_func_t)ltms_literal_free, NULL))) {
        free(clause);
        return NULL;
    }

    return clause;
}

static void ltms_literal_free(ltms_literal literal)
{
    free(literal);
}

static void ltms_node_free(ltms_node node)
{
    array_free(node->pos_clauses);
    array_free(node->neg_clauses);

    free(node);
}

static void ltms_clause_free(ltms_clause clause)
{
    array_free(clause->literals);

    free(clause);
}

/* Traversals of clause literals: */
static ltms_literal find_unassigned_literal(ltms_clause clause)
{
    register unsigned int ix;

    for (ix = 0; ix < clause->literals->sz; ix++) {
        ltms_literal lit = (ltms_literal)clause->literals->arr[ix];
        if (ltms_node_is_unknown(lit->proposition)) {
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

static ltms_node find_satisfying_node(ltms_clause clause)
{
    register unsigned int ix;

    for (ix = 0; ix < clause->literals->sz; ix++) {
        ltms_literal lit = (ltms_literal)clause->literals->arr[ix];
        if ((lit->sign && ltms_node_is_true(lit->proposition)) ||
            (!lit->sign && ltms_node_is_false(lit->proposition))) {
            return lit->proposition;
        }
    }
/* The clause is not satisfied. */
    return NULL;
}

static void set(ltms engine, ltms_node node, signed char sign)
{
    register unsigned int ix;

    array opposite;
    array same;

    assert(ltms_node_is_unknown(node));

    if (sign) {
        ltms_node_set_true(node);

        opposite = node->neg_clauses;
        same = node->pos_clauses;
    } else {
        ltms_node_set_false(node);

        opposite = node->pos_clauses;
        same = node->neg_clauses;
    }

    if ((!sign && ltms_node_is_enabled_true(node)) ||
        (sign && ltms_node_is_enabled_false(node))) {
        engine->conflicts += 1;
    }

    for (ix = 0; ix < opposite->sz; ix++) {
        ltms_clause clause = (ltms_clause)opposite->arr[ix];
        clause->pvs -= 1;
        if (NULL != clause->satisfying_node) {
/* The clause is satisfied. */
            continue;
        }
        if (clause->pvs == 0) {
            engine->conflicts += 1;
        }
        if (clause->pvs == 1) {
            if (!ltms_clause_has_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN)) {
                ltms_clause_set_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN);
                stack_push(engine->unit_open, clause);
            }
        }
    }
    for (ix = 0; ix < same->sz; ix++) {
        ltms_clause clause = (ltms_clause)same->arr[ix];
        if (clause->satisfying_node == NULL) {
/* The clause was not satisfied, now it will be. */
            clause->satisfying_node = node;
        }
    }
}

static void unset(ltms engine, ltms_node node)
{
    register unsigned int ix;

    ltms_clause clause;

    array opposite;
    array same;

    signed char sign;

    if (ltms_node_is_unknown(node)) {
        return;
    }

    if (ltms_node_is_true(node)) {
        opposite = node->neg_clauses;
        same = node->pos_clauses;

        sign = 1;
    } else {
        assert(ltms_node_is_false(node));

        opposite = node->pos_clauses;
        same = node->neg_clauses;

        sign = 0;
    }
    ltms_node_set_unknown(node);

    if ((!sign && ltms_node_is_enabled_false(node)) ||
        (sign && ltms_node_is_enabled_true(node))) {
        stack_push(engine->unset_nodes, node);
    }
    if ((!sign && ltms_node_is_enabled_true(node)) ||
        (sign && ltms_node_is_enabled_false(node))) {
        engine->conflicts -= 1;

        stack_push(engine->unset_nodes, node);
    }

    for (ix = 0; ix < opposite->sz; ix++) {
        clause = (ltms_clause)opposite->arr[ix];
        clause->pvs += 1;
        if (clause->pvs == 1 && clause->satisfying_node == NULL) {
            engine->conflicts -= 1;
/* Push in the unit open fringe. */
            if (!ltms_clause_has_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN)) {
                ltms_clause_set_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN);
                stack_push(engine->unit_open, clause);
            }
        }
        if (clause->supported_node != NULL) {
            stack_push(engine->undo_nodes, clause->supported_node);
            clause->supported_node->supporting_clause = NULL;
            clause->supported_node = NULL;
        }
    }
    for (ix = 0; ix < same->sz; ix++) {
        clause = (ltms_clause)same->arr[ix];
        if (node == clause->satisfying_node) {
/* Find an alternative satisfying node if such exits. */
            clause->satisfying_node = find_satisfying_node(clause);
            if (clause->pvs == 1 && clause->satisfying_node == NULL) {
                if (!ltms_clause_has_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN)) {
                    ltms_clause_set_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN);
                    stack_push(engine->unit_open, clause);
                }
            }
        }
    }
}

static void bcp(ltms engine)
{
    ltms_clause clause;

    ltms_literal lit;

    while ((clause = (ltms_clause)stack_pop(engine->unit_open)) != NULL) {
        ltms_clause_clear_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN);
/* The clause could be in the meanwhile satisfied or modified not unit open. */
        if (clause->pvs == 1 && clause->satisfying_node == NULL) {
            lit = find_unassigned_literal(clause);

            clause->supported_node = lit->proposition;
            lit->proposition->supporting_clause = clause;

            set(engine, lit->proposition, lit->sign);
/* @todo: figure out what is this */
            if (engine->conflicts > 0) {
                break;
            }
        }
    }
}

ltms ltms_new()
{
    ltms engine = (ltms)malloc(sizeof(struct str_ltms));
    if (NULL == engine) {
        return NULL;
    }

    engine->conflicts = 0;

    engine->nodes = array_new((array_element_destroy_func_t)ltms_node_free, NULL);
    engine->clauses = array_new((array_element_destroy_func_t)ltms_clause_free, NULL);

    engine->unit_open = stack_new(NULL, NULL);
    engine->undo_nodes = stack_new(NULL, NULL);
    engine->unset_nodes = stack_new(NULL, NULL);

    return engine;
}

void ltms_free(ltms engine)
{
    stack_free(engine->unset_nodes);
    stack_free(engine->undo_nodes);
    stack_free(engine->unit_open);

    array_free(engine->clauses);
    array_free(engine->nodes);

    free(engine);
}

int ltms_add_node(ltms engine,
                  const int nodeidx,
                  const signed char is_assumable,
                  const signed char is_premise)
{
    ltms_node node = (ltms_node)malloc(sizeof(struct str_ltms_node));
    if (NULL == node) {
        return 0;
    }
    memset(node, 0, sizeof(struct str_ltms_node));

    assert(nodeidx == (int)engine->nodes->sz);

    node->index = nodeidx;

    if (is_assumable) {
        assert(!is_premise);

        ltms_node_set_flag(node, LTMS_NODE_FG_ASSUMPTION);
    }
    if (is_premise) {
        assert(!is_assumable);

        ltms_node_set_flag(node, LTMS_NODE_FG_PREMISE);
    }

    node->pos_clauses = array_new(NULL, NULL);
    node->neg_clauses = array_new(NULL, NULL);

    array_append(engine->nodes, node);

    return 1;
}

int ltms_add_clause(ltms engine, const_tv_clause cl)
{
    register unsigned int ix;

    const_int_list pos = cl->pos;
    const_int_list neg = cl->neg;

    ltms_clause clause;
    ltms_literal lit;

    if (NULL == (clause = ltms_clause_new())) {
        return 0;
    }

    clause->pvs = pos->sz + neg->sz;

    for (ix = 0; ix < pos->sz; ix++) {
        lit = ltms_literal_new(engine, pos->arr[ix], LTMS_LITERAL_TRUE);

        array_append(clause->literals, lit);
        array_append(lit->proposition->pos_clauses, clause);

        if (ltms_node_is_true(lit->proposition)) {
            if (NULL == clause->satisfying_node) {
                clause->satisfying_node = lit->proposition;
            }
        } else if (ltms_node_is_false(lit->proposition)) {
            clause->pvs -= 1;
        } else {
            assert(ltms_node_is_unknown(lit->proposition));
        }
    }
    for (ix = 0; ix < neg->sz; ix++) {
        lit = ltms_literal_new(engine, neg->arr[ix], LTMS_LITERAL_FALSE);

        array_append(clause->literals, lit);
        array_append(lit->proposition->neg_clauses, clause);

        if (ltms_node_is_false(lit->proposition)) {
            if (NULL == clause->satisfying_node) {
                clause->satisfying_node = lit->proposition;
            }
        } else if (ltms_node_is_true(lit->proposition)) {
            clause->pvs -= 1;
        } else {
            assert(ltms_node_is_unknown(lit->proposition));
        }
    }

    array_append(engine->clauses, clause);

    if (NULL == clause->satisfying_node) {
        if (clause->pvs == 0) {
            engine->conflicts += 1;
        }
        if (clause->pvs == 1) {
            if (!ltms_clause_has_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN)) {
                ltms_clause_set_flag(clause, LTMS_CLAUSE_FG_UNIT_OPEN);
                stack_push(engine->unit_open, clause);
            }
            bcp(engine);
        }
    }

    return 1;
}

int ltms_has_contradiction(ltms engine)
{
    return engine->conflicts > 0;
}

int ltms_enable_assumption(ltms engine,
                           const int nodeidx,
                           const signed char sign)
{
    ltms_node node = (ltms_node)engine->nodes->arr[nodeidx];
/*
    assert(ltms_node_has_flag(node, LTMS_NODE_FG_ASSUMPTION) ||
           ltms_node_has_flag(node, LTMS_NODE_FG_PREMISE));
*/
    if ((sign && ltms_node_has_flag(node, LTMS_NODE_FG_ENABLED_TRUE)) ||
        (!sign && ltms_node_has_flag(node, LTMS_NODE_FG_ENABLED_FALSE))) {
        return 1;
    }

    ltms_node_set_flag(node, sign ? LTMS_NODE_FG_ENABLED_TRUE : LTMS_NODE_FG_ENABLED_FALSE);

    if (!ltms_node_is_unknown(node)) {
        if ((sign && ltms_node_is_false(node)) ||
            (!sign && ltms_node_is_true(node))) {
            engine->conflicts += 1;
        }
        return 1;
    }

    set(engine, node, sign);
    bcp(engine);

    return 1;
}

void ltms_retract_assumption(ltms engine, const int nodeidx)
{
    ltms_node node;

    node = (ltms_node)engine->nodes->arr[nodeidx];

    if (ltms_node_is_unknown(node)) {
        return;
    }
    if ((ltms_node_is_true(node) && ltms_node_is_enabled_false(node)) ||
        (ltms_node_is_false(node) && ltms_node_is_enabled_true(node))) {
        engine->conflicts -= 1;
    }

    ltms_node_clear_flag(node, LTMS_NODE_FG_ENABLED_FALSE);
    ltms_node_clear_flag(node, LTMS_NODE_FG_ENABLED_TRUE);

    unset(engine, node);
    while ((node = (ltms_node)stack_pop(engine->undo_nodes)) != NULL) {
        unset(engine, node);
    }
    while ((node = (ltms_node)stack_pop(engine->unset_nodes)) != NULL) {
        assert(ltms_node_is_enabled_false(node) ||
               ltms_node_is_enabled_true(node));

        if (ltms_node_is_unknown(node)) {
            set(engine, node, ltms_node_is_enabled_false(node) ? 0 : 1);
        }
    }
    bcp(engine);
}

array ltms_get_conflict_assumptions(ltms engine)
{
    register unsigned int ix;
    register unsigned int iy;

    ltms_literal literal = NULL;
    ltms_clause clause = NULL;
    ltms_node node = NULL;

    array result;
    stack nodes;

    signed char *buf;

    result = array_new((array_element_destroy_func_t)ltms_literal_free,
                       (array_element_clone_func_t)ltms_literal_copy);

    buf = (signed char *)malloc(sizeof(signed char) * engine->nodes->sz);
    if (buf == NULL) {
        array_free(result);

        return NULL;
    }
    memset(buf, 0, sizeof(signed char) * engine->nodes->sz);

    nodes = stack_new(NULL, NULL);

    for (ix = 0; ix < engine->clauses->sz; ix++) {
        clause = (ltms_clause)engine->clauses->arr[ix];
        if (clause->pvs == 0 && clause->satisfying_node == NULL) {
            for (iy = 0; iy < clause->literals->sz; iy++) {
                literal = clause->literals->arr[iy];
                stack_push(nodes, literal->proposition);
            }
        }
    }

    for (ix = 0; ix < engine->nodes->sz; ix++) {
        node = (ltms_node)engine->nodes->arr[ix];
        if ((ltms_node_is_true(node) && ltms_node_is_enabled_false(node)) ||
            (ltms_node_is_false(node) && ltms_node_is_enabled_true(node))) {
            if (node->supporting_clause != NULL) {
                stack_push(nodes, node);
            }
        }
    }

    while ((node = (ltms_node)stack_pop(nodes)) != NULL) {
        if (buf[node->index]) {
            continue;
        }
        buf[node->index] = 1;

        if (ltms_node_has_flag(node, LTMS_NODE_FG_ASSUMPTION)) {
            array_append(result,
                         ltms_literal_new(engine,
                                          node->index,
                                          LTMS_LITERAL_FALSE));
            continue;
        }
        if (node->supporting_clause != NULL &&
            node->supporting_clause->satisfying_node != NULL && 
            node->supporting_clause->satisfying_node != node) {
            stack_push(nodes, node->supporting_clause->satisfying_node);
        }
    }
    stack_free(nodes);

    free(buf);

    return result;
}

ltms_state ltms_state_get(const ltms engine)
{
    register unsigned int ix;

    ltms_state state = (ltms_state)malloc(sizeof(struct str_ltms_state));
    if (NULL == state) {
        return NULL;
    }

    memset(state, 0, sizeof(struct str_ltms_state));

    if (NULL == (state->unit_open = stack_copy(engine->unit_open)) ||
        NULL == (state->undo_nodes = stack_copy(engine->undo_nodes)) ||
        NULL == (state->unset_nodes = stack_copy(engine->unset_nodes))) {
        ltms_state_free(state);
    }

    state->conflicts = engine->conflicts;

    if (NULL == (state->clauses_pvs = (unsigned int *)malloc(engine->clauses->sz * sizeof(unsigned int))) ||
        NULL == (state->clauses_satisfying_node = (ltms_node *)malloc(engine->clauses->sz * sizeof(ltms_node))) ||
        NULL == (state->clauses_supported_node = (ltms_node *)malloc(engine->clauses->sz * sizeof(ltms_node))) ||

        NULL == (state->nodes_flags = (signed char *)malloc(engine->nodes->sz * sizeof(signed char))) ||
        NULL == (state->nodes_supporting_clause = (ltms_clause *)malloc(engine->nodes->sz * sizeof(ltms_clause)))) {
        ltms_state_free(state);
    }

    for (ix = 0; ix < engine->clauses->sz; ix++) {
        ltms_clause clause = engine->clauses->arr[ix];

        state->clauses_pvs[ix] = clause->pvs;
        state->clauses_satisfying_node[ix] = clause->satisfying_node;
        state->clauses_supported_node[ix] = clause->supported_node;
    }

    for (ix = 0; ix < engine->nodes->sz; ix++) {
        ltms_node node = engine->nodes->arr[ix];

        state->nodes_flags[ix] = node->flags;
        state->nodes_supporting_clause[ix] = node->supporting_clause;
    }

    return state;
}

void ltms_state_free(ltms_state state)
{
    if (NULL != state->unit_open) {
        stack_free(state->unit_open);
    }
    if (NULL != state->undo_nodes) {
        stack_free(state->undo_nodes);
    }
    if (NULL != state->unset_nodes) {
        stack_free(state->unset_nodes);
    }

    if (NULL != state->clauses_pvs) {
        free(state->clauses_pvs);
    }
    if (NULL != state->clauses_satisfying_node) {
        free(state->clauses_satisfying_node);
    }
    if (NULL != state->clauses_supported_node) {
        free(state->clauses_supported_node);
    }

    if (NULL != state->nodes_flags) {
        free(state->nodes_flags);
    }
    if (NULL != state->nodes_supporting_clause) {
        free(state->nodes_supporting_clause);
    }
}

void ltms_state_restore(ltms engine, ltms_state state)
{
    register unsigned int ix;

    stack_free(engine->unit_open);
    stack_free(engine->undo_nodes);
    stack_free(engine->unset_nodes);

    engine->unit_open = stack_copy(state->unit_open);
    engine->undo_nodes = stack_copy(state->undo_nodes);
    engine->unset_nodes = stack_copy(state->unset_nodes);

    for (ix = 0; ix < engine->clauses->sz; ix++) {
        ltms_clause clause = engine->clauses->arr[ix];

        clause->pvs = state->clauses_pvs[ix];
        clause->satisfying_node = state->clauses_satisfying_node[ix];
        clause->supported_node = state->clauses_supported_node[ix];
    }

    for (ix = 0; ix < engine->nodes->sz; ix++) {
        ltms_node node = engine->nodes->arr[ix];

        node->flags = state->nodes_flags[ix];
        node->supporting_clause = state->nodes_supporting_clause[ix];
    }

    engine->conflicts = state->conflicts;
}
