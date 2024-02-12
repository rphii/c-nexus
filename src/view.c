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
            TRY(node_fmt(out, current, nexus->show_desc, "", false), ERR_NODE_FMT);
            TRY(node_fmt_sub(out, current, nexus->show_desc, nexus->show_preview, view->sub_sel), ERR_NODE_FMT_SUB);
        } break;
        case VIEW_SEARCH: {
            ASSERT(nexus->max_preview, "max_preview cannot be 0");
            /* first perform the search */
            Str *search = &view->search;
            VrNode *findings = &nexus->findings;
            TRY(nexus_search(nexus, search, findings), ERR_NEXUS_SEARCH);
            /* check that sub selection is in bounds */
            size_t sub_sel = view->sub_sel;
            size_t sub_max = vrnode_length(findings);
            if(sub_sel >= sub_max) sub_sel = sub_max ? sub_max - 1 : 0;
            /* check that we display all items up to max_preview ... and shift accordingly */
            size_t ioff = sub_sel >= nexus->max_preview ? 1 + sub_sel - nexus->max_preview : 0;
            TRY(str_fmt(out, "Found " F("%4zu", FG_YL_B) " for : %.*s%s\n\n", vrnode_length(findings), STR_F(search), view->edit ? "_" : ""), ERR_STR_FMT);
            /* finally create output */
            Node *node_desc = 0;
            for(size_t i = 0; i < vrnode_length(findings) && i < nexus->max_preview; ++i) {
                size_t ireal = i+ioff;
                Node *node = vrnode_get_at(findings, ireal);
                char *select = !view->edit ? "  > " : "";
                if(!view->edit && sub_sel == ireal) {
                    select = "--> ";
                    node_desc = node;
                }
                TRY(node_fmt(out, node, false, select, (node_desc == node)), ERR_NODE_FMT);
            }
            if(vrnode_length(findings) > nexus->max_preview) {
                TRY(str_fmt(out, F("... (" F("%4zu", IT FG_YL_B) " more)\n", IT), vrnode_length(findings) - nexus->max_preview), ERR_STR_FMT);
            }
            if(nexus->show_desc && node_desc) {
                if(str_length(&node_desc->desc)) {
                    TRY(str_fmt(out, "\n%.*s\n\n", STR_F(&node_desc->desc)), ERR_STR_FMT);
                } else {
                    TRY(str_fmt(out, F("\nno description.\n\n", IT)), ERR_STR_FMT);
                }
            }
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
    str_zero(&dst->search);
    TRY(str_copy(&dst->search, &src->search), ERR_STR_COPY);
    return 0;
error:
    return -1;
}

