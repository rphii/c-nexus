#ifndef INFO_H

#include <stdbool.h>
#include "err.h"

typedef enum {
    INFO_NONE, /* ids below */

    INFO_syscmd_failed,
    INFO_directory,
    INFO_skipping_nofile_nodir,
    // parsing stufv
    INFO_parsing_file,
    INFO_parsing_skip_incorrect_extension,
    INFO_parsing_found_note,
    INFO_parsing_found_link,
    INFO_parsing_found_format,
    // nexus stuff
    INFO_nexus_rebuild_failed,
    INFO_nexus_init,
    INFO_nexus_title_node_not_found,
    INFO_nexus_stats,

    INFO__COUNT /* ids above */
} InfoList;

typedef enum {
    INFO_LEVEL_TEXT         = 0x01,
    INFO_LEVEL_IS_INFO      = 0x02,
    INFO_LEVEL_FILE_LINE    = 0x04,
    INFO_LEVEL_FUNCTION     = 0x08,
    INFO_LEVEL_ID           = 0x10,
    /* handy stuff */
    INFO_LEVEL_ALL          = 0xFFFF
} InfoLevelField;

typedef struct Info {
    InfoLevelField disabled[INFO__COUNT];
} Info;

/* The # operator converts symbol 'v' into a string */
#define STRINGIFY0(v) #v
#define STRINGIFY(v) STRINGIFY0(v)

#define info(id, str, ...)  do {\
        InfoLevelField disabled = info_query_disabled(INFO_##id); \
        if(disabled & INFO_LEVEL_TEXT) break; /* like.. if no text -> break entirely */ \
        bool decorators = false; \
        if(~disabled & INFO_LEVEL_IS_INFO) { \
            ERR_PRINTF(F("[INFO] ", FG_YL_B BOLD)); \
        } \
        if(~disabled & INFO_LEVEL_ID) { \
            ERR_PRINTF(F("<%s> ", FG_BL_B BOLD), STRINGIFY(id)); \
        } \
        if(~disabled & INFO_LEVEL_FILE_LINE) { \
            ERR_PRINTF(F("%s%s:%i", FG_WT_B), decorators ? ":" : "", __FILE__, __LINE__); \
            decorators = true; \
        } \
        if(~disabled & INFO_LEVEL_FUNCTION) { \
            ERR_PRINTF(F("%s%s", FG_WT_B), decorators ? ":" : "", __func__); \
            decorators = true; \
        } \
        ERR_PRINTF("%s", decorators ? " " : ""); \
        decorators = false; \
        ERR_PRINTF(str, ##__VA_ARGS__); \
        ERR_PRINTF("\n"); \
    } while(0)

InfoLevelField info_query_disabled(InfoList id);
void info_disable(InfoList id, InfoLevelField field);
void info_disable_all(InfoLevelField field);

void info_enable(InfoList id, InfoLevelField field);

#define INFO_H
#endif

