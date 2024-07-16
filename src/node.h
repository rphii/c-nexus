#ifndef NODE_H

#include <stdbool.h>

#include "err.h"
#include "str.h"
#include "vector.h"

typedef enum {
    NODE_DEFAULT,
    NODE_TIME,

} NodeList;

typedef enum {
    NODE_TYPE_NONE,
    NODE_TYPE_TEXT = NODE_TYPE_NONE,
    NODE_TYPE_ICON,
} NodeType;

typedef struct Node {
    //Icon icon;
    //VIcon icons;
    Str title;
    Str desc;
    Str cmd;
    // TODO: add/create a new/better link type! those that have a reference to the text in the desc, if pesent! adjust view accordingly
    VrStr textlinks;
    VrNode outgoing;
    VrNode incoming;
    //VrNode tags;
    NodeList id;
    // TODO it's stupid to have VrNode tags.. and also an additional TrNode icons in nexus.h ... -> add a type, saying wheter tag, etc, whatever!
    NodeType type;
} Node;

#if (NODE_SHOW_COUNT_IN_OUT)
#define NODE_FMT_LEN_SUB_INACTIVE   F("%*zu<->%-*zu", FG_BK_B)
#define NODE_FMT_LEN_SUB_ACTIVE     F("%*zu<->%-*zu", FG_CY_B)
#else
#define NODE_FMT_LEN_SUB_INACTIVE   F("%*zu", FG_BK_B)
#define NODE_FMT_LEN_SUB_ACTIVE     F("%*zu", FG_CY_B)
#endif

#define NODE_FMT_ARR_ACTIVE         F("-> ", FG_CY_B)
#define NODE_FMT_ARL_ACTIVE         F("<- ", FG_CY_B)
#define NODE_FMT_ARR_INACTIVE       F(" > ", FG_BK_B)
#define NODE_FMT_ARL_INACTIVE       F("<  ", FG_BK_B)

#define NODE_LEAF  0

void node_zero(Node *node);
void node_free(Node *node);
void node_clear(Node *node);

int node_cmp(Node *a, Node *b);

#define ERR_NODE_COPY "failed copying node"
ErrDecl node_copy(Node *dst, Node *src);

#define ERR_NODE_CREATE "failed creating node"
#define ERR_node_create(node, title, cmd, desc) "failed creating node"
ErrDecl node_create(Node *node, Str *title, Str *cmd, Str *desc);

#define ERR_NODE_FOLLOW "failed following node"
ErrDecl node_follow(Node **node, size_t *sub_sel);

#define ERR_NODE_FMT_DESC "failed formatting node description"
ErrDecl node_fmt_desc(Str *out, Node *node);

#define ERR_NODE_FMT_SUB "failed formatting sub nodes"
ErrDecl node_fmt_sub(Str *out, Node *node, bool show_desc, bool show_preview, size_t max_preview, size_t sub_sel);

Node *node_get_sub_sel(Node *node, size_t sub_sel);

#define ERR_NODE_FMT "failed formatting node"
#define ERR_node_fmt(out, node, show_desc, select, padl, padr, active) "failed formatting '%.*s'", STR_F(&node->title)
ErrDecl node_fmt(Str *out, Node *node, bool show_desc, const char *select, int padl, int padr, bool active);

void node_set_sub(Node *node, size_t *sub_sel, size_t to_set);

void node_mv_vertical(Node *node, int count);

#define NODE_H
#endif

