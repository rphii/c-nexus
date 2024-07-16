#include "err.h"
#include "node.h"
#include "cmd.h"
#include "str.h"
#include "vector.h"

int node_cmp(Node *a, Node *b) {
    ASSERT_ARG(a);
    ASSERT_ARG(b);
    return str_cmp_esci(&a->title, &b->title);
}

void node_zero(Node *node)
{
    ASSERT_ARG(node);
    memset(node, 0, sizeof(*node));
}

void node_free(Node *node)
{
    ASSERT_ARG(node);
    /* free all things */
    //for(size_t i = 0; i < ICON_BUNDLE_MAX; ++i) {
    //    icon_free(&node->icons.items[i]);
    //}
    str_free(&node->title);
    str_free(&node->desc);
    str_free(&node->cmd);
    vrnode_free(&node->outgoing);
    vrnode_free(&node->incoming);
    node_zero(node);
}

int node_fmt_desc(Str *out, Node *node)
{
    ASSERT_ARG(out);
    ASSERT_ARG(node);
    size_t len_cmd = str_length(&node->cmd);
    size_t len_desc = str_length(&node->desc);
    if(len_cmd || len_desc) {
        if(len_cmd) {
            TRY(str_fmt(out, "\n" CMD_FMT("$ %.*s") "\n", STR_F(&node->cmd)), ERR_STR_FMT);
        }
        if(len_desc) {
            TRY(str_fmt(out, "\n%.*s\n", STR_F(&node->desc)), ERR_STR_FMT);
        }
    } else {
        TRY(str_fmt(out, F("\nno description.\n", IT)), ERR_STR_FMT);
    }
    return 0;
error:
    return -1;
}

Node *node_get_sub_sel(Node *node, size_t sub_sel)
{
    ASSERT_ARG(node);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    if(sub_sel >= sO+sI) sub_sel = sO+sI - 1;
    if(sub_sel < sO) {
        return vrnode_get_at(&node->outgoing, sub_sel);
    } else if(sub_sel < sO+sI) {
        return vrnode_get_at(&node->incoming, sub_sel);
    } else {
        //ABORT("index (%zu) out of range (%zu+%zu)", sub_sel, sO, sI);
    }
    return 0;
}

int node_fmt(Str *out, Node *node, bool show_desc, const char *select, int padl, int padr, bool active)
{
    ASSERT_ARG(out);
    ASSERT_ARG(node);
    ASSERT_ARG(select);
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    //icon_fmt(iconstr, node->icon);
#if (NODE_SHOW_COUNT_IN_OUT)
    if(!active) {
        TRYC(str_fmt(out, "" NODE_FMT_LEN_SUB_INACTIVE " %s", padl, sI, padr, sO, select)); //, iconstr, STR_F(&node->title));
    } else {
        TRYC(str_fmt(out, "" NODE_FMT_LEN_SUB_ACTIVE " %s", padl, sI, padr, sO, select)); //, iconstr, STR_F(&node->title));
    }
#else
    if(!active) {
        TRYC(str_fmt(out, "" NODE_FMT_LEN_SUB_INACTIVE " %s", padl > padr ? padl : padr, sI+sO, select)); //, iconstr, STR_F(&node->title));
    } else {
        TRYC(str_fmt(out, "" NODE_FMT_LEN_SUB_ACTIVE " %s", padl > padr ? padl : padr, sI+sO, select)); //, iconstr, STR_F(&node->title));
    }
#endif
    /* icons */
    bool tagged = false;
    for(size_t i = 0; i < vrnode_length(&node->outgoing); ++i) {
        Node *tag = vrnode_get_at(&node->outgoing, i);
        if(tag->type != NODE_TYPE_ICON) continue;
        TRYC(str_fmt(out, "%s%.*s", tagged ? " " : "", STR_F(&tag->title)));
        tagged = true;
    }
    for(size_t i = 0; i < vrnode_length(&node->incoming); ++i) {
        Node *tag = vrnode_get_at(&node->incoming, i);
        if(tag->type != NODE_TYPE_ICON) continue;
        TRYC(str_fmt(out, "%s%.*s", tagged ? " " : "", STR_F(&tag->title)));
        tagged = true;
    }
    if(!tagged) {
        TRYC(str_fmt(out, "-"));
    }
#if 0
    for(size_t i = 0; i < vrnode_length(&node->tags); ++i) {
        Node *tag = vrnode_get_at(&node->tags, i);
        TRYC(str_fmt(out, "%s%.*s", i ? " " : "", STR_F(&tag->title)));
    }
    if(!vrnode_length(&node->tags)) {
        TRYC(str_fmt(out, "-"));
    }
#endif
    //for(size_t i = 0; i < vicon_length(&node->icons); ++i) {
        //IconBundle *icon = vicon_get_at(&node->icons, i);
        //IconStr iconstr = {0};
        //icon_fmt(iconstr, icon->time);
        //TRYC(icons_fmt, out, &node->icons);
        TRYC(str_fmt(out, " : %.*s\n", STR_F(&node->title)));
    //}
    if(show_desc) {
        TRY(node_fmt_desc(out, node), ERR_NODE_FMT_DESC);
    }
    return 0;
error:
    return -1;
}

int node_fmt_sub(Str *out, Node *node, bool show_desc, bool show_preview, size_t max_preview, size_t sub_sel)
{
    ASSERT_ARG(out);
    ASSERT_ARG(node);
    ASSERT(max_preview, "max_preview cannot be 0");
    int err = 0;
    size_t sO = vrnode_length(&node->outgoing);
    size_t sI = vrnode_length(&node->incoming);
    size_t n = sO+sI;
    /* determine padding ... TODO improve, in some way shape or form so we can also pad the icon ... */
    int paddingr = 0;
    int paddingl = 0;
    Str padtest = {0};
    for(size_t i = 0; i < sO; i++) {
        Node *sub = vrnode_get_at(&node->outgoing, i);
        str_clear(&padtest);
#if (NODE_SHOW_COUNT_IN_OUT)
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->outgoing)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)paddingr) paddingr = (int)str_length(&padtest);
        str_clear(&padtest);
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->incoming)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)paddingl) paddingl = (int)str_length(&padtest);
#else
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->outgoing)+vrnode_length(&sub->incoming)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)paddingl) paddingl = (int)str_length(&padtest);
#endif
    }
    for(size_t i = 0; i < sI; i++) {
        Node *sub = vrnode_get_at(&node->incoming, i);
        str_clear(&padtest);
#if (NODE_SHOW_COUNT_IN_OUT)
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->outgoing)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)paddingr) paddingr = (int)str_length(&padtest);
        str_clear(&padtest);
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->incoming)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)paddingl) paddingl = (int)str_length(&padtest);
#else
        TRY(str_fmt(&padtest, "%zu", vrnode_length(&sub->outgoing)+vrnode_length(&sub->incoming)), ERR_STR_FMT);
        if(str_length(&padtest) > (size_t)paddingl) paddingl = (int)str_length(&padtest);
#endif
    }
    //printf("l %zu, r %zu\n", paddingl, paddingr);
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
            const char *arrow = i+ioff == sub_sel ? NODE_FMT_ARR_ACTIVE : NODE_FMT_ARR_INACTIVE;
            if(SIZE_IS_NEG(sub_sel)) arrow = "";
            TRY(node_fmt(out, sub, false, arrow, paddingl, paddingr, i+ioff == sub_sel), ERR_NODE_FMT);
        } else if(ireal < n) {
            const char *arrow = i+ioff == sub_sel ? NODE_FMT_ARL_ACTIVE : NODE_FMT_ARL_INACTIVE;
            if(SIZE_IS_NEG(sub_sel)) arrow = "";
            sub = vrnode_get_at(&node->incoming, ireal-sO);
            TRY(node_fmt(out, sub, false, arrow, paddingl, paddingr, i+ioff == sub_sel), ERR_NODE_FMT);
        }
        if(ireal == sub_sel) sub_info = sub;
    }
    if(n > max_preview) {
        TRY(str_fmt(out, F("\n... (", IT) F("%4zu", IT FG_YL_B) F(" more)\n", IT), sO+sI - max_preview), ERR_STR_FMT);
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
    ASSERT_ARG(dst);
    ASSERT_ARG(src);
    TRY(node_create(dst, &src->title, &src->cmd, &src->desc), ERR_NODE_CREATE);
    return 0;
error:
    return -1;
}

int node_create(Node *node, Str *title, Str *cmd, Str *desc)
{
    ASSERT_ARG(node);
    ASSERT_ARG(title);
    //INFO("creating T:%.*s,C:%s,D:%s\n", STR_F(title), cmd ? cmd->s : "", desc ? desc->s : "");
    if(!str_length(title)) THROW("title can't be empty");
    node_zero(node);
    //TRY(vicon_reserve(&node->icons, 1), ERR_VEC_RESERVE);
#if 0
    if(icons) { // TODO maybe get rid of this if and assert icons up top?
        //printf("COPY %zu icons\n", icons->len);
        memcpy(&node->icons.items, icons->items, sizeof(*icons->items) * ((icons->len < ICON_BUNDLE_MAX) ? icons->len : ICON_BUNDLE_MAX));
        //vicon_copy(&node->icons, icons);
    }
#endif
    //node->icons.items[0].time = icon;
    //node->icons.items[0].id = ICON_BUNDLE_TIME;
    TRYC(str_copy(&node->title, title));
    if(desc && str_length(desc)) TRY(str_fmt(&node->desc, "%.*s", STR_F(desc)), ERR_STR_FMT);
    if(cmd && str_length(cmd)) TRY(str_fmt(&node->cmd, "%.*s", STR_F(cmd)), ERR_STR_FMT);
    return 0;
error:
    return -1;
}

int node_follow(Node **node, size_t *sub_sel)
{
    ASSERT_ARG(node);
    ASSERT_ARG(sub_sel);
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
    ASSERT_ARG(node);
    ASSERT_ARG(sub_sel);
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




