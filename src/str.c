#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* inclusion and configuration of vector */
#include "str.h"
#include "platform.h"

#define VEC_SETTINGS_DEFAULT_SIZE STR_DEFAULT_SIZE
#define VEC_SETTINGS_KEEP_ZERO_END 1
#define VEC_SETTINGS_STRUCT_ITEMS s

VEC_IMPLEMENT(Str, str, char, BY_VAL, 0);

/* other functions */

void str_pop_back_char(Str *str)
{
    bool next;
    do {
        next = false;
        size_t len = str_length(str);
        char c = 0;
        if(len) {
            str_pop_back(str, &c);
            next = (bool)((c & 0xC0) == 0x80);
        }
    } while(next);
}

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

int str_get_str(Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    int err = 0;
    int c = 0;
    while((c = getchar()) != '\n' && c != EOF) {
        TRY(str_fmt(str, "%c", (char)c), ERR_STR_FMT);  /* append string */
    }
    if(!str->last && (!c || c == EOF || c == '\n')) {
        //THROW("an error"); /* TODO describe this error */
    }
clean:
    fflush(stdin);
    return err;
error: ERR_CLEAN;
}

ErrDecl str_fmt_ext(Str *ext, Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    ASSERT(ext, ERR_NULL_ARG);
    size_t len = str_length(str);
    if(len) {
        size_t i = str_rch(str, '.', 0);
        if(i < len) {
            TRYF(str_fmt, ext, "%.*s", (int)(len - i), str_iter_at(str, i));
        }
    }
    return 0;
error:
    return -1;
}

ErrDecl str_fmt_noext(Str *ext, Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    ASSERT(ext, ERR_NULL_ARG);
    size_t len = str_length(str);
    if(len) {
        size_t iE = str_rch(str, '.', 0);
        TRYF(str_fmt, ext, "%.*s", (int)(iE), str_iter_begin(str));
    }
    return 0;
error:
    return -1;
}

ErrDecl str_fmt_basename(Str *basename, Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    ASSERT(basename, ERR_NULL_ARG);
    size_t len = str_length(str);
    if(len) {
        size_t iE = str_rch(str, '.', 0);
        size_t i0 = str_rch(str, '/', 0);
        if(i0 < len && PLATFORM_CH_SUBDIR != '/') {
            i0 = str_rch(str, PLATFORM_CH_SUBDIR, 0);
        }
        if(i0 < len) ++i0;
        else if(i0 >= len) i0 = 0;
        TRYF(str_fmt, basename, "%.*s", (int)(iE - i0), str_iter_at(str, i0));
    }
    return 0;
error:
    return -1;
}

// TODO: what if up is larger than the directory string? what should be returned then??
ErrDecl str_fmt_dir(Str *dir, Str *str, size_t up)
{
    ASSERT(str, ERR_NULL_ARG);
    ASSERT(dir, ERR_NULL_ARG);
    size_t len = str_length(str);
    size_t len_dir = str_length(dir);
    if(len) {
        size_t i = str_rch(str, '/', up);
        if(i < len) {
            TRYF(str_fmt, dir, "%.*s", (int)(i+1), str_iter_begin(str));
        }
        else if(PLATFORM_CH_SUBDIR != '/') {
            i = str_rch(str, PLATFORM_CH_SUBDIR, up);
            if(i < len) {
                TRYF(str_fmt, dir, "%.*s", (int)(i+1), str_iter_begin(str));
            }
        }
    }
    if(len_dir == str_length(dir)) {
        TRYF(str_fmt, dir, ".");
    }
    return 0;
error:
    return -1;
}

ErrDecl str_fmt_nodir(Str *nodir, Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    ASSERT(nodir, ERR_NULL_ARG);
    size_t len = str_length(str);
    if(len) {
        size_t i0 = str_rch(str, '/', 0);
        if(i0 < len && PLATFORM_CH_SUBDIR != '/') {
            i0 = str_rch(str, PLATFORM_CH_SUBDIR, 0);
        }
        if(i0 < len) ++i0;
        else if(i0 >= len) i0 = 0;
        TRYF(str_fmt, nodir, "%.*s", (int)(len - i0), str_iter_at(str, i0));
    }
    return 0;
error:
    return -1;
}

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

void str_triml(Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    while(str->first < str->last) {
        char c = str->s[str->first];
        if(!isspace(c)) break;
        ++str->first;
    }
}

void str_trimr(Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    while(str->last > str->first) {
        char c = str->s[str->last - 1];
        if(!isspace(c)) break;
        --str->last;
    }
}

void str_trim(Str *str)
{
    ASSERT(str, ERR_NULL_ARG);
    str_triml(str);
    str_trimr(str);
}

size_t str_rch(Str *str, char ch, size_t n)
{
    size_t ni = 0;
    for(size_t i = str_length(str); i > 0; --i) {
        char c = str_get_at(str, i - 1);
        if(c == ch) {
            if(ni == n) return i - 1;
            ++ni;
        }
    }
    return str_length(str);
}


size_t str_hash(Str *a)
{
    ASSERT(a, ERR_NULL_ARG);
    size_t hash = 5381;
    size_t i = 0;
    while(i < str_length(a)) {
        unsigned char c = (unsigned char)str_get_at(a, i++);
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

size_t str_hash_ci(Str *a)
{
    ASSERT(a, ERR_NULL_ARG);
    size_t hash = 5381;
    size_t i = 0;
    while(i < str_length(a)) {
        unsigned char c = (unsigned char)tolower(str_get_at(a, i++));
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}


