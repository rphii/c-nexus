#include <time.h>

#include "str.h"
#include "icon.h"
#include "colorprint.h"


char *icon_str(IconList icon)
{
    switch(icon) {
        case ICON_ROOT: return F("📚 ROOT", FG_BK_B);
        case ICON_WIKI: return F("📖 WIKI", FG_GN);
        case ICON_MATH: return F("🧮 MATH", FG_MG);
        case ICON_PHYSICS: return F("🌌 PHYS", FG_BL);
        case ICON_HISTORY: return F("🏛️ HIST", FG_YL);
        case ICON_NOTE: return F("✏️ NOTE", FG_BK_B);
        case ICON_DATE: return F("📅 DATE", FG_RD);
        case ICON_NONE: // fallthrough
        default: return F("-", FG_BK_B);
    }
}

void icon_fmt(IconStr str, Icon icon)
{
    if(icon < 0) {
        snprintf(str, ICON_STR_LEN, "%s", icon_str(icon));
    } else {
        time_t tt = (time_t)icon;
        struct tm *t = localtime(&tt);
        strftime(str, ICON_STR_LEN, F("📅 %Y-%m-%d", FG_RD), t);
    }
}

Icon icon_base(int year, int month, int day, int hour, int minute, int second)
{
    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    time_t tt = mktime(&t);
    return (Icon)tt;
}

