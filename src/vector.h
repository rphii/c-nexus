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

VEC_INCLUDE(Vu8,  vu8,  uint8_t,  BY_VAL);
VEC_INCLUDE(Vu16, vu16, uint16_t, BY_VAL);
VEC_INCLUDE(Vu32, vu32, uint32_t, BY_VAL);
VEC_INCLUDE(Vu64, vu64, uint64_t, BY_VAL);

/* other types of vectors */
struct Node;
VEC_INCLUDE(VNode, vnode, struct Node, BY_REF);
VEC_INCLUDE(VrNode, vrnode, struct Node *, BY_VAL);

#define VECTOR_H
#endif

