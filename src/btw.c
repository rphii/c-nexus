#include "nexus.h"
#include "btw.h"
#include "file.h"
#include "cmd.h"

void btw_free(Btw *parse) //{{{
{
    ASSERT_ARG(parse);
    str_free(&parse->content);
    str_free(&parse->basename);
    str_free(&parse->ext);
} //}}}

ErrDecl btw_parse_file_prepare(Nexus *nexus, Str *filename, Btw *parse) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(parse);

    str_clear(&parse->ext);
    str_clear(&parse->content);
    str_clear(&parse->basename);
    TRYF(str_fmt_basename, &parse->basename, filename);
    TRYF(str_fmt_ext, &parse->ext, filename);

    if(str_cmp(&parse->ext, &STR(".btw"))) {
        INFO("incorrect extension '%.*s', parsing '%.*s' anyways", STR_F(&parse->ext), STR_F(filename));
        // TODO make a flag for this?
    }

    TRYF(file_str_read, filename, &parse->content);
    str_trim(&parse->content);

    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_parse_file_nofree(Nexus *nexus, Str *filename, Btw *parse) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(parse);

    TRYF(btw_parse_file_prepare, nexus, filename, parse);
    /* after prepare we have:
     * - raw content
     * - root note title (basename)
     * next steps:
     */

    Node *node = 0;
    TRYF(nexus_insert_node, nexus, &node, &parse->basename, CMD_NONE, &parse->content, ICON_NONE);
    printf("%.*s\n", STR_F(&parse->content));

    return 0;
error:
    return -1;
} //}}}

