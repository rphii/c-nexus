#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

#include "search.h"
#include "icon.h"

#if 0
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
            } else if(!isspace(c_last) && isspace(c)) {
                TRY(str_fmt(out, "%.*s ", (int)(i - iX), str_iter_at(in, iX)), ERR_STR_FMT);
                iX = i + 1;
            } else if(isspace(c_last) && isspace(c)) {
                iX = i + 1;
            }
            if(!skip) c_last = c;
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
#endif

int search_fmt_nofree(bool ignorecase, Str *buf_searchon, Str *buf_formatted, Str *find, char *format, ...)
{
    /* form format string */
    va_list argp;
    va_start(argp, format);
    int result = str_fmt_va(buf_formatted, format, argp);
    va_end(argp);
    /* search */
    int found = search_nofree(ignorecase, buf_searchon, find, buf_formatted);
    return (found && !result);
}

// *find* string in *content* and *buf_searchon* is allocated by this function. free outside
int search_nofree(bool ignorecase, Str *buf_searchon, Str *find, Str *content)
{
    ASSERT(buf_searchon, ERR_NULL_ARG);
    ASSERT(content, ERR_NULL_ARG);
    ASSERT(find, ERR_NULL_ARG);
    int found = 0;
#if 1
    TRYF(str_remove_escapes, buf_searchon, content);
    if(!str_length(find)) {
        found = -1;
    } else {
        found = (int)((str_find_substring(buf_searchon, find)));
    }
#else
    TRY(str_fmt(buf_searchon, "if echo '"), ERR_STR_FMT);
    TRY(search_static_remove_escapes(buf_searchon, buf_formatted), ERR_SEARCH_STATIC_REMOVE_ESCAPES);
    TRY(str_fmt(buf_searchon, "' | grep -F %s -q '", ignorecase ? "-i" : ""), ERR_STR_FMT);
    TRY(search_static_remove_escapes(buf_searchon, find), ERR_SEARCH_STATIC_REMOVE_ESCAPES);
    TRY(str_fmt(buf_searchon, "' 2>/dev/null; then exit 0; else exit 1; fi"), ERR_STR_FMT);
    //printf("CMD:%.*s\n", STR_F(buf_searchon));
    found = !system(buf_searchon->s);
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



