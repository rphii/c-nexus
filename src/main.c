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
    printf("Building...\n");

    Str p = {0};
    Nexus nexus = {0};
    TRY(nexus_init(&nexus), ERR_NEXUS_INIT);

    while(!nexus.quit) {
        platform_clear();
        str_clear(&p);
        TRY(view_fmt(&nexus, &p, &nexus.view), ERR_VIEW_FMT);
        printf("%.*s", STR_F(&p));
        int key = platform_getch();
        TRY(nexus_userinput(&nexus, key), ERR_NEXUS_USERINPUT);
    }

error:
    str_free(&p);
    nexus_free(&nexus);
    printf("\ndone.\n");
    return 0;
}

