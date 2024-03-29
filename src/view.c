#include "view.h"
#include "nexus.h"

void view_free(View *view)
{
    str_free(&view->search);
    memset(view, 0, sizeof(*view));
}

int view_fmt(Nexus *nexus, Str *out, View *view)
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(view, ERR_NULL_ARG);
    switch(view->id) {
        case VIEW_NORMAL: {
            Node *current = view->current;
            TRY(node_fmt(out, current, nexus->config.show_desc, "", 0, 0, false), ERR_NODE_FMT);
            TRY(node_fmt_sub(out, current, nexus->config.show_desc, nexus->config.show_preview, nexus->config.max_preview, view->sub_sel), ERR_NODE_FMT_SUB);
        } break;
        case VIEW_SEARCH_ALL: {
            /* first perform the search */
            Str *search = &view->search;
            Node *findings = &nexus->findings;
            if(!nexus->findings_updated) {
                TRY(nexus_search(nexus, 0, search, findings), ERR_NEXUS_SEARCH);
                /* on THIS line I suggest sorting stuff! */
                vrnode_sort(&findings->outgoing);
                vrnode_sort(&findings->incoming);
                nexus->findings_updated = true;
            }
            /* check that sub selection is in bounds */
            size_t sub_sel = view->sub_sel;
            size_t sub_max = vrnode_length(&findings->outgoing);
            if(sub_sel >= sub_max) sub_sel = sub_max ? sub_max - 1 : 0;
            if(view->edit) sub_sel = SIZE_MAX;
            TRY(str_fmt(out, "Found " F("%4zu", FG_YL_B) " for : %.*s%s\n\n", vrnode_length(&findings->outgoing)+vrnode_length(&findings->incoming), STR_F(search), view->edit ? "_" : ""), ERR_STR_FMT);
            TRY(node_fmt_sub(out, findings, nexus->config.show_desc, nexus->config.show_preview, nexus->config.max_preview, sub_sel), ERR_NODE_FMT_SUB);
        } break;
        case VIEW_SEARCH_SUB: {
            /* first perform the search */
            Str *search = &view->search;
            Node *findings = &nexus->findings;
            ASSERT(view->search_on, "Cannot search on null!");
            if(!nexus->findings_updated) {
                TRY(nexus_search(nexus, view->search_on, search, findings), ERR_NEXUS_SEARCH);
                /* on THIS line I suggest sorting stuff! */
                vrnode_sort(&findings->outgoing);
                vrnode_sort(&findings->incoming);
                nexus->findings_updated = true;
            }
            /* check that sub selection is in bounds */
            size_t sub_sel = view->sub_sel;
            size_t sO = vrnode_length(&findings->outgoing);
            size_t sI = vrnode_length(&findings->incoming);
            size_t sub_max = sO+sI;
            if(sub_sel >= sub_max) sub_sel = sub_max ? sub_max - 1 : 0;
            if(view->edit) sub_sel = SIZE_MAX;
            TRY(str_fmt(out, "Found " F("%4zu", FG_YL_B) " on '%.*s' for : %.*s%s\n\n", vrnode_length(&findings->outgoing)+vrnode_length(&findings->incoming), STR_F(&view->search_on->title), STR_F(search), view->edit ? "_" : ""), ERR_STR_FMT);
            TRY(node_fmt_sub(out, findings, nexus->config.show_desc, nexus->config.show_preview, nexus->config.max_preview, sub_sel), ERR_NODE_FMT_SUB);
        } break;
        case VIEW_ICON: {
            Node *current = &nexus->nodeicon;
            TRY(node_fmt(out, current, nexus->config.show_desc, "", 0, 0, false), ERR_NODE_FMT);
            TRY(node_fmt_sub(out, current, nexus->config.show_desc, nexus->config.show_preview, nexus->config.max_preview, view->sub_sel), ERR_NODE_FMT_SUB);
        } break;
        case VIEW_NONE: THROW("view id should not be NONE");
        default: THROW("unknown id: %u", view->id);
    }
    return 0;
error:
    return -1;
}

int view_copy(View *dst, View *src)
{
    ASSERT(dst, ERR_NULL_ARG);
    ASSERT(src, ERR_NULL_ARG);
    memset(dst, 0, sizeof(*dst));
    dst->current = src->current;
    dst->edit = src->edit;
    dst->id = src->id;
    dst->sub_sel = src->sub_sel;
    dst->search_on = src->search_on;
    str_zero(&dst->search);
    TRY(str_copy(&dst->search, &src->search), ERR_STR_COPY);
    return 0;
error:
    return -1;
}

