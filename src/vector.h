#ifndef VECTOR_H

#include "vec.h"

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#if 0
VEC_INCLUDE(Vu8,  vu8,  uint8_t,  BY_VAL);
VEC_INCLUDE(Vu16, vu16, uint16_t, BY_VAL);
VEC_INCLUDE(Vu32, vu32, uint32_t, BY_VAL);
VEC_INCLUDE(Vu64, vu64, uint64_t, BY_VAL);
#endif

/* other types of vectors */
struct Node;
VEC_INCLUDE(VNode, vnode, struct Node, BY_REF);
VEC_INCLUDE(VrNode, vrnode, struct Node *, BY_VAL);
void vrnode_sort(VrNode *vec);

struct View;
VEC_INCLUDE(VView, vview, struct View, BY_VAL);

struct Str;
VEC_INCLUDE(VsStr, vsstr, struct Str, BY_REF);

struct BtwLext;
VEC_INCLUDE(VBtwLex, vbtwlex, struct BtwLex, BY_REF);

#define VECTOR_H
#endif

