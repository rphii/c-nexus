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
#define STR_L(string)           (Str){.s = string, .last = string ? strlen(string) : 0}
#define STR_LL(string, length)  (Str){.s = string, .last = length}

#define STR_F(s)                (int)str_length(s), str_iter_begin(s)

#define ERR_STR_CAT_BACK    "failed appending string to other string"
#define ERR_STR_FMT         "failed string formatting"
#define ERR_STR_COPY        "failed copying string"

/* other functions */

void str_pop_back_char(Str *str);
void str_pop_back_word(Str *str);

ErrDecl str_fmt_va(Str *str, char *format, va_list argp);
ErrDecl str_fmt(Str *str, char *format, ...);

#define ERR_STR_GET_STR     "failed getting string from user"
ErrDecl str_get_str(Str *str);

int str_cmp(Str *a, Str *b);
size_t str_count_overlap(Str *a, Str *b, bool ignorecase);
size_t str_find_substring(Str *str, Str *sub);
size_t str_hash(Str *a);
size_t str_hash_ci(Str *a);


#define STR_H
#endif

