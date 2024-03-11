#ifndef NEXUS_H

#include "err.h"
#include "str.h"
#include "node.h"
#include "vector.h"
#include "lookup.h"
#include "view.h"
#include "arg.h"

typedef struct Nexus {
    TNode nodes;
    TNodeIcon nodesicon;
    Node nodeicon;
    VView views;
    View view;
    bool quit;
    struct {
        Str entry;
        ViewList view;
        bool show_desc;
        bool show_preview;
        size_t max_preview;
    } config;
    Node findings;
    bool findings_updated;
    Arg *args;
} Nexus;

#define ERR_NEXUS_INIT "failed initialization of nexus"
ErrDecl nexus_init(Nexus *nexus);

#define ERR_NEXUS_ARG "failed applying arguments"
ErrDecl nexus_arg(Nexus *nexus, Arg *arg);

void nexus_free(Nexus *nexus);

#define NEXUS_ROOT  "Nexus"

#define ERR_NEXUS_INSERT_NODE "failed insertion of node into nexus"
//ErrDecl nexus_insert_node(Nexus *nexus, Node *node);
ErrDecl nexus_insert_node(Nexus *nexus, Node **ref, char *title, char *cmd, char *desc, Icon icon);

#define NEXUS_INSERT(nexus, root, ref, icon, cmd, title, description, ...)  do { \
        Node *temp, unused; \
        TRY(nexus_insert_node(nexus, &temp, title, cmd, description, icon), ERR_NEXUS_INSERT_NODE); \
        TRY(nexus_link(nexus, root, temp), ERR_NEXUS_LINK); \
        NEXUS_LINKS_EV_STR(nexus, temp, __VA_ARGS__); \
        if(ref != 0) { \
            memcpy(ref != 0 ? ref : &unused, temp, sizeof(*temp)); \
        } \
    } while(0)

#define ERR_NEXUS_LINK "failed linking nodes"
ErrDecl nexus_link(Nexus *nexus, Node *src, Node *dst);

#define NEXUS_LINKS_EV_STR(nexus, src, ...)     do { \
        char *arr64789[] = {__VA_ARGS__}; \
        for(size_t i64789 = 0; i64789+1 < sizeof(arr64789) / sizeof(char *)+1; i64789++) { \
            char *s = arr64789[i64789]; \
            if(!s) continue; \
            Node e64789 = {.title = STR_L(s)}; \
            TRY(nexus_link(nexus, src, &e64789), ERR_NEXUS_LINK); \
        } \
    } while(0)

#define ERR_NEXUS_USERINPUT "failed processing user input"
ErrDecl nexus_userinput(Nexus *nexus, int key);

#define ERR_NEXUS_GET "failed getting nexus node"
Node *nexus_get(Nexus *nexus, const char *title);

#define ERR_NEXUS_SEARCH "failed searching nexus"
ErrDecl nexus_search(Nexus *nexus, Str *search, VrNode *findings);

#define ERR_NEXUS_FOLLOW_SUB "failed following current nexus node"
ErrDecl nexus_follow_sub(Nexus *nexus, View *view);

#define ERR_NEXUS_CHANGE_VIEW "failed changing view"
ErrDecl nexus_change_view(Nexus *nexus, View *view, ViewList id);

#define ERR_NEXUS_HISTORY_BACK "failed going back in history"
ErrDecl nexus_history_back(Nexus *nexus, View *view);

#define ERR_NEXUS_BUILD "failed building nexus"
ErrDecl nexus_build(Nexus *nexus);

#define ERR_NEXUS_BUILD_PHYSICS "failed building physics"
ErrDecl nexus_build_physics(Nexus *nexus, Node *anchor);

#define NEXUS_H
#endif

