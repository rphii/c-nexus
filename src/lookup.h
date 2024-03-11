#ifndef LOOKUP_H

#include "lutd.h"

/* other types of lookup tables */
struct Node;
LUTD_INCLUDE(TNode, tnode, struct Node, BY_REF);
LUTD_INCLUDE(TNodeIcon, tnodeicon, struct Node, BY_REF);

void tnode_sort_sub(TNode *tnode);

#if 0
LUTD_INCLUDE(TLink, tlink, Link, BY_REF);
#endif

#define LOOKUP_H
#endif

