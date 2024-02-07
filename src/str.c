#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* inclusion and configuration of vector */
#include "str.h"

#define VEC_SETTINGS_DEFAULT_SIZE STR_DEFAULT_SIZE
#define VEC_SETTINGS_KEEP_ZERO_END 1
#define VEC_SETTINGS_STRUCT_ITEMS s

VEC_IMPLEMENT(Str, str, char, BY_VAL, 0);

/* other functions */

#if 1
int str_fmt(Str *str, char *format, ...)
{
    if(!str) return -1;
    if(!format) return -1;
    // calculate length of append string
    va_list argp;
    va_start(argp, format);
    size_t len_app = (size_t)vsnprintf(0, 0, format, argp);
    if((int)len_app < 0) {
        return -1;
    }
    va_end(argp);
    // calculate required memory
    size_t len_new = str->last + len_app;
    if(str_reserve(str, len_new)) {
        return -1;
    }
    // actual append
    va_start(argp, format);
    int len_chng = vsnprintf(&(str->s)[str->last], len_app + 1, format, argp);
    va_end(argp);
    // check for success
    if(len_chng >= 0 && (size_t)len_chng <= len_app) {
        str->last += (size_t)len_chng; // successful, change length
    } else {
        return -1;
    }
    return 0;
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

