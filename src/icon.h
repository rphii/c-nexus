#ifndef ICON_H

typedef enum {
    ICON_NONE,
    ICON_NOTE,
    ICON_ROOT,
    ICON_WIKI,
    ICON_MATH,
    ICON_PHYSICS,
    ICON_HISTORY,
} Icon;

char *icon_str(Icon icon);

#define ICON_H
#endif

