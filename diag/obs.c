#include "tv.h"
#include "obs.h"
#include "hash.h"
#include "defs.h"
#include "variable.h"
#include "hierarchy.h"

#include <assert.h>

static void free_term(void *bucket)
{
    tv_term term = *(tv_term *)bucket;

    rfre_tv_term(term);
}

static unsigned long name_hash_function(const char *key,
                                        unsigned int UNUSED(key_length))
{
	unsigned long h = 5381;

    const char *start = key;

	for (; *start != '\0'; start++) {
		h += (h << 5);
		h ^= (unsigned long)*start;
	}

	return h;
}

observations observations_new()
{
    observations context = malloc(sizeof(struct str_observations));
    if (NULL == context) {
        return NULL;
    }

    if (NULL == (context->observations = (hash_table *)malloc(sizeof(hash_table)))) {
        free(context);
        return NULL;
    }

    hash_init(context->observations, 2, name_hash_function, free_term);

    context->names = array_new(NULL, NULL);

    return context;
}

void observations_free(observations context)
{
    hash_destroy(context->observations);
    array_free(context->names);

    free(context->observations);
    free(context);
}

static array map_variables(const_variable_list variables,
                           const_variable_list model_variables)
{
    const_variable var;
    array map;

    register unsigned int ix;
    unsigned int pos;

    map = array_new(NULL, NULL);

    for (ix = 0; ix < variables->sz; ix++) {
        var = variables->arr[ix];
        if (!search_variable_list(model_variables, var->name, &pos)) {
            array_free(map);
            return NULL;
        }
        array_append(map, (void *)pos);
    }

    return map;
}

signed char observations_load(hierarchy dump,
                              const_variable_list model_variables,
                              observations *output)
{
    array map;

    tv_dnf dnf;
    tv_cnf cnf;

    lydia_symbol name;

    observations result;

    register unsigned int ix;
    register unsigned int iy;
    register unsigned int iz;

    result = observations_new();
    if (NULL == result) {
        return 1; /* Memory allocation error. */
    }

    for (ix = 0; ix < dump->nodes->sz; ix++) {
        tv_term obs;

        name = dump->nodes->arr[ix]->type;

        array_append(result->names, name);

        if ((dump->nodes->arr[ix]->edges == edge_listNIL) ||
            (dump->nodes->arr[ix]->edges->sz != 0)) {
            return 2; /* Not an observations file. */
        }

        if (dump->nodes->arr[ix]->constraints->tag == TAGtv_dnf) {
            dnf = to_tv_dnf(dump->nodes->arr[ix]->constraints);
            assert(dnf != tv_dnfNIL);
            assert(dnf->terms != tv_term_listNIL);
            assert(dnf->terms->sz == 1);

            map = map_variables(dnf->variables, model_variables);
            if (map == NULL) {
                return 3; /* Cannot map observations to model. */
            }

            obs = rdup_tv_term(dnf->terms->arr[0]);
            for (iy = 0; iy < obs->neg->sz; iy++) {
                obs->neg->arr[iy] = (int)map->arr[obs->neg->arr[iy]];
            }
            for (iy = 0; iy < obs->pos->sz; iy++) {
                obs->pos->arr[iy] = (int)map->arr[obs->pos->arr[iy]];
            }
            array_free(map);

            hash_add(result->observations,
                     name->name,
                     strlen(name->name),
                     (char *)&obs,
                     sizeof(tv_term),
                     NULL);
        } else if (dump->nodes->arr[ix]->constraints->tag == TAGtv_cnf) {
            tv_clause_list obs;

            cnf = to_tv_cnf(dump->nodes->arr[ix]->constraints);
            assert(cnf != tv_cnfNIL);
            assert(cnf->clauses != tv_clause_listNIL);

            map = map_variables(cnf->variables, model_variables);
            if (map == NULL) {
                return 3; /* Cannot map observations to model. */
            }

            obs = rdup_tv_clause_list(cnf->clauses);
            for (iy = 0; iy < obs->sz; iy++) {
                tv_clause clause = obs->arr[iy];
                for (iz = 0; iz < clause->neg->sz; iz++) {
                    clause->neg->arr[iz] = (int)map->arr[clause->neg->arr[iz]];
                }
                for (iz = 0; iz < clause->pos->sz; iz++) {
                    clause->pos->arr[iz] = (int)map->arr[clause->pos->arr[iz]];
                }
            }
            array_free(map);

            hash_add(result->observations,
                     (char *)name,
                     sizeof(lydia_symbol),
                     (char *)&obs,
                     sizeof(tv_clause_list),
                     NULL);
        } else {
/* Neither hDNF nor hCNF. */
            return 4;
        }
    }

    *output = result;

    return 0;
}

void *observation_get(observations context, const char *name)
{
    void *pdata;

    if (hash_find(context->observations,
                  name,
                  strlen(name),
                  &pdata)) {
        return NULL;
    }

    return *(void **)pdata;
}
