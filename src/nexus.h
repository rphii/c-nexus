#ifndef NEXUS_H

#include "err.h"
#include "str.h"
#include "node.h"
#include "vector.h"
#include "lookup.h"
#include "view.h"
#include "arg.h"

#define NEXUS_TAG_IDENTIFIER    "🏷️"

#define ICON_ROOT F("📚 ROOT", FG_BK_B)
#define ICON_TAG  "#tag"
#define ICON_WIKI F("📖 WIKI", FG_GN)
#define ICON_MATH F("🧮 MATH", FG_MG)
#define ICON_PHYSICS F("🌌 PHYS", FG_BL)
#define ICON_HISTORY F("🏛️ HIST", FG_YL)
#define ICON_NOTE F("✏️ NOTE", FG_BK_B)
#define ICON_DATE F("📅 DATE", FG_RD)
#define ICON_NONE "-"

#define NEXUS_LOOKUP_NOTES  12
#define NEXUS_LOOKUP_ICONS  8

typedef struct NexusCore {
    TNode nodes;
    //TrNode icons;
} NexusCore;

typedef struct Nexus {
    NexusCore core;
    //TNode nodes;
    //TrNode icons;
    //TNodeIcon nodesicon;
    //Node nodeicon;
    VView views;
    View view;
    bool quit;
    struct {
        VsStr *files;
        Str entry;
        Str extensions;
        ViewList view;
        bool show_desc;
        bool show_preview;
        size_t max_preview;
        size_t max_file_size;
    } config;
    Node findings;
    Node tags;
    bool findings_updated;
    Arg *args;
} Nexus;

#define ERR_NEXUS_INIT "failed initialization of nexus"
#define ERR_nexus_init(nexus) "failed initialization of nexus"
ErrDecl nexus_init(Nexus *nexus);

#define ERR_NEXUS_ARG "failed applying arguments"
#define ERR_nexus_arg(...) "failed applying arguments"
ErrDecl nexus_arg(Nexus *nexus, Arg *arg);

void nexus_free(Nexus *nexus);

#define NEXUS_ROOT  "Nexus"

#define ERR_NEXUS_INSERT_NODE "failed insertion of node into nexus"
//ErrDecl nexus_insert_node(Nexus *nexus, Node *node);
#define ERR_nexus_insert_node(nexus, ref, title, cmd, desc, icons) "failed insertion of node into nexus"
ErrDecl nexus_insert_node(NexusCore *core, Node **ref, Str *title, Str *cmd, Str *desc);
//ErrDecl nexus_insert_node(Nexus *nexus, Node **ref, Str *title, Str *cmd, Str *desc, Icon icon);

// TODO this below because I am losing my mind otherwise
#if 0
#define nexus_tag_node_ERR(nexus, ref, temp, icon) "failed tagging node"
ErrDecl nexus_tag_node(Nexus *nexus, Node *node, Node *temp, IconBundle icon);
#endif

#define NEXUS_INSERT(nexus, root, ref, icon, cmd, title_note, description, ...)  do { \
        Node *temp, unused; \
        TRY(nexus_insert_node(&nexus->core, &temp, &STR_L(title_note), &STR_L(cmd), &STR_L(description)), ERR_NEXUS_INSERT_NODE); \
        TRY(nexus_link(&nexus->core, &(root)->title, &temp->title, 0, (size_t *)SIZE_MAX), ERR_NEXUS_LINK); \
        Str tagstr = icon ? STR_L(icon) : STR(ICON_NONE); \
        TRY(nexus_tag(&nexus->core, &temp->title, &tagstr, 0, (size_t *)SIZE_MAX), ERR_NEXUS_TAG); \
        NEXUS_LINKS_EV_STR(nexus, temp, __VA_ARGS__); \
        if(ref != 0) { \
            memcpy(ref != 0 ? ref : &unused, temp, sizeof(*temp)); \
        } \
    } while(0)

#define ERR_NEXUS_LINK "failed linking nodes"
#define ERR_nexus_link(nexus, src, dst, made_link, ...) "failed linking nodes"
//ErrDecl nexus_link(Nexus *nexus, Node *src, Node *dst, size_t *linked);
//ErrDecl nexus_link(Nexus *nexus, Str *src, Str *dest, size_t *linked);
//ErrDecl nexus_link(NexusCore *core, Str *src, Str *dest, size_t *linked);
ErrDecl nexus_link(NexusCore *core, Str *src, Str *dest, size_t *linked, size_t *count);

#define ERR_NEXUS_TAG "failed tagging nodes"
#define ERR_nexus_tag(nexus, src, dst, made_tag, ...) "failed tagging nodes"
//ErrDecl nexus_tag(Nexus *nexus, Node *src, Node *tag, size_t *tagged);
//ErrDecl nexus_tag(NexusCore *core, Str *src, Str *tag, size_t *tagged);
ErrDecl nexus_tag(NexusCore *core, Str *src, Str *tag, size_t *tagged, size_t *count);

#define NEXUS_LINKS_EV_STR(nexus, src, ...)     do { \
        char *arr64789[] = {__VA_ARGS__}; \
        for(size_t i64789 = 0; i64789+1 < sizeof(arr64789) / sizeof(char *)+1; i64789++) { \
            char *s = arr64789[i64789]; \
            if(!s) continue; \
            Node e64789 = {.title = STR_L(s)}; \
            TRY(nexus_link(&nexus->core, &src->title, &e64789.title, 0, (size_t *)SIZE_MAX), ERR_NEXUS_LINK); \
        } \
    } while(0)

#define ERR_NEXUS_USERINPUT "failed processing user input"
ErrDecl nexus_userinput(Nexus *nexus, int key);

#define ERR_NEXUS_GET "failed getting nexus node"
Node *nexus_get(Nexus *nexus, Str *title);

#define ERR_NEXUS_FIND_OR_CREATE "failed finding or creating node"
ErrDecl nexus_find_or_create(Nexus *nexus, Node *find, Node **found);

#define ERR_NEXUS_SEARCH "failed searching nexus"
ErrDecl nexus_search(Nexus *nexus, Node *anchor, Str *search, Node *results);

#define ERR_NEXUS_FOLLOW_SUB "failed following current nexus node"
ErrDecl nexus_follow_sub(Nexus *nexus, View *view);

#define ERR_NEXUS_CHANGE_VIEW "failed changing view"
ErrDecl nexus_change_view(Nexus *nexus, View *view, ViewList id);

#define ERR_NEXUS_HISTORY_BACK "failed going back in history"
ErrDecl nexus_history_back(Nexus *nexus, View *view);

#define ERR_NEXUS_BUILD "failed building nexus"
ErrDecl nexus_build(Nexus *nexus, VsStr *files);

#define ERR_NEXUS_BUILD_PHYSICS "failed building physics"
ErrDecl nexus_build_physics(Nexus *nexus, Node *anchor);

#define ERR_nexus_create_if_nonexist(nexus, title, ...) "failed creating node: '%.*s'", STR_F(title)
ErrDecl nexus_create_if_nonexist(NexusCore *core, Str *title, size_t *count);

#define ERR_nexus_add_text(nexus, title, ...) "failed adding text to node: '%.*s'", STR_F(title)
ErrDecl nexus_add_text(NexusCore *core, Str *title, Str *text);

#define ERR_nexus_merge(...) "failed fusing nexus cores"
ErrDecl nexus_merge(NexusCore *dst, NexusCore *src, size_t *links);

int nexus_current_view_arg(Nexus *nexus);

#define NEXUS_H
#endif

