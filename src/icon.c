#include "icon.h"
#include "colorprint.h"

char *icon_str(Icon icon)
{
    switch(icon) {
        case ICON_ROOT: return F("ROOT", FG_GN_B);
        case ICON_WIKI: return F("WIKI", FG_GN_B);
        case ICON_MATH: return F("MATH", FG_GN_B);
        case ICON_PHYSICS: return F("PHYS", FG_GN_B);
        case ICON_HISTORY: return F("HIST", FG_GN_B);
        case ICON_NONE: // fallthrough
        default: return F("-", FG_GN_B);
    }
}

