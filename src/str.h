#ifndef STR_H

#define STR_DEFAULT_SIZE 32

#include <stdarg.h>
#include <stdbool.h>

#include "err.h"

/* configuration, inclusion and de-configuration of vector */

#define VEC_SETTINGS_DEFAULT_SIZE STR_DEFAULT_SIZE
#define VEC_SETTINGS_KEEP_ZERO_END 1
#define VEC_SETTINGS_STRUCT_ITEMS s
#include "vec.h"

VEC_INCLUDE(Str, str, char, BY_VAL);

#undef VEC_SETTINGS_STRUCT_ITEMS
#undef VEC_SETTINGS_KEEP_ZERO_END
#undef VEC_SETTINGS_DEFAULT_SIZE

#define STR(string)             (Str){.s = string, .last = sizeof(string)/sizeof(*string)-1}
#define STR_L(string)           (Str){.s = string, .last = strlen(string ? string : "")}
#define STR_LL(string, length)  (Str){.s = string, .last = length}

#define STR_F(s)                (int)str_length(s), str_iter_begin(s)

#define STR_I0(str, i0)         (const Str){.s = (str).s, .first = (str).first + i0, .last = (str).last}
#define STR_IE(str, iE)         (const Str){.s = (str).s, .first = (str).first, .last = (str).last - iE}

#define ERR_STR_CAT_BACK    "failed appending string to other string"
#define ERR_STR_FMT         "failed string formatting"
#define ERR_STR_COPY        "failed copying string"

/* other functions */

void str_pop_back_char(Str *str);
void str_pop_back_word(Str *str);
void str_triml(Str *str);
void str_trimr(Str *str);
void str_trim(Str *str);

#define str_fmt_va_ERR(str, format, argp) "failed formatting string"
ErrDecl str_fmt_va(Str *str, const char *format, va_list argp);
#define str_fmt_ERR(str, format, ...) "failed formatting string"
ErrDecl str_fmt(Str *str, const char *format, ...);
#define str_fmt_ext_ERR(ext, str) "failed formatting extension"
ErrDecl str_fmt_ext(Str *ext, const Str *str); // extract extension
#define str_fmt_noext_ERR(ext, str) "failed removing extension"
ErrDecl str_fmt_noext(Str *ext, const Str *str); // remove extension
#define str_fmt_dir_ERR(dir, str, up) "failed formatting directory"
ErrDecl str_fmt_dir(Str *dir, const Str *str, size_t up); // extract directory
#define str_fmt_nodir_ERR(nodir, str) "failed formatting without directory"
ErrDecl str_fmt_nodir(Str *nodir, const Str *str); // remove directory
#define str_fmt_basename_ERR(basename, str) "failed formatting basename"
ErrDecl str_fmt_basename(Str *basename, const Str *str); // remove extention+directory

#define str_fmt_line_ERR(line, str, i0) "failed getting line (index %zu / length %zu)", *i0, str_length(str)
ErrDecl str_fmt_line(Str *line, const Str *str, size_t *i0);

#define ERR_STR_GET_STR     "failed getting string from user"
ErrDecl str_get_str(Str *str);

int str_cmp(const Str *a, const Str *b);
size_t str_count_overlap(const Str *a, const Str *b, bool ignorecase);
size_t str_find_substring(const Str *str, const Str *sub);
size_t str_find_any(const Str *str, const Str *any);
size_t str_ch(const Str *str, char ch, size_t n);
size_t str_ch_pair(const Str *str, char c1);
size_t str_find_ws(const Str *str);
size_t str_rch(const Str *str, char ch, size_t n);
size_t str_irch(const Str *str, size_t iE, char ch, size_t n);
size_t str_hash(const Str *a);
size_t str_hash_ci(const Str *a);

#define STR_H
#endif

