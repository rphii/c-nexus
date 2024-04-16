#ifndef BTW_H

#include "str.h"
#include "err.h"
#include "vector.h"

typedef struct BtwParse {
    Str title;
    Str cmd;
    Str description;

} BtwParse;

typedef enum {
    BTW_LEX_STRING,
    BTW_LEX_FORMAT,
    BTW_LEX_LINK,
    BTW_LEX_SEPARATOR, // mainly { or }
} BtwLexList;

#define BTW_FLAG_BOLD       (1U<<0)
#define BTW_FLAG_ITALIC     (1U<<1)
#define BTW_FLAG_UNDERLINE  (1U<<2)
#define BTW_FLAG_NOLINK     (1U<<3)

typedef unsigned int BtwFlag;

typedef struct BtwLex {
    BtwLexList id;
    size_t i0; // starting index of current item / end index is start index of next
    size_t line;
    Str str; // snippet of text
    BtwFlag flag;
} BtwLex;

typedef struct Btw {
    Str *filename;
    Str ext;
    Str basename;
    Str content;
    VBtwLex items;
    VStr links;
    VStr titles;
    VSize indices;
} Btw;

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

typedef struct Nexus Nexus;

void btwlex_free(BtwLex *lex);

void btw_free(Btw *parse);

#define btw_lex_ERR(items, str) "failed lexing string"
ErrDecl btw_lex(VBtwLex *btw, Str *str);
#define btw_parse_ERR(nexus, items) "failed parsing"
ErrDecl btw_parse(Nexus *nexus, Btw *btw);

#define btw_file_prepare_ERR(nexus, filename, btw) "failed preparing file '%.*s'", STR_F(filename)
ErrDecl btw_file_prepare(Nexus *nexus, Str *filename, Btw *btw);
#define btw_parse_file_nofree_ERR(nexus, filename, btw) "failed parsing file '%.*s'", STR_F(filename)
ErrDecl btw_parse_file_nofree(struct Nexus *nexus, Str *filename, Btw *btw);


#define BTW_H
#endif

