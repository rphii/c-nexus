#include "err.h"
#include "node.h"
#include "cmd.h"

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
    str_free(&node->cmd);
    vrnode_free(&node->outgoing);
    vrnode_free(&node->incoming);
    //vnode_free(&node->nodes);
    node_zero(node);
}

int node_fmt_desc(Str *out, Node *node)
{
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    size_t len_cmd = str_length(&node->cmd);
    size_t len_desc = str_length(&node->desc);
    if(len_cmd || len_desc) {
        if(len_cmd) {
            TRY(str_fmt(out, "\n" CMD_FMT("$ %.*s") "\n", STR_F(&node->cmd)), ERR_STR_FMT);
        }
        if(len_desc) {
            TRY(str_fmt(out, "\n%.*s\n", STR_F(&node->desc)), ERR_STR_FMT);
        }
        TRY(str_fmt(out, "\n"), ERR_STR_FMT);
    } else {
        TRY(str_fmt(out, F("\nno description.\n\n", IT)), ERR_STR_FMT);
    }
    return 0;
error:
    return -1;
}

Node *node_get_sub_sel(Node *node, size_t sub_sel)
{
    ASSERT(node, ERR_NULL_ARG);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    if(sub_sel < sO) {
        return vrnode_get_at(&node->outgoing, sub_sel);
    } else if(sub_sel < sO+sI) {
        return vrnode_get_at(&node->incoming, sub_sel);
    } else {
        ABORT("index (%zu) out of range (%zu+%zu)", sub_sel, sO, sI);
    }
    return 0;
}

int node_fmt(Str *out, Node *node, bool show_desc, const char *select, int pad, bool active)
{
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(select, ERR_NULL_ARG);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    if(!active) {
        TRY(str_fmt(out, "" NODE_FMT_LEN_SUB_INACTIVE " :: %s%s %.*s \n", pad, sO+sI, select, icon_str(node->icon), STR_F(&node->title)), ERR_STR_FMT)
    } else {
        TRY(str_fmt(out, "" NODE_FMT_LEN_SUB_ACTIVE " :: %s%s %.*s \n", pad, sO+sI, select, icon_str(node->icon), STR_F(&node->title)), ERR_STR_FMT)
    }
    if(show_desc) {
        TRY(node_fmt_desc(out, node), ERR_NODE_FMT_DESC);
    }
    return 0;
error:
    return -1;
}

int node_fmt_sub(Str *out, Node *node, bool show_desc, bool show_preview, size_t max_preview, size_t sub_sel)
{
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(max_preview, "max_preview cannot be 0");
    int err = 0;
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    size_t n = sO+sI;
    /* determine padding ... TODO improve, in some way shape or form so we can also pad the icon ... */
    int padding = 0;
    Str padtest = {0};
    for(size_t i = 0; i < sO; i++) {
        str_clear(&padtest);
        Node *sub = vrnode_get_at(&node->outgoing, i);
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->outgoing) + vrnode_length(&sub->incoming)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)padding) padding = (int)str_length(&padtest);
    }
    for(size_t i = 0; i < sI; i++) {
        str_clear(&padtest);
        Node *sub = vrnode_get_at(&node->incoming, i);
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->outgoing) + vrnode_length(&sub->incoming)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)padding) padding = (int)str_length(&padtest);
    }
    /* actually format */
    Node *sub_info = 0;
    size_t sub_sel2 = n > max_preview ? sub_sel + max_preview / 2 : 0;
    size_t ioff = sub_sel2 >= max_preview ? 1 + sub_sel2 - max_preview : 0;
    if(n > max_preview && ioff + max_preview > n) ioff = n - max_preview;
    //printf("ioff=%zu, sub_sel=%zu\n", ioff, sub_sel);
    for(size_t i = 0; i < n && i < max_preview; i++) {
        size_t ireal = i+ioff;
        Node *sub = 0;
        if(ireal < sO) {
            sub = vrnode_get_at(&node->outgoing, ireal);
            const char *arrow = i+ioff == sub_sel ? "--> " : "  > ";
            if(SIZE_IS_NEG(sub_sel)) arrow = "";
            TRY(node_fmt(out, sub, false, arrow, padding, i+ioff == sub_sel), ERR_NODE_FMT);
        } else if(ireal < n) {
            const char *arrow = i+ioff == sub_sel ? "<-- " : "<   ";
            if(SIZE_IS_NEG(sub_sel)) arrow = "";
            sub = vrnode_get_at(&node->incoming, ireal-sO);
            TRY(node_fmt(out, sub, false, arrow, padding, i+ioff == sub_sel), ERR_NODE_FMT);
        }
        if(ireal == sub_sel) sub_info = sub;
    }
    if(n > max_preview) {
        TRY(str_fmt(out, F("... (", IT) F("%4zu", IT FG_YL_B) F(" more)\n", IT), sO+sI - max_preview), ERR_STR_FMT);
    }
    if(show_preview && sub_info) {
        TRY(node_fmt_desc(out, sub_info), ERR_NODE_FMT_DESC);
    }
clean:
    str_free(&padtest);
    return err;
error:
    ERR_CLEAN;
}

int node_copy(Node *restrict dst, Node *restrict src)
{
    ASSERT(dst, ERR_NULL_ARG);
    ASSERT(src, ERR_NULL_ARG);
    TRY(node_create(dst, str_iter_begin(&src->title), str_iter_begin(&src->cmd), str_iter_begin(&src->desc), src->icon), ERR_NODE_CREATE);
    return 0;
error:
    return -1;
}

int node_create(Node *node, const char *title, const char *cmd, const char *desc, Icon icon)
{
    ASSERT(node, ERR_NULL_ARG);
    ASSERT(title, ERR_NULL_ARG);
    node_zero(node);
    node->icon = icon;
    TRY(str_fmt(&node->title, "%s", title), ERR_STR_FMT);
    if(desc) TRY(str_fmt(&node->desc, "%s", desc), ERR_STR_FMT);
    if(cmd) TRY(str_fmt(&node->cmd, "%s", cmd), ERR_STR_FMT);
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




