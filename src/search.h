#ifndef SEARCH_H

#include <stdbool.h>
#include "str.h"
#include "err.h"

int search_fmt_nofree(bool ignorecase, Str *nofree_cmd, Str *nofree_content, Str *find, char *format, ...);
int search_fmt(bool ignorecase, Str *find, char *pattern, ...);

int search_nofree(bool ignorecase, Str *cmd, Str *find, Str *nofree_content);
int search(bool ignorecase, Str *find, Str *content);


#define SEARCH_H
#endif

