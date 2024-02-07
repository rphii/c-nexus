#ifndef NEXUS_H

#include "err.h"
#include "str.h"
#include "node.h"
#include "vector.h"
#include "lookup.h"

typedef struct Nexus {
    TNode nodes;
} Nexus;

#define ERR_NEXUS_INIT "failed initialization of nexus"
ErrDecl nexus_init(Nexus *nexus);
void nexus_free(Nexus *nexus);

#define NEXUS_ROOT  "Nexus"

#define ERR_NEXUS_INSERT_NODE "failed insertion of node into nexus"
ErrDecl nexus_insert_node(Nexus *nexus, Node *node);

#define NEXUS_INSERT(nexus, root, ref, icon, title, description, ...)  do { \
        Node temp, unused; \
        TRY(node_create(&temp, title, description, icon), ERR_NODE_CREATE); \
        TRY(nexus_insert_node(nexus, &temp), ERR_NEXUS_INSERT_NODE); \
        TRY(nexus_link(nexus, root, &temp), ERR_NEXUS_LINK); \
        NEXUS_LINKS_EV_STR(nexus, &temp, __VA_ARGS__); \
        if(ref != 0) { \
            memcpy(ref != 0 ? ref : &unused, &temp, sizeof(temp)); \
        } \
    } while(0)

#define ERR_NEXUS_LINK "failed linking nodes"
ErrDecl nexus_link(Nexus *nexus, Node *src, Node *dst);

#define NEXUS_LINKS_EV_STR(nexus, src, ...)     do { \
        for(size_t i64789 = 0; i64789+1 < sizeof((char *[]){__VA_ARGS__}) / sizeof(char *)+1; i64789++) { \
            char *s = (char *[]){__VA_ARGS__}[i64789]; \
            Node e64789 = {.title = STR_L(s)}; \
            TRY(nexus_link(nexus, src, &e64789), ERR_NEXUS_LINK); \
        } \
    } while(0)

#define ERR_NEXUS_GET "failed getting nexus node"
Node *nexus_get(Nexus *nexus, const char *title);

#define ERR_NEXUS_BUILD "failed building nexus"
ErrDecl nexus_build(Nexus *nexus);

#define NEXUS_H
#endif
