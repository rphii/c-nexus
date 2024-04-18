#include "view.h"
#include "nexus.h"
#include "vector.h"

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
            TRYF(node_fmt, out, current, nexus->config.show_desc, "", 0, 0, false);
            TRY(str_fmt(out, "\n"), ERR_STR_FMT);
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
            char *fmt = "Found " VIEW_FMT_SEARCH_INACTIVE " for : %.*s%s\n\n";
            if(view->edit) {
                fmt = "Found " VIEW_FMT_SEARCH_ACTIVE " for : %.*s%s\n\n";
                sub_sel = SIZE_MAX;
            }
            TRY(str_fmt(out, fmt, 4, vrnode_length(&findings->outgoing)+vrnode_length(&findings->incoming), STR_F(search), view->edit ? VIEW_EDITING_CURSOR : ""), ERR_STR_FMT);
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
            char *fmt = VIEW_FMT_SEARCH_INACTIVE;// " %s %.*s : %.*s%s\n\n";
            if(view->edit) {
                fmt = VIEW_FMT_SEARCH_ACTIVE;// " %s %.*s : %.*s%s\n\n";
                sub_sel = SIZE_MAX;
            }
            TRYF(str_fmt, out, fmt, 4, vrnode_length(&findings->outgoing)+vrnode_length(&findings->incoming));//, iconstr, STR_F(&view->search_on->title), STR_F(search), view->edit ? VIEW_EDITING_CURSOR : ""), ERR_STR_FMT);
            TRYF(icons_fmt, out, view->search_on->icons);
#if 0
            for(size_t i = 0; i < vicon_length(&view->search_on->icons); ++i) {
                IconBundle icon = vicon_get_at(&view->search_on->icons, i);
                IconStr iconstr = {0};
                icon_fmt(iconstr, icon.time);
                TRYF(str_fmt, out, "%s", iconstr);
            }
#endif
            TRY(node_fmt_sub(out, findings, nexus->config.show_desc, nexus->config.show_preview, nexus->config.max_preview, sub_sel), ERR_NODE_FMT_SUB);
        } break;
        case VIEW_ICON: {
            Node *current = nexus->view.current;
            TRY(node_fmt(out, current, nexus->config.show_desc, "", 0, 0, false), ERR_NODE_FMT);
            TRY(str_fmt(out, "\n"), ERR_STR_FMT);
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

