#ifndef BTW_H

#include "err.h"
#include "vector.h"

typedef struct Nexus Nexus;

ErrDecl btw_parse_files(struct Nexus *nexus, VsStr *files);
ErrDecl btw_parse_file_nofree(struct Nexus *nexus, Str *filename, Str *content);

#define BTW_H
#endif

