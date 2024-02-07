#include "nexus.h"

int nexus_init(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    TRY(tnode_init(&nexus->nodes, 12), ERR_LUTD_INIT);
    return 0;
error:
    return -1;
} //}}}

void nexus_free(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    tnode_free(&nexus->nodes);
} //}}}

Node *nexus_get(Nexus *nexus, const char *title) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(title, ERR_NULL_ARG);
    Node *result = 0;
    size_t i0 = 0, j0 = 0;
    Node find = {0};
    TRY(node_create(&find, title, "", 0), ERR_NODE_CREATE);
    if(tnode_find(&nexus->nodes, &find, &i0, &j0)) {
        THROW("node does not exist in nexus: '%.*s'", STR_F(&find.title));
    }
    result = nexus->nodes.buckets[i0].items[j0];
clean:
    node_free(&find);
    return result;
error:
    goto clean;
} //}}}

int nexus_insert_node(Nexus *nexus, Node *node) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    if(tnode_has(&nexus->nodes, node)) THROW("should not insert node with equal title");
    TRY(tnode_add(&nexus->nodes, node), ERR_LUTD_ADD);
    return 0;
error:
    return -1;
} //}}}

int nexus_link(Nexus *nexus, Node *src, Node *dest) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(src, ERR_NULL_ARG);
    ASSERT(dest, ERR_NULL_ARG);
    if(!tnode_has(&nexus->nodes, src)) THROW("node does not exist in nexus: '%.*s'", STR_F(&src->title));
    if(!tnode_has(&nexus->nodes, dest)) THROW("node does not exist in nexus: '%.*s'", STR_F(&dest->title));
    size_t i0 = 0, i1 = 0, j0 = 0, j1 = 0;
    tnode_find(&nexus->nodes, src, &i0, &j0);
    tnode_find(&nexus->nodes, dest, &i1, &j1);
    Node *ev_src = nexus->nodes.buckets[i0].items[j0];
    Node *ev_dest = nexus->nodes.buckets[i1].items[j1];
    Icon i_src = ev_src->icon;
    Icon i_dest = ev_dest->icon;
    if(i_src == i_dest) {
        // ...
    } else if(i_src > i_dest) {
        Node *temp = ev_src;
        ev_src = ev_dest;
        ev_dest = temp;
    }
    TRY(vrnode_push_back(&ev_src->outgoing, ev_dest), ERR_VEC_PUSH_BACK);
    TRY(vrnode_push_back(&ev_dest->incoming, ev_src), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
} //}}}

#define ERR_NEXUS_BUILD_PHYSICS "failed building physics"
int nexus_build_physics(Nexus *nexus, Node *anchor)
{
    Node base;
    NEXUS_INSERT(nexus, anchor, &base, ICON_PHYSICS, "Physics", "");
    NEXUS_INSERT(nexus, &base, 0, ICON_PHYSICS, "Second", "A second is an SI-unit of time.");
    NEXUS_INSERT(nexus, &base, 0, ICON_PHYSICS, "Meter", "A meter is an SI-unit of length.");
    NEXUS_INSERT(nexus, &base, 0, ICON_PHYSICS, "Speed of Light", "The speed of light " F("in vacumm", IT) " is\n" F("c = 299'792'458 meters/second", BOLD) "", "Second", "Meter");
    return 0;
error:
    return -1;
}

int nexus_build(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);

    Node root;
    TRY(node_create(&root, NEXUS_ROOT, "", ICON_NONE), ERR_NODE_CREATE);
    TRY(nexus_insert_node(nexus, &root), ERR_NEXUS_INSERT_NODE);

    TRY(nexus_build_physics(nexus, &root), ERR_NEXUS_BUILD_PHYSICS);

    return 0;
error:
    return -1;
} //}}}

