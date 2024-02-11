#include <stdlib.h>
#include <stdio.h>

#include "platform.h"
#include "err.h"
#include "node.h"
#include "vector.h"
#include "lookup.h"

#include "nexus.h"
#include "search.h"

int main(void)
{
#if 0
    Str cmd = {0}, content = {0};
    Str append = STR(F("Hello", FG_GN BOLD) ", " F("World", BG_RD IT) "!");
    int found = search_fmt_nofree(true, &cmd, &content, &STR("hello, world!"), "%s", F("Hello", FG_GN BOLD) ", " F("World", BG_RD IT) "!");
    printf("%s\n", found ? "found" : "NOT found");
    //printf("CONTENT:%s\n", content.s);
    //printf("CMD:%s\n", cmd.s);
    str_free(&cmd);
    str_free(&content);

    return 0;
#endif

    printf("Building...\n");

    Nexus nexus = {0};
    TRY(nexus_init(&nexus), ERR_NEXUS_INIT);
    TRY(nexus_build(&nexus), ERR_NEXUS_BUILD);

    Node *current = 0;
    TRY(!(current = nexus_get(&nexus, NEXUS_ROOT)), ERR_NEXUS_GET);

    bool show_desc = true;
    bool quit = false;

    while(!quit) {
        platform_clear();
        TRY(node_print(current, show_desc), ERR_NODE_PRINT);
        int key = platform_getch();
        switch(key) {
            case ' ': {
                show_desc ^= true;
            } break;
            case 'j': {
                node_mv_vertical(current, 1);
            } break;
            case 'k': {
                node_mv_vertical(current, -1);
            } break;
            case 'l': {
                TRY(nexus_follow_sub(&nexus, &current), ERR_NEXUS_FOLLOW_SUB);
            } break;
            case 'h': {
                nexus_history_back(&nexus, &current);
            } break;
            case 'Q':
            case 'q': {
                quit = true;
            } break;
            case 'f': {
                /* THIS IS REALLY WHACKY !!! WILL HAVE TO BE MADE BETTER :) BUT... THIS WORKS :) */
                bool edit = true;
                Str search = {0};
                Str p = {0};
                VrNode findings = {0};
                size_t sub_sel = 0;
                while(!quit) {
                    //platform_clear();
                    str_clear(&p);
                    platform_clear();
                    TRY(nexus_search(&nexus, &search, &findings), ERR_NEXUS_SEARCH);
                    printf("Found %zu for : %.*s%s\n", vrnode_length(&findings), STR_F(&search), edit ? "_" : "");
                    
                    if(!edit) {
                        size_t sub_sel_max = vrnode_length(&findings);
                        if(SIZE_IS_NEG(sub_sel)) {
                            sub_sel = sub_sel_max ? sub_sel_max - 1 : 0;
                        } else if(sub_sel >= sub_sel_max) {
                            sub_sel = 0;
                        }
                    }

                    Node *node_desc = 0;
                    for(size_t i = 0; i < vrnode_length(&findings); i++) {
                        Node *node = vrnode_get_at(&findings, i);
                        size_t sO = vrnode_length(&node->outgoing);
                        size_t sI = vrnode_length(&node->incoming);
                        if(edit) {
                            //printf("%zu : %s %.*s\n", i+1, icon_str(node->icon), STR_F(&node->title));
                            //printf("%zu : %s %.*s\n", i+1, icon_str(node->icon), STR_F(&node->title));
                            TRY(str_fmt(&p, "%s " NODE_FMT_LEN_SUB " :: %.*s \n", icon_str(node->icon), sO+sI, STR_F(&node->title)), ERR_STR_FMT);
                            //node_print(
                        } else {
                            TRY(str_fmt(&p, "%s " NODE_FMT_LEN_SUB " :: %s %.*s \n", icon_str(node->icon), sO+sI, i==sub_sel?"-->":"  >", STR_F(&node->title)), ERR_STR_FMT);
                            if(i==sub_sel && show_desc) {
                                node_desc = node;
                            }
                            //printf("%zu %s : %s %.*s\n", i+1, i==sub_sel ? "-->" : "  >", icon_str(node->icon), STR_F(&node->title));
                        }
                    }
                    if(!edit && node_desc && str_length(&node_desc->desc)) {
                        TRY(str_fmt(&p, "\n%.*s\n\n", STR_F(&node_desc->desc)), ERR_STR_FMT);
                    } else {
                        TRY(str_fmt(&p, F("\nno description.\n\n", IT)), ERR_STR_FMT);
                    }
                    printf("%.*s", STR_F(&p));

                    key = platform_getch();
                    if(edit) {
                        if(key >= 0x20 && key != 127) {
                            TRY(str_fmt(&search, "%c", key), ERR_STR_FMT);
                        } else if(key == 127) {
                            if(str_length(&search)) str_pop_back(&search, 0);
                        } else if(key == 8) {
                            str_pop_back_word(&search);
                        } else if(key == '\n') {
                            edit = false;
                        }
                    } else {
                        if(key == 'j') {
                            sub_sel++;
                        } else if(key == 'k') {
                            sub_sel--;
                        } else if(key == 'l') {
                            Node *node = vrnode_get_at(&findings, sub_sel);
                            TRY(vrnode_push_back(&nexus.history, current), ERR_VEC_PUSH_BACK);
                            //current = nexus_get(&nexus, node->title));
                            TRY(!(current = nexus_get(&nexus, node->title.s)), ERR_NEXUS_GET);
                            quit = true;
                        } else if(key == '\n' || key == 'q' || key == 'h') {
                            quit = true;
                        } else if(key == 'f') {
                            edit = true;
                        }
                    }
                }
                str_free(&search);
                str_free(&p);
                vrnode_free(&findings);
                quit = false;
            } break;
        }
    }

error:
    nexus_free(&nexus);
    printf("\ndone.\n");
    return 0;
}

