#ifndef BTW_H

#include "str.h"
#include "err.h"
#include "vector.h"
#include "lookup.h"

typedef struct BtwParse {
    Str title;
    Str cmd;
    Str description;
} BtwParse;

typedef enum {
    BTW_LEX_NONE,
    BTW_LEX_STRING,
    BTW_LEX_WHITESPACE,
    BTW_LEX_FORMAT,
    BTW_LEX_LINK,
    BTW_LEX_SEPARATOR, // mainly { or }
    /* ids above */
    BTW_LEX__COUNT
} BtwLexList;

#define BTW_FLAG_BOLD       (1U<<0)
#define BTW_FLAG_ITALIC     (1U<<1)
#define BTW_FLAG_UNDERLINE  (1U<<2)
#define BTW_FLAG_NOLINK     (1U<<3)
#define BTW_FLAG_TAG        (1U<<4)

typedef unsigned int BtwFlag;

typedef struct BtwLex {
    //size_t line_i0; // starting index of current line in file
    size_t line_num; // actual line number
    //size_t i0; // starting index of item
    size_t iE; // ending index of item
    //Str str; // snippet of text
    BtwLexList id;
} BtwLex;

typedef struct BtwLink {
    Str str;
    BtwFlag flags;
} BtwLink;

void btwlink_free(BtwLink *link);

typedef struct Btw {
    Str *filename;
    Str ext;
    Str basename;
    Str content;
    VStr dirfiles;
    VBtwLex items;
    //VIcon icons;
#if 0
    VStr links;
    VStr titles;
    VSize indices;
    VSize flags;
#endif
    struct {
        TNode nodes;
        VBtwLink titles;
        VBtwLink refs;
        VSize indices;
    } parse;
    struct {
        size_t direxec;
        size_t attempts;
        size_t success;
        size_t maxres;
        size_t links;
    } stats;
} Btw;

typedef struct Nexus Nexus;

typedef struct BtwExec {
    struct Nexus *nexus;
    Btw *btw;
} BtwExec;

/* color strings
 * bk   black       bk-b    black-bright
 * rd   red         rd-b    red-bright
 * gn   green       gn-b    green-bright
 * yl   yellow      yl-b    yellow-bright
 * bl   blue        bl-b    blue-bright
 * mg   magenta     mg-b    magenta-bright
 * cy   cyan        cy-b    cyan-bright
 * wt   white       wt-b    white-bright
 */

void btwlex_free(BtwLex *lex);

void btw_free(Btw *parse);

bool btw_parse_color(Btw *btw, const Str *str, V3u8 col);

#define ERR_btw_lex(items, str) "failed lexing string"
ErrDecl btw_lex(VBtwLex *btw, Str *str);
#define ERR_btw_parse(nexus, items) "failed parsing"
ErrDecl btw_parse(Nexus *nexus, Btw *btw);

ErrDecl btw_parse_exec(Str *filename, void *args);

#define ERR_btw_file_prepare(nexus, filename, btw) "failed preparing file '%.*s'", STR_F(filename)
ErrDecl btw_file_prepare(Nexus *nexus, Str *filename, Btw *btw);
#define ERR_btw_parse_file(nexus, filename, btw) "failed parsing file '%.*s'", STR_F(filename)
ErrDecl btw_parse_file(struct Nexus *nexus, Str *filename, Btw *btw);

#define BTW_H
#endif

