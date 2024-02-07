#include "node.h"
#include "lookup.h"

static inline size_t tnode_hash(Node *node)
{
    size_t hash = str_hash(&node->title);
    return hash;
}

static inline int tnode_cmp(Node *a, Node *b)
{
    return str_cmp(&a->title, &b->title);
}

LUTD_IMPLEMENT(TNode, tnode, Node, BY_REF, tnode_hash, tnode_cmp, node_free);


#if 0
static inline size_t tlink_hash(Link *link)
{
    size_t hash = (size_t)link;//str_hash(&node->title);
    return hash;
}

static inline int tlink_cmp(Link *a, Link *b)
{
    return 0;//str_cmp(&a->title, &b->title);
}

LUTD_IMPLEMENT(TLink, tlink, Link, BY_REF, tlink_hash, tlink_cmp, link_free);
#endif

