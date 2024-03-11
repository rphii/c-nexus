#ifndef ICON_H

#include <stdint.h>

#include "err.h"

typedef enum {
    ICON_NONE = INT64_MIN,
    /* !!! icons below !!! */
    ICON_ROOT,
    ICON_DATE,
    ICON_NOTE,
    ICON_WIKI,
    ICON_MATH,
    ICON_PHYSICS,
    ICON_HISTORY,
} IconList;

typedef int64_t Icon;

#define ICON_STR_LEN    48
typedef char IconStr[ICON_STR_LEN];

char *icon_str(IconList icon);
void icon_fmt(IconStr str, Icon icon);
Icon icon_base(int year, int month, int day, int hour, int minute, int second);

#define ICON_H
#endif

