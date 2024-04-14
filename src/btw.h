#ifndef BTW_H

#include "str.h"
#include "err.h"
#include "vector.h"

typedef struct Btw {
    Str basename;
    Str ext;
    Str content;
} Btw;

typedef struct Nexus Nexus;

void btw_free(Btw *parse);

#define btw_parse_file_prepare_ERR(nexus, filename, parse) "failed preparing file '%.*s'", STR_F(filename)
ErrDecl btw_parse_file_prepare(Nexus *nexus, Str *filename, Btw *parse);
#define btw_parse_file_nofree_ERR(n, f, c) "failed parsing file '%.*s'", STR_F(filename)
ErrDecl btw_parse_file_nofree(struct Nexus *nexus, Str *filename, Btw *parse);


#define BTW_H
#endif

