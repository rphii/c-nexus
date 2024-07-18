#include "node.h"
#include "view.h"
#include "str.h"
#include "btw.h"

#include "vector.h"

VEC_IMPLEMENT(VSize, vsize, size_t, BY_VAL, 0);

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

#if (VECTOR_SORT_DATES_FIRST)
#define FAKE_TIME(x)    (INT64_MIN-x)
#else
#define FAKE_TIME(x)    (x)
#endif

VrNodeSortFunc vrnode_sort_func(int id) {
    switch((NodeSortList)id) {
        case NODE_SORT_BY_TITLE: return vrnode_sort_by_title;
        case NODE_SORT_BY_INDEX: return vrnode_sort_by_index;
        default: ABORT(ERR_UNREACHABLE);
    }
}

void vrnode_sort_by_index(VrNode *vec) {
    ASSERT_ARG(vec);
    //ASSERT_ARG(counts);
    size_t h, i, j, n = vrnode_length(vec);
    Node *temp;
    for (h = n; h /= 2;) {
        for (i = h; i < n; i++) {
            //t = a[i];
            temp = vrnode_get_at(vec, i);
            //temp_count = counts[i];
            //for (j = i; j >= h && t < a[j - h]; j -= h) {
            for (j = i; j >= h && (temp->index - 1) < (vrnode_get_at(vec, j-h)->index - 1); j -= h) {
                vrnode_set_at(vec, j, vrnode_get_at(vec, j-h));
                //counts[j] = counts[j-h];
                //a[j] = a[j - h];
            }
            //a[j] = t;
            vrnode_set_at(vec, j, temp);
            //counts[j] = temp_count;
        }
    }
}

void vrnode_sort_by_title(VrNode *vec)
{
#if 1
    size_t h, i, j, n = vrnode_length(vec);
    Node *temp;
    for (h = n; h /= 2;) {
        for (i = h; i < n; i++) {
            //t = a[i];
            temp = vrnode_get_at(vec, i);
            //for (j = i; j >= h && t < a[j - h]; j -= h) {
            for (j = i; j >= h && str_cmp(&temp->title, &vrnode_get_at(vec, j-h)->title) < 0; j -= h) {
                vrnode_set_at(vec, j, vrnode_get_at(vec, j-h));
                //a[j] = a[j - h];
            }
            //a[j] = t;
            vrnode_set_at(vec, j, temp);
        }
    }
#endif
    /* shell sort, https://rosettacode.org/wiki/Sorting_algorithms/Shell_sort#C */
#if 0 // TODO !!!
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
#endif
}


VEC_IMPLEMENT(VView, vview, View, BY_VAL, view_free);

VEC_IMPLEMENT(VsStr, vsstr, Str, BY_REF, 0);
VEC_IMPLEMENT(VvStr, vvstr, Str, BY_VAL, 0);

VEC_IMPLEMENT(VrStr, vrstr, Str *, BY_VAL, 0);
VEC_IMPLEMENT(VStr, vstr, Str, BY_REF, str_free);
VEC_IMPLEMENT(VBtwLex, vbtwlex, BtwLex, BY_REF, btwlex_free);
VEC_IMPLEMENT(VBtwLink, vbtwlink, BtwLink, BY_REF, btwlink_free);

//VEC_IMPLEMENT(VIcon, vicon, IconBundle, BY_VAL, icon_free);

