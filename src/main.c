#include <stdio.h>

#include "platform.h"
#include "err.h"
#include "nexus.h"
#include "arg.h"
#include "screen.h"

int main(int argc, const char **argv)
{
    screen_enter();

    int err = 0;

    Str p = {0};
    Arg arg = {0};
    Nexus nexus = {0};

    TRY(platform_colorprint_init(), ERR_PLATFORM_COLORPRINT_INIT);

    TRY(arg_parse(&arg, argc, argv), ERR_ARG_PARSE);
    if(arg.exit_early) goto clean;

    INFO("Building the Nexus...");
    TRY(nexus_arg(&nexus, &arg), ERR_NEXUS_ARG);
    TRY(nexus_init(&nexus), ERR_NEXUS_INIT);

    while(!nexus.quit) {
        platform_clear();
        str_clear(&p);
        TRY(view_fmt(&nexus, &p, &nexus.view), ERR_VIEW_FMT);
        printf("%.*s", STR_F(&p));
        int key = platform_getch();
        TRY(nexus_userinput(&nexus, key), ERR_NEXUS_USERINPUT);
    }

clean:
    nexus_free(&nexus);
    arg_free(&arg);
    str_free(&p);
    screen_leave();
    return err;
error:
    ERR_CLEAN;
}

