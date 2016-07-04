#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>

#include "priority_queue.h"
#include "variable.h"
#include "array.h"
#include "ds.h"

extern void cdas_best_kernels_print(FILE *, tv_variables_cache, trie);
extern void cdas_assumable_list_print(FILE *, tv_variables_cache, array);
extern void cdas_node_print(FILE *, tv_variables_cache, cdas_node);
extern void cdas_node_list_print(FILE *, tv_variables_cache, array);
extern void cdas_priority_queue_print(FILE *, tv_variables_cache, priority_queue);
extern void cdas_constituent_kernels_print(FILE *, tv_variables_cache, cdas_context);
extern void cdas_label_print(FILE *, array);

#endif
