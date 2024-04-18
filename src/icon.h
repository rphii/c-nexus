#ifndef ICON_H

#include <stdint.h>

#include "str.h"
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

typedef enum {
    ICON_BUNDLE_NONE,
    ICON_BUNDLE_TIME,
    ICON_BUNDLE_STR,
} IconBundleList;

typedef int64_t IconTime;
typedef int64_t Icon;

#define ICON_BUNDLE_MAX     8

#if 0
typedef struct IconBundle {
    IconBundleList id;
    Str str;
    IconTime time;
} IconBundle;
#else
typedef struct IconBundle {
    Str str;
    IconTime time;
    IconBundleList id;
} IconBundle;
#endif

typedef IconBundle VIcon[ICON_BUNDLE_MAX];

void icon_free(IconBundle *icon);

#define icon_fmt_ERR(out, icons) "failed formatting icon"
ErrDecl icon_fmt(Str *out, VIcon icons);

#define ICON_STR_LEN    48
typedef char IconStr[ICON_STR_LEN]; // TODO rename this to icondatestr or something

void icon_free(IconBundle *icon);

char *icon_str(IconList icon);
//void icon_fmt(IconStr str, Icon icon);
Icon icon_base(int year, int month, int day, int hour, int minute, int second);


#define ICON_H
#endif

