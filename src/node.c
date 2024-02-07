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

int node_print(Node *node, bool show_desc, size_t sub_sel)
{
    ASSERT(node, ERR_NULL_ARG);
    int err = 0;
    Str p = {0};
    TRY(str_fmt(&p, "%s : %.*s\n", icon_str(node->icon), STR_F(&node->title)), ERR_STR_FMT);
    if(show_desc) {
        if(str_length(&node->desc)) {
            TRY(str_fmt(&p, "\n%.*s\n\n", STR_F(&node->desc)), ERR_STR_FMT);
        } else {
            TRY(str_fmt(&p, F("\nno description.\n\n", IT)), ERR_STR_FMT);
        }
    }
    /* outgoing */
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    size_t iE = 0;
    Node *sub_info = 0;
    for(size_t i = 0; i < sO; i++, iE++) {
        Node *sub = vrnode_get_at(&node->outgoing, i);
        TRY(str_fmt(&p, "%s : %s %.*s\n", icon_str(sub->icon), iE == sub_sel ? "-->" : "  >", STR_F(&sub->title)), ERR_STR_FMT);
        if(iE == sub_sel) sub_info = sub;
    }
    /* incoming */
    for(size_t i = 0; i < sI; i++, iE++) {
        Node *sub = vrnode_get_at(&node->incoming, i);
        TRY(str_fmt(&p, "%s : %s %.*s\n", icon_str(sub->icon), iE == sub_sel ? "<--" : "<  ", STR_F(&sub->title)), ERR_STR_FMT);
        if(iE == sub_sel) sub_info = sub;
    }
    if(show_desc && sub_info) {
        if(str_length(&sub_info->desc)) {
            TRY(str_fmt(&p, "\n%.*s\n\n", STR_F(&sub_info->desc)), ERR_STR_FMT);
        } else {
            TRY(str_fmt(&p, F("\nno description.\n\n", IT)), ERR_STR_FMT);
        }
    }
    printf("%.*s", STR_F(&p));
clean:
    str_free(&p);
    return err;
error:
    ERR_CLEAN;
#if 0
    for(size_t i = 0; i < vnode_length(&node->nodes); i++) {
        Node *sub = vnode_get_at(&node->nodes, i);
        printf("%s %.*s\n", i == sub_sel ? ">" : " ", (int)str_length(&sub->title), sub->title.s);
    }
#endif
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
    *sub_sel = result->sub_index;
    return 0;
error:
    return -1;
}

void node_set(Node *node, size_t *sub_sel)
{
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(sub_sel, ERR_NULL_ARG);
    *sub_sel = node->sub_index;
}

void node_set_sub(Node *node, size_t *sub_sel, size_t to_set)
{
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(sub_sel, ERR_NULL_ARG);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    size_t max = sO + sI;
    if((ssize_t)to_set < 0) {
        *sub_sel = max ? max - 1 : 0;
    } else if(to_set >= max) {
        *sub_sel = 0;
    } else {
        *sub_sel = to_set;
    }
    node->sub_index = *sub_sel;
}



