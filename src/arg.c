#include <ctype.h>
#include <errno.h>

#include "arg.h"
#include "nexus.h"
#include "str.h"

/* arguments */

static const char *static_arg[][2] = {
    [ARG_NONE] = {0, 0},
    [ARG_HELP] = {"-h", "--help"},
    [ARG_VERSION] = {0, "--version"},
    [ARG_ENTRY] = {0, "--entry"},
    [ARG_VIEW] = {0, "--view"},
    [ARG_SHOW_DESCRIPTION] = {"-d", "--show-description"},
    [ARG_SHOW_PREVIEW] = {"-p", "--show-preview"},
    [ARG_MAX_LIST] = {"-l", "--max-list"},
};

const char *arg_str(ArgList id)
{
    return static_arg[id][1];
}

static const char *static_desc[] = {
    [ARG_NONE] = 0,
    [ARG_HELP] = "print this help",
    [ARG_VERSION] = "display the version",
    [ARG_ENTRY] = "specify node to start out on",
    [ARG_VIEW] = "specify the view to start out on",
    [ARG_SHOW_DESCRIPTION] = "show or hide description at startup",
    [ARG_SHOW_PREVIEW] = "show or hide preview at startup",
    [ARG_MAX_LIST] = "maximum notes to show at once (e.g. search) before scrolling",
};

/* specify */

static const Specify static_specify[ARG__COUNT] = {
    [ARG_ENTRY] = SPECIFY(SPECIFY_STRING),
    [ARG_VIEW] = SPECIFY(SPECIFY_OPTION, SPECIFY_NORMAL, SPECIFY_SEARCH),
    [ARG_SHOW_DESCRIPTION] = SPECIFY(SPECIFY_OPTION, SPECIFY_YES, SPECIFY_TRUE, SPECIFY_NO, SPECIFY_FALSE),
    [ARG_SHOW_PREVIEW] = SPECIFY(SPECIFY_OPTION, SPECIFY_NO, SPECIFY_FALSE, SPECIFY_YES, SPECIFY_TRUE),
    [ARG_MAX_LIST] = SPECIFY(SPECIFY_NUMBER)
};

static const char *static_specify_str[] = {
    [SPECIFY_NONE] = "none",
    [SPECIFY_OPTIONAL] = "OPTIONAL",
    [SPECIFY_OPTION] = "OPTION",
        [SPECIFY_NORMAL] = "normal",
        [SPECIFY_SEARCH] = "search",
        [SPECIFY_TRUE] = "true",
        [SPECIFY_YES] = "yes",
        [SPECIFY_FALSE] = "false",
        [SPECIFY_NO] = "no",
    [SPECIFY_NUMBER] = "NUMBER",
    [SPECIFY_STRING] = "STRING",
    [SPECIFY_BOOL] = "< y | n >",
};

const char *specify_str(SpecifyList id)
{
    return static_specify_str[id];
}


int print_line(int max, int current, int tabs, Str *str)
{
    if(!str) return 0;
    int result = 0;
    int printed = 0;
    int length = 0;
    char *until = 0;
    for(size_t i = 0; i < str_length(str); i += (size_t)length) {
        while(isspace((int)str->s[i])) {
            i++;
        }
        if(i >= str_length(str)) break;
        if(!until && i) {
            printf("\n");
        }
        char *s = &str->s[i];
        until = strchr(s, '\n');
        length = until ? (int)(until + 1 - s) : (int)strlen(s);
        if(length + current > max) length = max - current;
        printed = printf("%*s%.*s", tabs, "", length, s);
        tabs = current;
        result += printed;
    }
    return result;
}

#define ERR_ARG_SPECIFIC_OPTIONAL "failed specifying optional argument"
ErrDeclStatic arg_static_specific_optional(Arg *arg, ArgList id, const char *specify)
{
    ASSERT(arg, ERR_NULL_ARG);
    switch(id) {
        default: THROW("id (%u) doesn't have a corresponding specific optional", id);
    }
    return 0;
error:
    return -1;
}

static void arg_static_print_version(Arg *arg)
{
    ASSERT(arg, ERR_NULL_ARG);
#if defined(VERSION)
    screen_leave();
    if(strlen(VERSION)) {
        printf("%s version %s %s\n", arg->name, VERSION, PLATFORM_NAME);
    } else {
        printf(F("failed ", FG_RD)"(git not installed or you didn't clone the repository)\n");
    }
#else
    ABORT("missing version (added in Make as a preprocessor definition)");
#endif
}

#define ERR_ARG_EXECUTE "failed executing argument"
ErrDeclStatic arg_static_execute(Arg *arg, ArgList id)
{
    ASSERT(arg, ERR_NULL_ARG);
    if(id >= ARG__COUNT) THROW("incorrect id (%u)", id);
    SpecifyList *spec = 0;
    switch(id) {
        case ARG_HELP: {
            arg_help(arg);
            arg->exit_early = true;
        } break;
        case ARG_VERSION: {
            arg_static_print_version(arg);
            arg->exit_early = true;
        } break;
        case ARG_ENTRY: {
            if(!str_length(&arg->entry)) {
                screen_leave();
                printf("%*s" F("%s", BOLD) "=STRING is missing\n", arg->tabs.tiny, "", static_arg[id][1]);
                arg->exit_early = true;
            }
        } break;
        case ARG_MAX_LIST: {
            if(!arg->max_list) {
                screen_leave();
                printf("%*s" F("%s", BOLD) "=NUMBER cannot be 0\n", arg->tabs.tiny, "", static_arg[id][1]);
                arg->exit_early = true;
            }
        } break;
        case ARG_VIEW: { spec = &arg->view; } break;
        case ARG_SHOW_PREVIEW: { spec = &arg->show_preview; } break;
        case ARG_SHOW_DESCRIPTION: { spec = &arg->show_description; } break;
        default: THROW(ERR_UNHANDLED_ID" (%d)", id);
    }
    if(spec) {
        switch(*spec) {
            case SPECIFY_OPTION: {
                screen_leave();
                arg->exit_early = true;
                /* list options available */
                const Specify *spec2 = &static_specify[id];
                printf("%*s" F("%s", BOLD) " (one of %zu below)\n", arg->tabs.tiny, "", static_arg[id][1], spec2->len-1);
                for(size_t i = 1; i < spec2->len; i++) {
                    SpecifyList h = spec2->ids[i];
                    printf("%*s=%s%s\n", arg->tabs.main, "", static_specify_str[h], i == 1 ? F(" (default)", IT) : "");
                }
            } break;
            case SPECIFY_STRING: {
                screen_leave();
                arg->exit_early = true;
                printf("%*s" F("%s", BOLD) "=STRING is missing\n", arg->tabs.tiny, "", static_arg[id][1]);
            } break;
            default: break;
        }
    }
    return 0;
error:
    return -1;
}

#define ERR_ARG_ADD_TO_UNKNOWN "failed to add to unknown"
ErrDeclStatic arg_static_add_to_unknown(Arg *arg, Str *s)
{
    ASSERT(arg, ERR_NULL_ARG);
    ASSERT(s, ERR_NULL_ARG);
    bool add_comma = (str_length(&arg->unknown) != 0);
    TRY(str_fmt(&arg->unknown, "%s%.*s", add_comma ? ", " : "", STR_F(s)), ERR_STR_FMT);
    return 0;
error:
    return -1;
}

#define ERR_ARG_PARSE_SPEC "failed parsing specific argument"
ErrDeclStatic static_arg_parse_spec(Arg *args, ArgList arg, Str *argY, Specify spec)
{
    ASSERT(args, ERR_NULL_ARG);
    ASSERT(argY, ERR_NULL_ARG);
    if(!spec.len) return 0;
    SpecifyList id0 = spec.ids[0];
    void *to_set = 0;
    switch(arg) {
        case ARG_VIEW: { to_set = &args->view; } break;
        case ARG_ENTRY: { to_set = &args->entry; } break;
        case ARG_MAX_LIST: { to_set = &args->max_list; } break;
        case ARG_SHOW_PREVIEW: { to_set = &args->show_preview; } break;
        case ARG_SHOW_DESCRIPTION: { to_set = &args->show_description; } break;
        default: THROW("unhandled arg id (%u)", arg);
    }
    switch(id0) {
        case SPECIFY_OPTION: {
            SpecifyList *id_set = (SpecifyList *)to_set;
            *id_set = SPECIFY_OPTION;
            for(size_t k = 1; k < spec.len; ++k) {
                SpecifyList id = spec.ids[k];
                Str specs = STR_L((char *)static_specify_str[id]);
                //printf("  %zu:%.*s == %zu:%.*s\n", specs.last, STR_F(&specs), argY->last, STR_F(argY));
                if(str_cmp(&specs, argY)) continue;
                *id_set = id;
                //printf("  SELECTED!!!\n");
                break;
            }
        } break;
        case SPECIFY_STRING: {
            str_clear((Str *)to_set);
            TRY(str_fmt((Str *)to_set, "%.*s", STR_F(argY)), ERR_STR_FMT);
        } break;
        case SPECIFY_NUMBER: {
            errno = 0;
            char *endptr;
            char *begin = str_iter_begin(argY);
            size_t val = 0;
            if(begin) {
                val = (size_t)strtoll(begin, &endptr, 0);
            }
            if(*endptr) THROW("could not convert to number: %s (because of: %s)", begin, endptr);
            if(errno) THROW("strtoll conversion error");
            *(size_t *)to_set = val;
        } break;
        default: THROW("unhandled id0! (%u)", id0);
    }
    return 0;
error:
    return -1;
}

int arg_parse(Arg *args, int argc, const char **argv) /* {{{ */
{
    ASSERT(args, ERR_NULL_ARG);
    ASSERT(argv, ERR_NULL_ARG);
    /* default arguments */
    args->show_description = static_specify[ARG_SHOW_DESCRIPTION].ids[1];
    args->show_preview = static_specify[ARG_SHOW_PREVIEW].ids[1];
    args->max_list = 20; /* TODO .. make something with this */
    args->view = SPECIFY_NORMAL;
    TRY(str_fmt(&args->entry, "%s", NEXUS_ROOT), ERR_STR_FMT);
    /* set up */
    args->name = argv[0];
    args->tabs.tiny = 2;
    args->tabs.main = 7;
    args->tabs.ext = 34;
    args->tabs.spec = args->tabs.ext + 2;
    args->tabs.max = 80;
    //arg_help(args);
    /* actually parse */
    for(int i = 1; i < argc; ++i) {
        /* verify command line arguments */
        Str arg = STR_L((char *)argv[i]);
        size_t posX = str_find_substring(&arg, &STR("=")) - 1;
        bool unknown_arg = true;
        bool arg_opt = true; /* assume arg is two dashes */
        if(str_length(&arg) == 2) arg_opt = false; /* arg is 1 dash, since too short */
        if(str_length(&arg) >= 2 && posX == 2) arg_opt = false; /* arg is 1 dash, but with = */
        for(ArgList j = 0; j < ARG__COUNT; ++j) {
            Str cmp = STR_L((char *)static_arg[j][arg_opt]);
            Str argX = STR_LL(str_iter_begin(&arg), SIZE_IS_NEG(posX) ? str_length(&arg) : posX);
            Str argY = str_length(&arg) > str_length(&argX) + 1 ? STR_LL(str_iter_at(&arg, str_length(&argX) + 1), str_length(&arg) - str_length(&argX) - 1) : STR("");
            //printf("%.*s // %zi // X %.*s // Y %.*s === %.*s\n", STR_F(&arg), posX, STR_F(&argX), STR_F(&argY), STR_F(&cmp));
            if(!str_length(&cmp)) continue; /* arg to cmp is not even an argument */
            if(str_cmp(&argX, &cmp)) continue; /* arg to cmp to is not equal */
            Specify spec = static_specify[j];
            //printf("FOUND!! -> id %s (HAS %zu)\n", static_arg[j][1], spec.len);
            TRY(static_arg_parse_spec(args, j, &argY, spec), ERR_ARG_PARSE_SPEC);
            unknown_arg = false;
            break;
        }
        if(unknown_arg) {
            TRY(arg_static_add_to_unknown(args, &arg), ERR_ARG_ADD_TO_UNKNOWN);
        }
    }
#if 1
    /* done checking */
    if(str_length(&args->unknown)) {
        THROW("unknown arguments: %.*s", STR_F(&args->unknown));
    } else for(int i = 1; i < argc; i++) {
        /* run arguments */
        const char *arg = argv[i];
        size_t arg_opt = 1; /* assume that argument is a full-string flag */
        if(arg[1] && !arg[2]) arg_opt = 0; /* argument is a one-character flag */
        for(ArgList j = 0; j < ARG__COUNT; j++) {
            if(arg[1] && arg[2] == '=' && static_specify[j].len) arg_opt = 0;
            const char *cmp = static_arg[j][arg_opt];
            if(!cmp) continue;
            size_t cmp_len = strlen(cmp);
            if(strncmp(arg, cmp, cmp_len)) continue;
            TRY(arg_static_execute(args, j), ERR_ARG_EXECUTE);
            if(args->exit_early) break;
        }
        if(args->exit_early) break;
    }
#endif
    return 0;
error:
    return -1;
}; /* }}} */

void arg_help(Arg *arg) /* {{{ */
{
    ASSERT(arg, ERR_NULL_ARG);
    int err = 0;
    Str ts = {0};
    screen_leave();
    print_line(arg->tabs.max, 0, 0, &STR(F("c-nexus:", BOLD)" terminal note browsing application."));
    printf("\n\n");
    print_line(arg->tabs.max, 0, 0, &STR("Usage:\n"));
    TRY(str_fmt(&ts, "%s [options]\n", arg->name), ERR_STR_FMT);
    print_line(arg->tabs.max, arg->tabs.main, arg->tabs.main, &ts);
    printf("\n");
    print_line(arg->tabs.max, 0, 0, &STR("Options:\n"));
    for(size_t i = 0; i < ARG__COUNT; i++) {
        int tabs_offs = 0;
        int tp = 0;
        if(!i) continue;
        const char *arg_short = static_arg[i][0];
        const char *arg_long = static_arg[i][1];
        if(arg_short) {
            str_clear(&ts);
            TRY(str_fmt(&ts, "%s,", arg_short), ERR_STR_FMT);
            tp = print_line(arg->tabs.max, arg->tabs.tiny, arg->tabs.tiny, &ts);
            tabs_offs += tp;
        }
        if(arg_long) {
            str_clear(&ts);
            TRY(str_fmt(&ts, "%s", arg_long), ERR_STR_FMT);
            tp = print_line(arg->tabs.max, arg->tabs.main, arg->tabs.main-tabs_offs, &ts);
            tabs_offs += tp;
            const char *explained = static_desc[i];
            const Specify *specify = &static_specify[i];
            if(specify->len) {
                str_clear(&ts);
                const char *sgl = static_specify_str[specify->ids[0]];
                TRY(str_fmt(&ts, "=%s", sgl ? sgl : F("!!! missing string representation !!!", FG_RD_B)), ERR_STR_FMT);
                tp = print_line(arg->tabs.max, arg->tabs.spec, 0, &ts);
                tabs_offs += tp;
            }
            if(!explained) {
                ABORT("!!! missing argument explanation !!!");
            } else {
                str_clear(&ts);
                TRY(str_fmt(&ts, "%s ", explained), ERR_STR_FMT);
                tp = print_line(arg->tabs.max, arg->tabs.ext, arg->tabs.ext-tabs_offs, &ts);
                tabs_offs += tp;
                if(specify->len > 1) {
                    printf("\n%*s", arg->tabs.ext+2, "");
                    str_clear(&ts);
                    for(size_t j = 1; j < specify->len; j++) {
                        const char *spec = static_specify_str[specify->ids[j]];
                        if(!spec) continue;
                        TRY(str_fmt(&ts, "%s=%s", j == 1 ? "" : ", ", spec), ERR_STR_FMT);
                        if(j == 1) {
                            if(specify->ids[0] == SPECIFY_OPTION) {
                                TRY(str_fmt(&ts, ""F(" (default)", IT)""), ERR_STR_FMT);
                            } else if(specify->ids[0] == SPECIFY_OPTIONAL) {
                                TRY(str_fmt(&ts, ""F(" (optional)", IT)""), ERR_STR_FMT);
                            } else {
                                ABORT("!!! missing behavior hint !!!");
                            }
                        }
                    }
                    tp = print_line(arg->tabs.max, arg->tabs.spec, 0, &ts);
                }
            }
        }
        printf("\n");
    }
    printf("\n");
    printf("GitHub: %s\n", LINK_GITHUB);
clean:
    str_free(&ts);
    return; (void)err;
error: ERR_CLEAN;
} /* }}} */

void arg_free(Arg *arg)
{
    str_free(&arg->entry);
    str_free(&arg->unknown);
}


