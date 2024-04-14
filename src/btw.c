#include "nexus.h"
#include "btw.h"

ErrDecl btw_parse_file_nofree(Nexus *nexus, Str *filename, Str *content)
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(content);

    Str ext = {0};
    TRYF(str_fmt_noext, &ext, filename);
    printf("BASENAME: %.*s\n", STR_F(&ext));
    str_free(&ext);

    return 0;
error:
    return -1;
}

