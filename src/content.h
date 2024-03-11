#ifndef CONTENT_H

#include "err.h"
typedef struct Nexus Nexus;

#define ERR_CONTENT_BUILD "failed building content"
ErrDecl content_build(Nexus *nexus, Node *root);

#define CONTENT_H
#endif

