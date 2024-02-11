#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

#include "search.h"

#define ERR_SEARCH_STATIC_REMOVE_ESCAPES "failed removing escape sequences"
static inline int search_static_remove_escapes(Str *restrict out, Str *restrict in)
{
    ASSERT(out, ERR_NULL_ARG);
    ASSERT(in, ERR_NULL_ARG);
    bool skip = 0;
    size_t iX = 0;
    char c_last = 0;
    for(size_t i = 0; i < str_length(in); i++) {
        char c = str_get_at(in, i);
        if(!skip) {
            if(c == '\033') {
                TRY(str_fmt(out, "%.*s", (int)(i - iX), str_iter_at(in, iX)), ERR_STR_FMT);
                skip = true;
            } else if(c == '\'') {
                TRY(str_fmt(out, "%.*s\'\\\'\'", (int)(i - iX), str_iter_at(in, iX)), ERR_STR_FMT);
                iX = i + 1;
            } else if(c_last != '\n' && c == '\n') {
                TRY(str_fmt(out, "%.*s ", (int)(i - iX), str_iter_at(in, iX)), ERR_STR_FMT);
                iX = i + 1;
            } else if(isspace(c_last) && isspace(c)) {
                iX = i + 1;
            }
            c_last = c;
        } else {
            if(c == 'm') {
                iX = i + 1;
                skip = false;
            }
        }
    }
    if(!skip) {
        TRY(str_fmt(out, "%.*s", (int)(str_length(in) - iX), str_iter_at(in, iX)), ERR_STR_FMT);
    }
    return 0;
error:
    return -1;
}

int search_fmt_nofree(bool ignorecase, Str *nofree_cmd, Str *nofree_content, Str *find, char *format, ...)
{
    /* form format string */
    va_list argp;
    va_start(argp, format);
    //printf("format [[[%s]]]\n", format);
    int result = str_fmt_va(nofree_content, format, argp);
    //printf("content [[[%s]]]\n", nofree_content->s);
    va_end(argp);
    /* search */
    int found = search_nofree(ignorecase, nofree_cmd, find, nofree_content);
    return (found && !result);
}

int search_nofree(bool ignorecase, Str *nofree_cleaned, Str *find, Str *nofree_content)
{
    ASSERT(nofree_cleaned, ERR_NULL_ARG);
    ASSERT(nofree_content, ERR_NULL_ARG);
    ASSERT(find, ERR_NULL_ARG);
    int found = 0;
#if 1
    TRY(search_static_remove_escapes(nofree_cleaned, nofree_content), ERR_SEARCH_STATIC_REMOVE_ESCAPES);
    found = (int)((str_find_substring(nofree_cleaned, find)));
#else
    TRY(str_fmt(nofree_cleaned, "if echo '"), ERR_STR_FMT);
    TRY(search_static_remove_escapes(nofree_cleaned, nofree_content), ERR_SEARCH_STATIC_REMOVE_ESCAPES);
    TRY(str_fmt(nofree_cleaned, "' | grep -F %s -q '", ignorecase ? "-i" : ""), ERR_STR_FMT);
    TRY(search_static_remove_escapes(nofree_cleaned, find), ERR_SEARCH_STATIC_REMOVE_ESCAPES);
    TRY(str_fmt(nofree_cleaned, "' 2>/dev/null; then exit 0; else exit 1; fi"), ERR_STR_FMT);
    //printf("CMD:%.*s\n", STR_F(nofree_cleaned));
    found = !system(nofree_cleaned->s);
#endif
    return found;
error:
    return 0;
}

int search(bool ignorecase, Str *find, Str *content)
{
    ASSERT(content, ERR_NULL_ARG);
    ASSERT(find, ERR_NULL_ARG);
    Str cmd = {0};
    int found = search_nofree(ignorecase, &cmd, find, content);
    str_free(&cmd);
    return found;
}



