#include <stdlib.h>
#include <limits.h>

#include "qsort.h"

#define QSORT_STACK_SIZE (sizeof(size_t) * CHAR_BIT)

static void _lydia_qsort_swap(void *a, void *b, size_t siz)
{
    register char *tmp_a_char;
    register char *tmp_b_char;
    register int *tmp_a_int;
    register int *tmp_b_int;
    register size_t i;
    int t_i;
    char t_c;

    tmp_a_int = (int *)a;
    tmp_b_int = (int *)b;

    for (i = sizeof(int); i <= siz; i += sizeof(int)) {
        t_i = *tmp_a_int;
        *tmp_a_int++ = *tmp_b_int;
        *tmp_b_int++ = t_i;
    }

    tmp_a_char = (char *)tmp_a_int;
    tmp_b_char = (char *)tmp_b_int;

    for (i = i - sizeof(int) + 1; i <= siz; ++i) {
        t_c = *tmp_a_char;
        *tmp_a_char++ = *tmp_b_char;
        *tmp_b_char++ = t_c;
    }
}

void lydia_qsort(void *base, size_t nmemb, size_t siz, lydia_compare_func_t compare)
{
    void *begin_stack[QSORT_STACK_SIZE];
    void *end_stack[QSORT_STACK_SIZE];
    register char *begin;
    register char *end;
    register char *seg1;
    register char *seg2;
    register char *seg2p;
    register int loop;
    size_t offset;

    if (nmemb < 2) {
        return;
    }

    begin_stack[0] = (char *)base;
    end_stack[0] = (char *)base + ((nmemb - 1) * siz);

    for (loop = 0; loop >= 0; --loop) {
        begin = begin_stack[loop];
        end = end_stack[loop];

        while (begin < end) {
            offset = (end - begin) >> 1;
            _lydia_qsort_swap(begin, begin + (offset - (offset % siz)), siz);

            seg1 = begin + siz;
            seg2 = end;

            while (1) {
                for (; seg1 < seg2 && compare(begin, seg1) > 0; seg1 += siz);
                for (; seg2 >= seg1 && compare(seg2, begin) > 0; seg2 -= siz);
                                
                if (seg1 >= seg2) {
                    break;
                }
                                
                _lydia_qsort_swap(seg1, seg2, siz);

                seg1 += siz;
                seg2 -= siz;
            }

            _lydia_qsort_swap(begin, seg2, siz);

            seg2p = seg2;
                        
            if ((seg2p - begin) <= (end - seg2p)) {
                if ((seg2p + siz) < end) {
                    begin_stack[loop] = seg2p + siz;
                    end_stack[loop++] = end;
                }
                end = seg2p - siz;
            } else {
                if ((seg2p - siz) > begin) {
                    begin_stack[loop] = begin;
                    end_stack[loop++] = seg2p - siz;
                }
                begin = seg2p + siz;
            }
        }
    }
}
