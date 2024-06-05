#include "info.h"
#include "str.h"
#include <stdarg.h>

static Info s_info; /* I hate public variables ... */

InfoLevelField info_query_disabled(InfoList id) {
    InfoLevelField result = {0};
    if(id < INFO__COUNT) {
        result = s_info.disabled[id];
    }
    return result;
}

void info_disable(InfoList id, InfoLevelField field) {
    if(id < INFO__COUNT) {
        s_info.disabled[id] |= field;
    }
}

void info_disable_all(InfoLevelField field) {
    for(InfoList id = 0; id < INFO__COUNT; ++id) {
        s_info.disabled[id] |= field;
    }
}

void info_enable(InfoList id, InfoLevelField field) {
    if(id < INFO__COUNT) {
        s_info.disabled[id] &= ~field;
    }
}


