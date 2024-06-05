//usr/bin/env tcc -DPROC_COUNT=4 $(ls *.c | grep -v main.c) -run "$0" "$@" ; exit $?

#include <stdio.h>

#include "platform.h"
#include "err.h"
#include "nexus.h"
#include "arg.h"
//#include "screen.h"
//#include "colorprint.h"
#include "str.h"
#include "info.h"

//#include <ctype.h>

int main(int argc, const char **argv)
{
    int err = 0;

    Str p = {0};
    Arg arg = {0};
    Nexus nexus = {0};

    info_disable_all(INFO_LEVEL_ALL);
    info_enable(INFO_parsing_file, INFO_LEVEL_ALL);
    info_enable(INFO_parsing_found_link, INFO_LEVEL_TEXT);
    info_enable(INFO_parsing_found_note, INFO_LEVEL_TEXT);
    //info_disable(INFO_parsing_skip_incorrect_extension, INFO_LEVEL_ALL);
    //info_disable_all(INFO_LEVEL_ID);
    //info_disable(INFO_skipping_nofile_nodir, INFO_LEVEL_IS_INFO | INFO_LEVEL_FILE_LINE | INFO_LEVEL_FUNCTION);
    //info_disable(INFO_parsing_file, INFO_LEVEL_IS_INFO | INFO_LEVEL_FILE_LINE | INFO_LEVEL_FUNCTION);

    TRY(platform_colorprint_init(), ERR_PLATFORM_COLORPRINT_INIT);

    TRY(arg_parse(&arg, argc, argv), ERR_ARG_PARSE);
    if(arg.exit_early) goto clean;

    //screen_enter();
    info(nexus_init, "Building the Nexus...");
    TRY(nexus_arg(&nexus, &arg), ERR_NEXUS_ARG);
    TRY(nexus_init(&nexus), ERR_NEXUS_INIT);
    info(nexus_init, "Successfully initialized");
    getchar();
    //goto clean;

    while(!nexus.quit) {
        str_clear(&p);
        //platform_clear();
        TRY(view_fmt(&nexus, &p, &nexus.view), ERR_VIEW_FMT);
        printf("%.*s", STR_F(&p));
        int key = platform_getch();
        TRY(nexus_userinput(&nexus, key), ERR_NEXUS_USERINPUT);
    }

clean:
    nexus_free(&nexus);
    arg_free(&arg);
    str_free(&p);
    //screen_leave();
    return err;
error:
    ERR_CLEAN;
}

