#include "node.h"
#include "view.h"

#include "vector.h"

#if 0
VEC_IMPLEMENT(Vu8,  vu8,  uint8_t,  BY_VAL, 0);
VEC_IMPLEMENT(Vu16, vu16, uint16_t, BY_VAL, 0);
VEC_IMPLEMENT(Vu32, vu32, uint32_t, BY_VAL, 0);
VEC_IMPLEMENT(Vu64, vu64, uint64_t, BY_VAL, 0);
#endif

/* other types of vectors */
/* nodes */
VEC_IMPLEMENT(VNode, vnode, Node, BY_REF, node_free);
VEC_IMPLEMENT(VrNode, vrnode, Node *, BY_VAL, 0);

#if VECTOR_SORT_DATES_FIRST
#define FAKE_TIME(x)    (INT64_MIN-x)
#else
#define FAKE_TIME(x)    (x)
#endif

void vrnode_sort(VrNode *vec)
{
    /* shell sort, https://rosettacode.org/wiki/Sorting_algorithms/Shell_sort#C */
    size_t h, i, j, n = vrnode_length(vec);
    Node *temp;
    for (h = n; h /= 2;) {
        for (i = h; i < n; i++) {
            //t = a[i];
            temp = vrnode_get_at(vec, i);
            //for (j = i; j >= h && t < a[j - h]; j -= h) {
            for (j = i; j >= h && FAKE_TIME(temp->icon) < FAKE_TIME(vrnode_get_at(vec, j-h)->icon); j -= h) {
                vrnode_set_at(vec, j, vrnode_get_at(vec, j-h));
                //a[j] = a[j - h];
            }
            //a[j] = t;
            vrnode_set_at(vec, j, temp);
        }
        }
}


VEC_IMPLEMENT(VView, vview, View, BY_VAL, view_free);


