#include "err.h"
#include "node.h"

void node_zero(Node *node)
{
    ASSERT(node, ERR_NULL_ARG);
    memset(node, 0, sizeof(*node));
}

void node_free(Node *node)
{
    ASSERT(node, ERR_NULL_ARG);
    /* free all things */
    str_free(&node->title);
    str_free(&node->desc);
    vrnode_free(&node->outgoing);
    vrnode_free(&node->incoming);
    //vnode_free(&node->nodes);
    node_zero(node);
}

int node_fmt_desc(Str *out, Node *node)
{
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    if(str_length(&node->desc)) {
        TRY(str_fmt(out, "\n%.*s\n\n", STR_F(&node->desc)), ERR_STR_FMT);
    } else {
        TRY(str_fmt(out, F("\nno description.\n\n", IT)), ERR_STR_FMT);
    }
    return 0;
error:
    return -1;
}

int node_fmt(Str *out, Node *node, bool show_desc, const char *select, bool active)
{
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(select, ERR_NULL_ARG);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    if(!active) {
        TRY(str_fmt(out, "" NODE_FMT_LEN_SUB_INACTIVE " :: %s%s %.*s \n", sO+sI, select, icon_str(node->icon), STR_F(&node->title)), ERR_STR_FMT)
    } else { 
        TRY(str_fmt(out, "" NODE_FMT_LEN_SUB_ACTIVE " :: %s%s %.*s \n", sO+sI, select, icon_str(node->icon), STR_F(&node->title)), ERR_STR_FMT) 
    }
    if(show_desc) {
        TRY(node_fmt_desc(out, node), ERR_NODE_FMT_DESC);
    }
    return 0;
error:
    return -1;
}

int node_fmt_sub(Str *out, Node *node, bool show_desc, bool show_preview, size_t sub_sel)
{
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    size_t iE = 0;
    Node *sub_info = 0;
    for(size_t i = 0; i < sO; i++, iE++) {
        Node *sub = vrnode_get_at(&node->outgoing, i);
        TRY(node_fmt(out, sub, false, iE == sub_sel ? "--> " : "  > ", iE == sub_sel), ERR_NODE_FMT);
        if(iE == sub_sel) sub_info = sub;
    }
    /* incoming */
    for(size_t i = 0; i < sI; i++, iE++) {
        Node *sub = vrnode_get_at(&node->incoming, i);
        TRY(node_fmt(out, sub, false, iE == sub_sel ? "<-- " : "<   ", iE == sub_sel), ERR_NODE_FMT);
        if(iE == sub_sel) sub_info = sub;
    }
    if(show_desc && show_preview && sub_info) {
        TRY(node_fmt_desc(out, sub_info), ERR_NODE_FMT_DESC);
    }
    return 0;
error:
    return -1;
}

int node_create(Node *node, const char *title, const char *desc, Icon icon)
{
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(title, ERR_NULL_ARG);
    ASSERT(desc, ERR_NULL_ARG);
    node_zero(node);
    node->icon = icon;
    TRY(str_fmt(&node->title, "%s", title), ERR_STR_FMT);
    TRY(str_fmt(&node->desc, "%s", desc), ERR_STR_FMT);
    return 0;
error:
    return -1;
}

int node_follow(Node **node, size_t *sub_sel)
{
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(sub_sel, ERR_NULL_ARG);
    size_t sO = vrnode_length(&(*node)->outgoing);
    size_t sI = vrnode_length(&(*node)->incoming);
    Node *result = *node;
    if(*sub_sel < sO) {
        result = vrnode_get_at(&(*node)->outgoing, *sub_sel);
    } else if(*sub_sel - sO < sI) {
        result = vrnode_get_at(&(*node)->incoming, *sub_sel - sO);
    } else if(sO+sI) {
        THROW("sub_sel '%zu' too large", *sub_sel);
    }
    *node = result;
    *sub_sel = 0; //result->sub_index;
    return 0;
error:
    return -1;
}

void node_set_sub(Node *node, size_t *sub_sel, size_t to_set)
{
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(sub_sel, ERR_NULL_ARG);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    size_t max = sO + sI;
    if(SIZE_IS_NEG(to_set)) {
        *sub_sel = max ? max - 1 : 0;
    } else if(to_set >= max) {
        *sub_sel = 0;
    } else {
        *sub_sel = to_set;
    }
    //node->sub_index = *sub_sel;
}

#if 0
void node_mv_vertical(Node *node, int count)
{
    node_set_sub(node, &node->sub_index, node->sub_index + (size_t)count);
}
#endif



