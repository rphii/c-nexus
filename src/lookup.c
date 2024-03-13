#include <ctype.h>

#include "node.h"
#include "lookup.h"

static inline size_t tnode_hash(Node *node)
{
    size_t hash = str_hash_ci(&node->title);
    return hash;
}

int strcicmp(char const *a, size_t la, char const *b, size_t lb)
{
    if(la != lb) return -1;
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}


static inline int tnode_cmp(Node *a, Node *b)
{
    return strcicmp(str_iter_begin(&a->title), str_length(&a->title), str_iter_begin(&b->title), str_length(&b->title));
}

static inline size_t tnodeicon_hash(Node *node)
{
    size_t hash = 0;
    if(node->icon < 0) hash = 99194853094755497ULL * (size_t)node->icon; /* 83rd fibonacci number because why not */
    return hash;
}

static inline int tnodeicon_cmp(Node *a, Node *b)
{
    if(a->icon >= 0 && b->icon >= 0) return 0;
    else return !(a->icon == b->icon);
}

LUTD_IMPLEMENT(TNode, tnode, Node, BY_REF, tnode_hash, tnode_cmp, node_free);
LUTD_IMPLEMENT(TNodeIcon, tnodeicon, Node, BY_REF, tnodeicon_hash, tnodeicon_cmp, node_free);

void tnode_sort_sub(TNode *tnode)
{
    ASSERT(tnode, ERR_NULL_ARG);
    for(size_t i = 0; i < 1ULL << (tnode->width - 1); ++i) {
        for(size_t j = 0; j < tnode->buckets[i].cap; j++) { \
            Node *node = tnode->buckets[i].items[j];
            vrnode_sort(&node->incoming);
            vrnode_sort(&node->outgoing);
        }
    }
}

