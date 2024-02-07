#ifndef NODE_H

#include <stdbool.h>

#include "err.h"
#include "str.h"
#include "vector.h"
#include "icon.h"

typedef struct Node {
    Icon icon;
    Str title;
    Str desc;
    VrNode outgoing;
    VrNode incoming;
    size_t sub_index;
    //VNode nodes;
    //Node *prev;
    //Node *rejoin;
} Node;

#define NODE_FMT_LEN_SUB    F("%zu", FG_BK_B)

#define NODE_LEAF  0

void node_zero(Node *node);
void node_free(Node *node);

#define ERR_NODE_PRINT "failed printing node"
ErrDecl node_print(Node *node, bool show_desc);

#define ERR_NODE_CREATE "failed creating node"
ErrDecl node_create(Node *node, const char *title, const char *desc, Icon icon);

#define ERR_NODE_FOLLOW "failed following node"
ErrDecl node_follow(Node **node, size_t *sub_sel);

void node_set_sub(Node *node, size_t *sub_sel, size_t to_set);

void node_mv_vertical(Node *node, int count);

#define NODE_H
#endif

