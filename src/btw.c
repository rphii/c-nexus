#include "nexus.h"
#include "btw.h"
#include "file.h"
#include "cmd.h"
#include "str.h"

void btwlex_free(BtwLex *lex) { //{{{
    ASSERT_ARG(lex);
    str_free(&lex->str);
    memset(lex, 0, sizeof(*lex));
} //}}}

#define btw_lex_append_ERR(items, item) "failed appending lex item"
ErrDecl btw_lex_append(VBtwLex *items, BtwLex *item) { //{{{
    ASSERT_ARG(items);
    ASSERT_ARG(item);
#if 1
    if(item->flag) printf(F("F", BG_WT_B FG_BK));
    if(item->id == BTW_LEX_STRING && !str_length(&item->str)) {
        return 0;
    }
    if(item->id == BTW_LEX_STRING) {
        printf(F("[", FG_BK_B) "%.*s" F("]", FG_BK_B), STR_F(&item->str));
    }
    if(item->id == BTW_LEX_LINK) {
        printf(F("'%.*s'", FG_CY_B), STR_F(&item->str));
    }
    if(item->id == BTW_LEX_FORMAT) {
        printf(F("%.*s", FG_RD), STR_F(&item->str));
    }
#endif
    TRY(vbtwlex_push_back(items, item), ERR_VEC_PUSH_BACK);
    memset(item, 0, sizeof(*item));
    //printf(F("APPEND:%u:%.*s\n", FG_BK_B), item->id, STR_F(&item->str));
    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_lex_str(VBtwLex *items, Str *str) { //{{{
    ASSERT_ARG(items);
    ASSERT_ARG(str);
    int err = 0;
    size_t index = 0, line_index = 0;
    BtwLex temp = {0};
    temp.id = BTW_LEX_STRING;
    Str line = {0};
    while(index < str_length(str)) {
        str_clear(&line);
        TRYF(str_fmt_line, &line, str, &index);
        str_trim(&line);
        /* go over (trimmed) lines */
        /////printf("%4zu | %.*s", ++line_index, STR_F(&line));

#if 1
        /* check format+link */
        /////printf(" => ");
        bool format_link = false;
        do {
            format_link = false;
            size_t f0 = str_ch(&line, '#', 0);
            size_t f1 = str_ch(&line, '[', 0);
            if(f1 < str_length(&line)) {
                size_t f3 = str_ch_pair(&STR_I0(line, f1), ']') + f1;
                size_t f2 = str_irch(&line, f3, '#', 0);
                bool f0b = false;
                bool f2b = false;
                if(f3 < str_length(&line)) {
                    /* found [ and matching ] */
                    format_link = true;
                    if(f0 < f1) {
                        size_t ws = str_find_ws(&STR_I0(line, f0)) + f0;
                        if(ws > f1) {
                            /* valid #color[ found - but first, append previous text */
                            /////printf(F("#", BG_RD));
                            f0b = true;
                        }
                    }
                    if(f0b) {
                        size_t until = f0 < f1 ? f0 : f1;
                        temp.id = BTW_LEX_STRING;
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(until), str_iter_begin(&line));
                        TRYF(btw_lex_append, items, &temp);
                        /* append actual color */
                        temp.id = BTW_LEX_FORMAT;
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(f1-f0+1), str_iter_begin(&STR_I0(line, f0)));
                        TRYF(btw_lex_append, items, &temp);
                    } else {
                        size_t until = f1;
                        temp.id = BTW_LEX_STRING;
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(until), str_iter_begin(&line));
                        TRYF(btw_lex_append, items, &temp);
                    }
                    /////printf(F("[", FG_BK BG_GN));
                    //printf("{f0 %zu, f2 %zu, f3 %zu}", f0, f2, f3);
                    if(f2 > f1 && f2 < f3) {
                        size_t ws = str_find_ws(&STR_I0(line, f2)) + f2;
                        if(ws > f3) {
                            f2b = true;
                        }
                    }
                    if(f2b == true) {
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(f2-f1-1), str_iter_begin(&STR_I0(line, f1+1)));
                        /////printf("%.*s", (int)(f2-f1-1), str_iter_begin(&STR_I0(line, f1+1)));
                        /////printf(F("#", BG_BL));
                    } else {
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(f3-f1-1), str_iter_begin(&STR_I0(line, f1+1)));
                        /////printf("%.*s", (int)(f3-f1-1), str_iter_begin(&STR_I0(line, f1+1)));
                    }
                    /////printf(F("]", FG_BK BG_YL));
                    size_t fin11 = str_find_ws(&STR_I0(line, f3)) + f3;
                    size_t fin12 = str_find_any(&STR_I0(line, f3), &STR("[#")) + f3;
                    size_t fin1 = fin11 < fin12 ? fin11 : fin12;
                    size_t fin2 = str_ch(&STR_I0(line, f3), '|', 0) + f3;
                    //size_t fin3 = str_ch(&STR_I0(line, f3), ']', 0) + f3;
                    size_t done = fin1 < fin2 ? fin1 : fin2;
                    size_t bold = str_ch(&STR_I0(line, f3), 'b', 0) + f3;
                    size_t ital = str_ch(&STR_I0(line, f3), 'i', 0) + f3;
                    size_t undl = str_ch(&STR_I0(line, f3), 'u', 0) + f3;
                    size_t nlnk = str_ch(&STR_I0(line, f3), '!', 0) + f3;
                    // TODO: fix this shit // the mess above :)
                    //printf("\n[[[fin11:%zu fin12:%zu fin1:%zu fin2:%zu done%zu b%zu i%zu u%zu !%zu]]]\n", fin11, fin12, fin1, fin2, done, bold, ital, undl, nlnk);
                    if(bold < done) temp.flag |= BTW_FLAG_BOLD;
                    if(ital < done) temp.flag |= BTW_FLAG_ITALIC;
                    if(undl < done) temp.flag |= BTW_FLAG_UNDERLINE;
                    if(nlnk < done) temp.flag |= BTW_FLAG_NOLINK;
                    /////printf("%.*s", (int)(done-f3-1), str_iter_begin(&STR_I0(line, f3+1)));
                    temp.id = BTW_LEX_LINK;
                    TRYF(btw_lex_append, items, &temp);
                    /* idk man */
                    if(f2b == true) {
                        temp.id = BTW_LEX_FORMAT;
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(f3-f2+1), str_iter_begin(&STR_I0(line, f2)));
                        TRYF(btw_lex_append, items, &temp);
                    }
                    line.first += done + (fin2 == done);
                    if(line.first > line.last) line.first = line.last;
                    if(!str_length(&line)) break;
                }
            }
            if(!format_link) {
                TRYF(str_fmt, &temp.str, "%.*s", STR_F(&line));
            }
            //printf("LINE:%.*s\n", STR_F(&line));
        } while(format_link);
        TRYF(str_fmt, &temp.str, "\n");
#endif
        /////printf("\n");
    }
    TRYF(btw_lex_append, items, &temp);
clean:
    //printf("%.*s\n", STR_F(&temp.str));
    btwlex_free(&temp);
    str_free(&line);
    return err;
error:
    ERR_CLEAN;
} //}}}

void btw_free(Btw *btw) { //{{{
    ASSERT_ARG(btw);
    str_free(&btw->content);
    str_free(&btw->basename);
    str_free(&btw->ext);
} //}}}

ErrDecl btw_parse_file_prepare(Nexus *nexus, Str *filename, Btw *btw) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(btw);

    str_clear(&btw->ext);
    str_clear(&btw->content);
    str_clear(&btw->basename);
    TRYF(str_fmt_basename, &btw->basename, filename);
    TRYF(str_fmt_ext, &btw->ext, filename);

    if(str_cmp(&btw->ext, &STR(".btw"))) {
        // TODO make a flag for this?
        INFO("incorrect extension '%.*s', parsing '%.*s' anyways", STR_F(&btw->ext), STR_F(filename));
    }

    TRYF(file_str_read, filename, &btw->content);
    str_trim(&btw->content);

    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_parse_file_nofree(Nexus *nexus, Str *filename, Btw *btw) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(btw);

    VBtwLex items = {0};

    TRYF(btw_parse_file_prepare, nexus, filename, btw);
    TRYF(btw_lex_str, &items, &btw->content);
    /* after prepare we have:
     * - raw content
     * - root note title (basename)
     * next steps:
     * - parse sub notes
     */

    //Node *node = 0;
    //TRYF(nexus_insert_node, nexus, &node, &btw->basename, CMD_NONE, &btw->content, ICON_NONE);
    //printf("%.*s\n", STR_F(&btw->content));

    vbtwlex_free(&items);
    return 0;
error:
    return -1;
} //}}}

