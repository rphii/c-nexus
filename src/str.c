#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* inclusion and configuration of vector */
#include "str.h"

#define VEC_SETTINGS_DEFAULT_SIZE STR_DEFAULT_SIZE
#define VEC_SETTINGS_KEEP_ZERO_END 1
#define VEC_SETTINGS_STRUCT_ITEMS s

VEC_IMPLEMENT(Str, str, char, BY_VAL, 0);

/* other functions */

#if 1

void str_pop_back_word(Str *str)
{
    size_t len = str_length(str);
    if(len) {
        int ws = isspace(str_get_at(str, --len));
        while(len) {
            char c = str_get_at(str, --len);
            int wsI = isspace(c);
            if(ws && !wsI) { ++len; break; }
            if(!ws && wsI) { ++len; break; }
        }
    }
    str->last = str->first + len;
}

inline int str_fmt_va(Str *str, char *format, va_list argp)
{
    va_list argp2;
    va_copy(argp2, argp);
    size_t len_app = (size_t)vsnprintf(0, 0, format, argp2);
    va_end(argp2);

    if((int)len_app < 0) {
        return -1;
    }
    // calculate required memory
    size_t len_new = str->last + len_app;
    if(str_reserve(str, len_new)) {
        return -1;
    }
    // actual append
    int len_chng = vsnprintf(&(str->s)[str->last], len_app + 1, format, argp);
    // check for success
    if(len_chng >= 0 && (size_t)len_chng <= len_app) {
        str->last += (size_t)len_chng; // successful, change length
    } else {
        return -1;
    }
    return 0;
}

int str_fmt(Str *str, char *format, ...)
{
    if(!str) return -1;
    if(!format) return -1;
    // calculate length of append string
    va_list argp;
    va_start(argp, format);
    int result = str_fmt_va(str, format, argp);
    va_end(argp);
    return result;
}
#endif

int str_cmp(Str *a, Str *b)
{
    int result = -1;
    if(str_length(a) != str_length(b)) {
        return result;
    }
    result = memcmp(str_iter_begin(a), str_iter_begin(b), str_length(a));
    return result;
}

inline size_t str_count_overlap(Str *restrict a, Str *restrict b, bool ignorecase)
{
    size_t overlap = 0;
    size_t len = str_length(a) > str_length(b) ? str_length(b) : str_length(a);
    if(!ignorecase) {
        for(size_t i = 0; i < len; ++i) {
            char ca = str_get_at(a, i);
            char cb = str_get_at(b, i);
            if(ca == cb) ++overlap;
            else break;
        }
    } else {
        for(size_t i = 0; i < len; ++i) {
            int ca = tolower(str_get_at(a, i));
            int cb = tolower(str_get_at(b, i));
            if(ca == cb) ++overlap;
            else break;
        }
    }
    return overlap;
}

inline size_t str_find_substring(Str *restrict str, Str *restrict sub)
{
    /* basic checks */
    if(!str_length(sub)) return 1;
    if(str_length(sub) > str_length(str)) {
        return 0;
    }
    /* store original indices */
    Str ref = *str;
    /* check for substring */
    size_t i = 0;
    while(str_length(sub) <= str_length(&ref)) {
        size_t overlap = str_count_overlap(&ref, sub, true);
        if(overlap == str_length(sub)) {
            return i + 1;
        } else {
            i += overlap + 1;
            ref.first += overlap + 1;
        }
    }
    /* restore original */
    return 0;
}

size_t str_hash(Str *a)
{
    size_t hash = 5381;
    size_t i = 0;
    while(i < str_length(a)) {
        unsigned char c = (unsigned char)str_get_at(a, i++);
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

