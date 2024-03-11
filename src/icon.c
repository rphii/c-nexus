#include "icon.h"
#include "colorprint.h"

char *icon_str(Icon icon)
{
    switch(icon) {
        case ICON_ROOT: return F("📚 ROOT", FG_BK_B);
        case ICON_WIKI: return F("📖 WIKI", FG_GN);
        case ICON_MATH: return F("🧮 MATH", FG_MG);
        case ICON_PHYSICS: return F("🌌 PHYS", FG_BL);
        case ICON_HISTORY: return F("🏛️ HIST", FG_YL);
        case ICON_NOTE: return F("✏️ NOTE", FG_BK_B);
        case ICON_NONE: // fallthrough
        default: return F("-", FG_BK_B);
    }
}

