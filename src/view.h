#ifndef VIEW_H

#include "str.h"
#include "vector.h"
#include "node.h"
#include "err.h"

struct Nexus;

typedef enum {
    VIEW_NONE,
    /* views below */
    VIEW_NORMAL,
    VIEW_SEARCH,
    VIEW_ICON,
} ViewList;

typedef struct View {
    ViewList id;
    Node *current;  /* normal */
    Str search;     /* search */
    bool edit;      /* serach */
    size_t sub_sel; /* normal & search */
} View;

void view_free(View *view);

#define ERR_VIEW_FMT "failed formatting view"
ErrDecl view_fmt(struct Nexus *nexus, Str *out, View *view);

#define ERR_VIEW_COPY "failed copying view"
ErrDecl view_copy(View *dst, View *src);

#define VIEW_H
#endif

