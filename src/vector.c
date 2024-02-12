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

VEC_IMPLEMENT(VView, vview, View, BY_VAL, view_free);

