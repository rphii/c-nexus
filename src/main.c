//usr/bin/env tcc -DPROC_COUNT=4 $(ls *.c | grep -v main.c) -run "$0" "$@" ; exit $?

#include <stdio.h>

#include "platform.h"
#include "err.h"
#include "nexus.h"
#include "arg.h"
#include "screen.h"
//#include "colorprint.h"
#include "str.h"
#include "info.h"

//#include <ctype.h>


#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>

/* TODO: move this to someplace better */
struct sigaction old_action;

void sigint_handler(int sig_no)
{
    info_handle_abort();
    screen_leave();
    printf("\n");
    //printf("CTRL-C pressed\n");
    sigaction(SIGINT, &old_action, NULL);
    kill(0, SIGINT);
}


int main(int argc, const char **argv)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

    int err = 0;

    Str p = {0};
    Arg arg = {0};
    Nexus nexus = {0};

    info_disable_all(INFO_LEVEL_ALL);
    info_enable(INFO_nexus_stats, INFO_LEVEL_ALL);
    info_enable(INFO_nexus_init, INFO_LEVEL_ALL);

    info_enable(INFO_parsing_file, INFO_LEVEL_ALL);

    info_enable(INFO_parsing_create_note, INFO_LEVEL_TEXT | INFO_LEVEL_FILE_LINE);
    info_enable(INFO_parsing_add_text, INFO_LEVEL_TEXT | INFO_LEVEL_FILE_LINE);

    info_enable(INFO_parsing_found_format, INFO_LEVEL_TEXT | INFO_LEVEL_FILE_LINE);
    info_enable(INFO_parsing_found_link, INFO_LEVEL_TEXT | INFO_LEVEL_FILE_LINE);
#if 0
    info_enable(INFO_parsing_found_text, INFO_LEVEL_TEXT | INFO_LEVEL_FILE_LINE);
    info_enable(INFO_parsing_found_note, INFO_LEVEL_TEXT | INFO_LEVEL_FILE_LINE);
    info_enable(INFO_parsing_found_text, INFO_LEVEL_TEXT | INFO_LEVEL_FILE_LINE);


    info_enable(INFO_parsing_skip_too_large, INFO_LEVEL_ALL);
    //info_enable(INFO_parsing_skip_incorrect_extension, INFO_LEVEL_ALL);
    //info_enable(INFO_skipping_nofile_nodir, INFO_LEVEL_ALL);
#endif

    TRY(platform_colorprint_init(), ERR_PLATFORM_COLORPRINT_INIT);

    TRY(arg_parse(&arg, argc, argv), ERR_ARG_PARSE);
    if(arg.exit_early) goto clean;

    info(INFO_nexus_init, "Building the Nexus...");
    TRY(nexus_arg(&nexus, &arg), ERR_NEXUS_ARG);
    TRY(nexus_init(&nexus), ERR_NEXUS_INIT);
    info(INFO_nexus_init, "Successfully initialized");
    getchar();
    //goto clean;
    screen_enter();

    while(!nexus.quit) {
        str_clear(&p);
        platform_clear();
        TRY(view_fmt(&nexus, &p, &nexus.view), ERR_VIEW_FMT);
        printf("%.*s", STR_F(&p));
        int key = platform_getch();
        TRY(nexus_userinput(&nexus, key), ERR_NEXUS_USERINPUT);
    }

clean:
    info_handle_abort();
    nexus_free(&nexus);
    arg_free(&arg);
    str_free(&p);
    screen_leave();
    return err;
error:
    ERR_CLEAN;
}

