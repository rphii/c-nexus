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
        }
    }

error:
    nexus_free(&nexus);
    printf("done.\n");
    return 0;
}

