#include <time.h>

#include "vector.h"
#include "str.h"
#include "icon.h"
#include "colorprint.h"

void icon_free(IconBundle *icon) { /*{{{*/
    ASSERT_ARG(icon);
    str_free(&icon->str);
    memset(icon, 0, sizeof(*icon));
}/*{{{*/

char *icon_str(IconList icon)
{
    switch(icon) {
        case ICON_ROOT: return F("📚 ROOT", FG_BK_B);
        case ICON_TAG: return "#tag"; //return "🏷️";
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

ErrDecl icon_fmt_tag(Str *out, IconBundle icon) {/*{{{*/
    ASSERT_ARG(out);
    switch(icon.id) {
        case ICON_BUNDLE_TIME: {
            if(icon.time < 0) {
                TRYF(str_fmt, out, "%s", icon_str(icon.time));
            } else {
                TRYF(str_fmt, out, "%s", icon_str(ICON_DATE));
            }
        } break;
        case ICON_BUNDLE_STR: {
            TRYF(str_fmt, out, "%.*s", STR_F(&icon.str));
        } break;
        case ICON_BUNDLE_NONE: break;
        default: THROW("wrong icon id: %u", icon.id);
    }
    return 0;
error:
    return -1;
}/*}}}*/

ErrDecl icon_fmt(Str *out, char *lpad, IconBundle icon) {/*{{{*/
    ASSERT_ARG(out);
    char *pad = lpad ? lpad : "";
    switch(icon.id) {
        case ICON_BUNDLE_TIME: {
            if(icon.time < 0) {
                TRYF(str_fmt, out, "%s%s", pad, icon_str(icon.time));
            } else {
                IconStr str = {0};
                time_t tt = (time_t)icon.time;
                struct tm *t = localtime(&tt);
                strftime(str, ICON_STR_LEN, F("📅 %Y-%m-%d", FG_RD), t);
                TRYF(str_fmt, out, "%s%s", pad, str);
            }
        } break;
        case ICON_BUNDLE_STR: {
            TRYF(str_fmt, out, "%s%.*s", pad, STR_F(&icon.str));
        } break;
        case ICON_BUNDLE_NONE: break;
        default: THROW("wrong icon id: %u", icon.id);
    }
    return 0;
error:
    return -1;
}

ErrDecl icons_fmt(Str *out, VIcon *icons) {/*{{{*/
    ASSERT_ARG(out);
    ASSERT_ARG(icons);
    char *pad = "\0 ";
    size_t len = str_length(out);
    //printf("FORMATTING %u icons\n", icons->len);
    for(size_t i = 0; i < (icons->len < ICON_BUNDLE_MAX ? icons->len : ICON_BUNDLE_MAX); ++i) {
        TRYF(icon_fmt, out, pad, icons->items[i]);
        if(*pad != ' ' && str_length(out) != len) ++pad;
    }
    if(str_length(out) == len) {
        TRYF(str_fmt, out, "%s", ICON_LEAF_STR);// "🍃");
    }
    return 0;
error:
    return -1;
}/*}}}*/

#if 0
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
#endif

Icon icon_base(int year, int month, int day, int hour, int minute, int second)
{
    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    time_t tt = mktime(&t);
    return (Icon)tt;
}

