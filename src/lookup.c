#include <ctype.h>

#include "node.h"
#include "str.h"
#include "lookup.h"

static inline size_t tnode_hash(Node *node)
{
    //size_t hash = str_hash_ci(&node->title);
    size_t hash = str_hash_esci(&node->title);
    //printf("Node '%.*s' hash %zx\n", STR_F(&node->title), hash);
    return hash;
}

int strcicmp(char const *a, size_t la, char const *b, size_t lb)
{
    if(la != lb) return -1;
    for (size_t i = 0; i < la; ++i) {
        int d = tolower((unsigned char)a[i]) - tolower((unsigned char)b[i]);
        if (d != 0 || !*a)
            return d;
    }
    return 0;
}


static inline int tnode_cmp(Node *a, Node *b)
{
    return str_cmp_esci(&a->title, &b->title);
    //return strcicmp(str_iter_begin(&a->title_link), str_length(&a->title_link), str_iter_begin(&b->title_link), str_length(&b->title_link));
}

static inline size_t tnodeicon_hash(Node *node)
{
    size_t hash = 0;
#if 0
    if(node->icon < 0) hash = 99194853094755497ULL * (size_t)node->icon; /* 83rd fibonacci number because why not */
#endif
    return hash;
}

static inline int tnodeicon_cmp(Node *a, Node *b)
{
#if 0
    if(a->icon >= 0 && b->icon >= 0) return 0;
    else return !(a->icon == b->icon);
#endif
}

LUTD_IMPLEMENT(TNode, tnode, Node, BY_REF, tnode_hash, tnode_cmp, node_free);
LUTD_IMPLEMENT(TrNode, trnode, Node, BY_REF, tnode_hash, tnode_cmp, 0);
LUTD_IMPLEMENT(TNodeIcon, tnodeicon, Node, BY_REF, tnodeicon_hash, tnodeicon_cmp, node_free);

void tnode_sort_sub(TNode *tnode, VrNodeSortFunc sort)
{
    ASSERT(tnode, ERR_NULL_ARG);
    for(size_t i = 0; i < 1ULL << (tnode->width - 1); ++i) {
        for(size_t j = 0; j < tnode->buckets[i].cap; j++) { \
            Node *node = tnode->buckets[i].items[j];
            //printf("SORTING [%.*s]\n", STR_F(&node->title));
            sort(&node->incoming);
            sort(&node->outgoing);
        }
    }
}

