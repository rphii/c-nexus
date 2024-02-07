#include <stdlib.h>
#include <stdio.h>

#include "platform.h"
#include "err.h"
#include "node.h"
#include "vector.h"
#include "lookup.h"

#include "nexus.h"

int main(void)
{
    printf("Building...\n");

    Nexus nexus = {0};
    TRY(nexus_init(&nexus), ERR_NEXUS_INIT);
    TRY(nexus_build(&nexus), ERR_NEXUS_BUILD);

    VrNode stack = {0};
    Node *current = 0;
    TRY(!(current = nexus_get(&nexus, NEXUS_ROOT)), ERR_NEXUS_GET);

    size_t sub_index = 0;
    bool show_desc = true;
    bool quit = false;

    while(!quit) {
        platform_clear();
        TRY(node_print(current, show_desc, sub_index), ERR_NODE_PRINT);
        int key = platform_getch();
        switch(key) {
            case ' ': {
                show_desc ^= true;
            } break;
            case 'j': {
                node_set_sub(current, &sub_index, sub_index + 1);
            } break;
            case 'k': {
                node_set_sub(current, &sub_index, sub_index - 1);
            } break;
            case 'l': {
                TRY(vrnode_push_back(&stack, current), ERR_VEC_PUSH_BACK);
                TRY(node_follow(&current, &sub_index), ERR_NODE_FOLLOW);
            } break;
            case 'h': {
                if(vrnode_length(&stack)) {
                    vrnode_pop_back(&stack, &current);
                    node_set(current, &sub_index);
                }
            } break;
            case 'Q':
            case 'q': {
                quit = true;
            } break;
        }
    }

error:
    nexus_free(&nexus);
    vrnode_free(&stack);
    printf("done.\n");
    return 0;
}

