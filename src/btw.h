#ifndef BTW_H

#include "err.h"
#include "vector.h"

typedef struct Nexus Nexus;

#define ERR_btw_parse_file_nofree(n, f, c) "failed parsing file '%.*s'", STR_F(filename)
ErrDecl btw_parse_file_nofree(struct Nexus *nexus, Str *filename, Str *content);


#define BTW_H
#endif

