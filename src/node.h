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
    Str cmd;
    VrNode outgoing;
    VrNode incoming;
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

#define ERR_NODE_COPY "failed copying node"
ErrDecl node_copy(Node *restrict dst, Node *restrict src);

#define ERR_NODE_CREATE "failed creating node"
ErrDecl node_create(Node *node, const char *title, const char *cmd, const char *desc, Icon icon);

#define ERR_NODE_FOLLOW "failed following node"
ErrDecl node_follow(Node **node, size_t *sub_sel);

#define ERR_NODE_FMT_DESC "failed formatting node description"
ErrDecl node_fmt_desc(Str *out, Node *node);

#define ERR_NODE_FMT_SUB "failed formatting sub nodes"
ErrDecl node_fmt_sub(Str *out, Node *node, bool show_desc, bool show_preview, size_t max_preview, size_t sub_sel);

Node *node_get_sub_sel(Node *node, size_t sub_sel);

#define ERR_NODE_FMT "failed formatting node"
int node_fmt(Str *out, Node *node, bool show_desc, const char *select, int padl, int padr, bool active);

void node_set_sub(Node *node, size_t *sub_sel, size_t to_set);

void node_mv_vertical(Node *node, int count);

#define NODE_H
#endif

