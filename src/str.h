#ifndef STR_H

#define STR_DEFAULT_SIZE 32

/* configuration, inclusion and de-configuration of vector */

#define VEC_SETTINGS_DEFAULT_SIZE STR_DEFAULT_SIZE
#define VEC_SETTINGS_KEEP_ZERO_END 1
#define VEC_SETTINGS_STRUCT_ITEMS s
#include "vec.h"

VEC_INCLUDE(Str, str, char, BY_VAL);

#undef VEC_SETTINGS_STRUCT_ITEMS
#undef VEC_SETTINGS_KEEP_ZERO_END
#undef VEC_SETTINGS_DEFAULT_SIZE

#define STR(string)     (Str){.s = string, .last = sizeof(string)/sizeof(*string)-1}
#define STR_L(string)   (Str){.s = string, .last = strlen(string)}
#define STR_F(s)        (int)str_length(s), str_iter_begin(s)

#define ERR_STR_CAT_BACK    "failed appending string to other string"
#define ERR_STR_FMT         "failed string formatting"

/* other functions */

int str_fmt(Str *str, char *format, ...);

int str_cmp(Str *a, Str *b);
size_t str_hash(Str *a);

#define STR_H
#endif

