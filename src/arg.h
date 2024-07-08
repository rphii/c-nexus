#ifndef ARG_H

#include <stdbool.h>

#include "err.h"
#include "str.h"
#include "vector.h"

struct Nexus;

/* specify {{{ */

typedef enum {
    SPECIFY_NONE,
    /* below */
    SPECIFY_OPTIONAL,
    SPECIFY_OPTION,
        SPECIFY_YES,
        SPECIFY_TRUE,
        SPECIFY_NO,
        SPECIFY_FALSE,
        SPECIFY_NORMAL,
        SPECIFY_SEARCH_ALL,
        SPECIFY_SEARCH_SUB,
        SPECIFY_ICON,
    SPECIFY_NUMBER,
    SPECIFY_STRING,
    SPECIFY_LIST,
    SPECIFY_BOOL,
    /* certain default values */
    SPECIFY_EXTENSION,
    SPECIFY_MAX_FILE_SIZE,
    /* above */
    SPECIFY__COUNT
} SpecifyList;

typedef struct Specify {
    size_t len;
    SpecifyList *ids;
} Specify;

#define SPECIFY(...)  (Specify){ \
    .ids = (SpecifyList []){__VA_ARGS__}, \
    .len = sizeof((SpecifyList []){__VA_ARGS__}) / sizeof(SpecifyList)}

/* }}} */

/* arguments {{{ */

typedef enum {
    ARG_NONE,
    /* args below */
    ARG_HELP,
    ARG_VERSION,
    ARG_ENTRY,
    ARG_VIEW,
    ARG_SHOW_DESCRIPTION,
    ARG_SHOW_PREVIEW,
    ARG_MAX_LIST,
    ARG_EXTENSIONS,
    ARG_MAX_FILE_SIZE,
    /* args above */
    ARG__COUNT
} ArgList;

typedef struct Arg {
    const char *name;
    Str unknown;
    bool exit_early;
    SpecifyList view; // TODO: move into sub-struct
    SpecifyList show_description; // TODO: move into sub-struct
    SpecifyList show_preview; // TODO: move into sub-struct
    size_t max_list; // TODO: move into sub-struct
    size_t max_file_size; // TODO: move into sub-struct
    Str entry; // TODO: move into sub-struct
    Str extensions; // TODO: move into sub-struct
    VsStr files; // TODO: move into sub-struct (MAYBE???)
    struct {
        int tiny; /* tiny, because short is reserved */
        int main;
        int ext; /* abbr. for extended, because long is reserved  */
        int spec; /* specification tabs */
        int max;
    } tabs;
} Arg;

/* }}} */

#define LINK_GITHUB "https://github.com/rphii/c-nexus"

#define ERR_ARG_PARSE "failed parsing arguments"
ErrDecl arg_parse(Arg *arg, int argc, const char **argv);

const char *arg_str(ArgList id);
const char *specify_str(SpecifyList id);

void arg_help(Arg *arg);
void arg_free(Arg *arg);

#define ARG_H
#endif

