#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* inclusion and configuration of vector */
#include "vector.h"
#include "str.h"
#include "platform.h"

#define VEC_SETTINGS_DEFAULT_SIZE STR_DEFAULT_SIZE
#define VEC_SETTINGS_KEEP_ZERO_END 1
#define VEC_SETTINGS_STRUCT_ITEMS s

VEC_IMPLEMENT(Str, str, char, BY_VAL, 0);

/* other functions */

// basic, no fail, manipulation function {{{

void str_pop_back_char(Str *str) //{{{
{
    ASSERT_ARG(str);
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
} //}}}

void str_pop_back_word(Str *str) //{{{
{
    ASSERT_ARG(str);
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
} //}}}

void str_triml(Str *str) //{{{
{
    ASSERT_ARG(str);
    while(str->first < str->last) {
        char c = str->s[str->first];
        if(!isspace(c)) break;
        ++str->first;
    }
} //}}}

void str_trimr(Str *str) //{{{
{
    ASSERT_ARG(str);
    while(str->last > str->first) {
        char c = str->s[str->last - 1];
        if(!isspace(c)) break;
        --str->last;
    }
} //}}}

void str_trim(Str *str) //{{{
{
    ASSERT_ARG(str);
    str_triml(str);
    str_trimr(str);
} //}}}

// }}}

// pseudo directory {{{

void str_cstr(Str *str, char *cstr, size_t len) {
    ASSERT_ARG(str);
    ASSERT_ARG(cstr);
    cstr[0] = 0;
    snprintf(cstr, len, "%.*s", STR_F(str));
}

inline int str_fmt_va(Str *str, const char *format, va_list argp) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(format);
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
} //}}}

int str_fmt(Str *str, const char *format, ...) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(format);
    if(!str) return -1;
    if(!format) return -1;
    // calculate length of append string
    va_list argp;
    va_start(argp, format);
    int result = str_fmt_va(str, format, argp);
    va_end(argp);
    return result;
} //}}}

ErrDecl str_fmt_ext(Str *ext, const Str *str) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(ext);
    size_t len = str_length(str);
    if(len) {
        size_t i = str_rch(str, '.', 0);
        if(i < len) {
            /* in case we have something like: file.dir/filename -> / is after . */
            size_t j = str_rch(str, PLATFORM_CH_SUBDIR, 0);
            if((j < len && j < i) || (j == len)) {
                TRYF(str_fmt, ext, "%.*s", (int)(len - i), str_iter_at(str, i));
            }
        }
    }
    return 0;
error:
    return -1;
} //}}}

ErrDecl str_fmt_noext(Str *ext, const Str *str) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(ext);
    size_t len = str_length(str);
    if(len) {
        size_t iE = str_rch(str, '.', 0);
        TRYF(str_fmt, ext, "%.*s", (int)(iE), str_iter_begin(str));
    }
    return 0;
error:
    return -1;
} //}}}

ErrDecl str_fmt_basename(Str *basename, const Str *str) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(basename);
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
} //}}}

// TODO: what if up is larger than the directory string? what should be returned then??
ErrDecl str_fmt_dir(Str *dir, const Str *str, size_t up) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(dir);
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
} //}}}

ErrDecl str_fmt_nodir(Str *nodir, const Str *str) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(nodir);
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
} //}}}

//}}}

int str_get_str(Str *str) //{{{
{
    ASSERT_ARG(str);
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
} //}}}

ErrDecl str_fmt_line(Str *line, const Str *str, size_t *i0) { //{{{
    ASSERT_ARG(line);
    ASSERT_ARG(str);
    ASSERT_ARG(i0);
    Str fake = *str;
    fake.first += *i0;
    size_t i = str_ch(&fake, '\n', 0);
    TRYF(str_fmt, line, "%.*s", (int)i, str_iter_begin(&fake));
    *i0 += i + 1; // TODO do I have to/should I check for if i<str_length(str)???
    return 0;
error:
    return -1;
} //}}}

// comparing stuff {{{

int str_cmp(const Str *a, const Str *b) //{{{
{
    ASSERT_ARG(a);
    ASSERT_ARG(b);
    int result = -1;
    if(str_length(a) != str_length(b)) {
        return result;
    }
    result = memcmp(str_iter_begin(a), str_iter_begin(b), str_length(a));
    return result;
} //}}}

int str_cmp_ci(const Str *a, const Str *b) {/*{{{*/
    ASSERT_ARG(a);
    ASSERT_ARG(b);
    if(str_length(a) != str_length(b)) return -1;
    for (size_t i = 0; i < str_length(a); ++i) {
        int d = tolower(str_get_at(a, i)) - tolower(str_get_at(b, i));
        if (d != 0) return d;
    }
    return 0;
}/*}}}*/

int str_cmp_ci_any(const Str *a, const Str **b, size_t len) {/*{{{*/
    ASSERT_ARG(a);
    ASSERT_ARG(b);
    for (size_t i = 0; i < len; ++i) {
        const Str *bb = b[i];
        int result = str_cmp_ci(a, bb);
        if(!result) return 0;
    }
    return -1;
}/*}}}*/

inline size_t str_count_overlap(const Str *restrict a, const Str *restrict b, bool ignorecase) //{{{
{
    ASSERT_ARG(a);
    ASSERT_ARG(b);
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
} //}}}

inline size_t str_find_substring(const Str *restrict str, const Str *restrict sub) //{{{
{
    ASSERT_ARG(str);
    ASSERT_ARG(sub);
    /* basic checks */
    if(!str_length(sub)) return 0;
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
} //}}}

size_t str_find_any(const Str *str, const Str *any) { //{{{
    ASSERT_ARG(str);
    ASSERT_ARG(any);
    size_t result = str_length(str);
    for(size_t i = 0; i < str_length(any); ++i) {
        size_t temp = str_ch(str, str_get_at(any, i), 0);
        if(temp < result) result = temp;
    }
    return result;
} //}}}

size_t str_find_nany(const Str *str, const Str *any) { //{{{
    ASSERT_ARG(str);
    ASSERT_ARG(any);
    size_t result = str_length(str);
    for(size_t i = 0; i < str_length(any); ++i) {
        size_t temp = str_nch(str, str_get_at(any, i), 0);
        if(temp < result) result = temp;
    }
    return result;
} //}}}

size_t str_nch(const Str *str, char ch, size_t n) { //{{{
    ASSERT_ARG(str);
    size_t ni = 0;
    for(size_t i = 0; i < str_length(str); ++i) {
        char c = str_get_at(str, i);
        if(c != ch) {
            if(ni == n) return i;
            ++ni;
        }
    }
    return str_length(str);
} //}}}

size_t str_ch(const Str *str, char ch, size_t n) { //{{{
    ASSERT_ARG(str);
    size_t ni = 0;
    for(size_t i = 0; i < str_length(str); ++i) {
        char c = str_get_at(str, i);
        if(c == ch) {
            if(ni == n) return i;
            ++ni;
        }
    }
    return str_length(str);
} //}}}

size_t str_ch_pair(const Str *str, char c1) { //{{{
    ASSERT_ARG(str);
    size_t level = 1;
    char c0 = str_get_at(str, 0);
    for(size_t i = 1; i < str_length(str); ++i) {
        char c = str_get_at(str, i);
        if(c == c0) level++;
        else if(c == c1) level--;
        if(level <= 0) return i;
    }
    return str_length(str);
} //}}}

size_t str_find_ws(const Str *str) { //{{{
    ASSERT_ARG(str);
    for(size_t i = 0; i < str_length(str); ++i) {
        char c = str_get_at(str, i);
        if(isspace(c)) return i;
    }
    return str_length(str);
} //}}}

size_t str_find_nws(const Str *str) { //{{{
    ASSERT_ARG(str);
    for(size_t i = 0; i < str_length(str); ++i) {
        char c = str_get_at(str, i);
        if(!isspace(c)) return i;
    }
    return str_length(str);
} //}}}

// find reverse non-whitespace
size_t str_find_rnws(const Str *str) { //{{{
    ASSERT_ARG(str);
    for(size_t i = str_length(str); i > 0; --i) {
        char c = str_get_at(str, i - 1);
        if(!isspace(c)) return i - 1;
    }
    return str_length(str);
} //}}}

size_t str_rch(const Str *str, char ch, size_t n) //{{{
{
    ASSERT_ARG(str);
    size_t ni = 0;
    for(size_t i = str_length(str); i > 0; --i) {
        char c = str_get_at(str, i - 1);
        if(c == ch) {
            if(ni == n) return i - 1;
            ++ni;
        }
    }
    return str_length(str);
} //}}}

size_t str_rnch(const Str *str, char ch, size_t n) {
    ASSERT_ARG(str);
    size_t ni = 0;
    for(size_t i = str_length(str); i > 0; --i) {
        char c = str_get_at(str, i - 1);
        if(c != ch) {
            if(ni == n) return i - 1;
            ++ni;
        }
    }
    return 0; //str_length(str);
}

size_t str_count_ch(const Str *str, char ch) {/*{{{*/
    ASSERT_ARG(str);
    size_t result = 0;
    for(size_t i = 0; i < str_length(str); ++i) {
        char c = str_get_at(str, i);
        if(c == ch) ++result;
    }
    return result;
}/*}}}*/

size_t str_irch(const Str *str, size_t iE, char ch, size_t n) { //{{{
    ASSERT_ARG(str);
    if(iE <= str_length(str)) {
        size_t ni = 0;
        for(size_t i = iE; i > 0; --i) {
            char c = str_get_at(str, i - 1);
            if(c == ch) {
                if(ni == n) return i - 1;
                ++ni;
            }
        }
    }
    return str_length(str);
} //}}}

size_t str_hash(const Str *a) //{{{
{
    ASSERT_ARG(a);
    size_t hash = 5381;
    size_t i = 0;
    while(i < str_length(a)) {
        unsigned char c = (unsigned char)str_get_at(a, i++);
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
} //}}}

size_t str_hash_ci(const Str *a) //{{{
{
    ASSERT_ARG(a);
    size_t hash = 5381;
    size_t i = 0;
    while(i < str_length(a)) {
        unsigned char c = (unsigned char)tolower(str_get_at(a, i++));
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
} //}}}

//}}}

ErrDecl str_remove_escapes(Str *restrict out, Str *restrict in)
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
#if 0
            } else if(c == '\'') {
                TRY(str_fmt(out, "%.*s\'\\\'\'", (int)(i - iX), str_iter_at(in, iX)), ERR_STR_FMT);
                iX = i + 1;
#endif
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

