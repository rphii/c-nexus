#include "nexus.h"
#include "btw.h"
#include "file.h"
#include "cmd.h"
#include "str.h"
#include "vector.h"

void btwlex_free(BtwLex *lex) { //{{{
    ASSERT_ARG(lex);
    str_free(&lex->str);
    memset(lex, 0, sizeof(*lex));
} //}}}

#define btw_lex_append_ERR(items, item, i0, line_index) "failed appending lex item"
ErrDecl btw_lex_append(VBtwLex *items, BtwLex *item, size_t i0, size_t line_index) { //{{{
    ASSERT_ARG(items);
    ASSERT_ARG(item);
    // TODO: if link, to a trim? or somewhere else?
    if(item->id == BTW_LEX_STRING && !str_length(&item->str)) {
        return 0;
    }
#if 1
    if(item->flag) {
        printf(F("F", BG_WT_B FG_BK));
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
    if(item->id == BTW_LEX_SEPARATOR) {
        printf(F("%.*s", FG_BL_B), STR_F(&item->str));
    }
#endif
    item->line = line_index;
    item->i0 = i0;
    TRY(vbtwlex_push_back(items, item), ERR_VEC_PUSH_BACK);
    memset(item, 0, sizeof(*item));
    //printf(F("APPEND:%u:%.*s\n", FG_BK_B), item->id, STR_F(&item->str));
    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_lex(VBtwLex *items, Str *str) { //{{{
    ASSERT_ARG(items);
    ASSERT_ARG(str);
    int err = 0;
    size_t index = 0, line_index = 0, i0 = 0;
    BtwLex temp = {0};
    temp.id = BTW_LEX_STRING;
    Str line = {0};
    while(index < str_length(str)) {
        i0 = index;
        str_clear(&line);
        ++line_index;
        TRYF(str_fmt_line, &line, str, &index);
        str_trim(&line);
        /* go over (trimmed) lines */
        /////printf("%4zu | %.*s", ++line_index, STR_F(&line));

#if 1
        /* check format+link */
        /////printf(" => ");
        bool handled = false;
        do {
            handled = false;
            size_t sep0 = str_find_any(&line, &STR("{}"));
            size_t f0 = str_ch(&line, '#', 0);
            size_t f1 = str_ch(&line, '[', 0);
            //printf("\n[[sep0 %zu  f0 %zu  f1 %zu:%.*s]]\n", sep0, f0, f1, STR_F(&line));
            if(f1 < str_length(&line) && (f1 < sep0 || f0 < sep0)) {
                //printf(" => link\n");
                size_t f3 = str_ch_pair(&STR_I0(line, f1), ']') + f1;
                if(f3 >= str_length(&line)) {
                    f3 = str_rch(&line, ']', 0);
                }
                size_t f2 = str_irch(&line, f3, '#', 0);
                bool f0b = false;
                bool f2b = false;
                if(f3 < str_length(&line)) {
                    /* found [ and matching ] */
                    handled = true;
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
                        TRYF(btw_lex_append, items, &temp, i0, line_index);
                        /* append actual color */
                        temp.id = BTW_LEX_FORMAT;
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(f1-f0+1), str_iter_begin(&STR_I0(line, f0)));
                        TRYF(btw_lex_append, items, &temp, i0, line_index);
                    } else {
                        size_t until = f1;
                        temp.id = BTW_LEX_STRING;
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(until), str_iter_begin(&line));
                        TRYF(btw_lex_append, items, &temp, i0, line_index);
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
                    size_t fin12 = str_find_any(&STR_I0(line, f3), &STR("[#{}")) + f3; // TODO: this is shit.. make it less shit. e.g. [a]] -> the ] gets swallowed whole, but we can't put it into here because (again) this is shit
                    size_t fin1 = fin11 < fin12 ? fin11 : fin12;
                    size_t fin2 = str_ch(&STR_I0(line, f3), '|', 0) + f3;
                    //size_t fin3 = str_ch(&STR_I0(line, f3), ']', 0) + f3;
                    size_t done = fin1 < fin2 ? fin1 : fin2;
                    size_t bold = str_ch(&STR_I0(line, f3), 'b', 0) + f3;
                    size_t ital = str_ch(&STR_I0(line, f3), 'i', 0) + f3;
                    size_t undl = str_ch(&STR_I0(line, f3), 'u', 0) + f3;
                    size_t nlnk = str_ch(&STR_I0(line, f3), '!', 0) + f3;
                    // TODO: fix this shit // the mess above :)
                    // just put it into format->string?
                    //printf("\n[[[fin11:%zu fin12:%zu fin1:%zu fin2:%zu done%zu b%zu i%zu u%zu !%zu]]]\n", fin11, fin12, fin1, fin2, done, bold, ital, undl, nlnk);
                    if(bold < done) temp.flag |= BTW_FLAG_BOLD;
                    if(ital < done) temp.flag |= BTW_FLAG_ITALIC;
                    if(undl < done) temp.flag |= BTW_FLAG_UNDERLINE;
                    if(nlnk < done) temp.flag |= BTW_FLAG_NOLINK;
                    /////printf("%.*s", (int)(done-f3-1), str_iter_begin(&STR_I0(line, f3+1)));
                    temp.id = BTW_LEX_LINK;
                    TRYF(btw_lex_append, items, &temp, i0, line_index);
                    /* idk man */
                    if(f2b == true) {
                        temp.id = BTW_LEX_FORMAT;
                        TRYF(str_fmt, &temp.str, "%.*s", (int)(f3-f2+1), str_iter_begin(&STR_I0(line, f2)));
                        TRYF(btw_lex_append, items, &temp, i0, line_index);
                    }
                    line.first += done + (fin2 == done);
                    if(line.first > line.last) line.first = line.last;
                    if(!str_length(&line)) break;
                }
            } else if(sep0 < str_length(&line)) {
                //printf(" => sep\n");
                /* push back previously found string */
                handled = true;
                size_t until = sep0;
                temp.id = BTW_LEX_STRING;
                TRYF(str_fmt, &temp.str, "%.*s", (int)(until), str_iter_begin(&line));
                TRYF(btw_lex_append, items, &temp, i0, line_index);
                /* push back current separators */
                temp.id = BTW_LEX_SEPARATOR;
                TRYF(str_fmt, &temp.str, "%.*s", 1, str_iter_begin(&STR_I0(line, sep0)));
                TRYF(btw_lex_append, items, &temp, i0, line_index);
                line.first += until + 1;
            }
            if(!handled) {
                TRYF(str_fmt, &temp.str, "%.*s", STR_F(&line));
            }
            //printf("LINE:%.*s\n", STR_F(&line));
        } while(handled);
        TRYF(str_fmt, &temp.str, "\n");
#endif
        /////printf("\n");
    }
    TRYF(btw_lex_append, items, &temp, i0, line_index);
clean:
    //printf("%.*s\n", STR_F(&temp.str));
    btwlex_free(&temp);
    str_free(&line);
    return err;
error:
    ERR_CLEAN;
} //}}}

size_t btw_parse_match_pattern(VBtwLex *items, size_t i0, size_t n_pat, BtwLexList **pat)
{
    ASSERT_ARG(items);
    ASSERT_ARG(pat);
    size_t pat_longest = n_pat;
    for(size_t pat_index = 0; pat_index < n_pat; ++pat_index) {
        size_t len_pat = pat[pat_index][0];
        //printf("PATTERN %zu/%zu (%zu)\n", pat_index, n_pat, len_pat);
        for(size_t i = 0; i < len_pat; ++i) {
            size_t ii = i + i0;
            if(ii >= vbtwlex_length(items)) goto next;
            BtwLex *item = vbtwlex_get_at(items, ii);
            //printf("  %u == %u ?\n", item->id, pat[pat_index][i+1]);
            if(item->id != pat[pat_index][i+1]) goto next;
        }
        if((pat_longest < n_pat && len_pat > pat[pat_longest][0]) || pat_longest >= n_pat) {
            pat_longest = pat_index;
        }
next:;
     //printf(" => pat_longest %zu\n", pat_longest);
    }
    return pat_longest;
}

size_t btw_parse_is_link(VBtwLex *items, size_t i0) { //{{{
    ASSERT_ARG(items);
    BtwLexList *pat[] = {
        (BtwLexList []){3, BTW_LEX_FORMAT, BTW_LEX_LINK, BTW_LEX_FORMAT},  // #[#]
        (BtwLexList []){2, BTW_LEX_LINK, BTW_LEX_FORMAT},                  //  [#]
        (BtwLexList []){2, BTW_LEX_FORMAT, BTW_LEX_LINK},                  // #[ ]
        (BtwLexList []){1, BTW_LEX_LINK},                                  //  [ ]
    };
    size_t n_pat = sizeof(pat)/sizeof(*pat);
    size_t i_pat = btw_parse_match_pattern(items, i0, n_pat, pat);
    //printf("i_pat %zu/%zu\n", i_pat, n_pat);
    return (i_pat < n_pat) ? pat[i_pat][0] : 0;
} //}}}

size_t btw_parse_is_ws(VBtwLex *items, size_t i0, size_t *n_newline) {/*{{{*/
    ASSERT_ARG(items);
    ASSERT_ARG(n_newline);
    if(i0 >= vbtwlex_length(items)) return 0;
    BtwLex *item = vbtwlex_get_at(items, i0);
    if(item->id == BTW_LEX_STRING) {
        size_t nws = str_find_nws(&item->str);
        if(nws >= str_length(&item->str)) {
            *n_newline = str_count_ch(&item->str, '\n');
            return 1;
        }
    }
    return 0;
}/*}}}*/

#define btw_parse_is_scope_ERR(items, i0, len, ref) "failed confirming scope"
ErrDecl btw_parse_is_scope(VBtwLex *items, size_t i0, size_t *len, Btw *btw) {/*{{{*/
    ASSERT_ARG(items);
    ASSERT_ARG(len);
    ASSERT_ARG(btw);
    if(i0 >= vbtwlex_length(items)) return 0;
    /* error stuff */
    bool err_scope = false;
    Str hint = {0};
    size_t line_i0 = 0;
    size_t line = -1;
    /* non-error stuff */
    size_t index = i0;
    int level = 0;
    do {
        BtwLex *item = vbtwlex_get_at(items, index);
        if(index == i0) {
            line = item->line;
            line_i0 = item->i0;
        }
        if(item->id == BTW_LEX_SEPARATOR) {
            for(size_t i = 0; i < str_length(&item->str); ++i) {
                char c = str_get_at(&item->str, i);
                if(c == '{') ++level;
                if(c == '}') --level;
            }
        }
        ++index;
    } while(level > 0 && index < vbtwlex_length(items));
    if(level > 0) { err_scope = true; THROW("brackets { or } mismatch of %i levels on line %zu:", level, line); }
    *len = index;
    return 0;
error:
    if(err_scope) {
        (void)str_fmt_line(&hint, &btw->content, &line_i0);
        printf(" %.*s:" F("%zu", FG_WT_B) " | %.*s\n", STR_F(btw->filename), line, STR_F(&hint));
        str_free(&hint);
    }
    return -1;
}/*}}}*/

size_t btw_parse_is_note(VBtwLex *items, size_t i0) {/*{{{*/
    ASSERT_ARG(items);
    size_t index = i0;
    size_t link = btw_parse_is_link(items, index);
    if(link) {
        index += link;
        size_t n_newline = 0;
        size_t ws = btw_parse_is_ws(items, index, &n_newline);
        index += ws;
    }
    return 0;
}/*}}}*/

ErrDecl btw_parse(Nexus *nexus, Btw *btw) { //{{{
    ASSERT_ARG(nexus);
    ASSERT_ARG(btw);
    int err = 0;
    VrStr links = {0};
    VrStr titles = {0};
    VBtwLex *items = &btw->items;
    //printf("len: %zu\n", vbtwlex_length(&btw->items));
    TRY(vrstr_push_back(&titles, &btw->basename), ERR_VEC_PUSH_BACK);
    size_t index = 0;
    while(index < vbtwlex_length(items)) {
        size_t link = btw_parse_is_link(items, index);
        if(link) {
            size_t i0 = index;
            size_t n_newline = 0;
            index += link;
            //printf("link %zu : %zu\n", index, link);
            size_t ws = btw_parse_is_ws(items, index, &n_newline);
            //printf("ws %zu : %zu\n", index, link);
            index += ws;
            if(n_newline <= 1) {
                size_t scope = 0;
                TRYF(btw_parse_is_scope, items, index, &scope, btw);
                if(scope) {
                    printf("scope %zu->%zu\n", i0, scope);
                }
            }
        }
        else {
            ++index;
        }
        //size_t ws = btw_parse_is_ws(items, index+link);
        //printf("link %zu, ws %zu\n", link, ws);
    }
    //for(size_t i = 0; i < vbtwlex_length(items); ++i) {
    //    BtwLex *item = vbtwlex_get_at(items, i);
    //    Str *title = vrstr_get_back(&titles);
    //}
clean:
    vrstr_free(&titles);
    vrstr_free(&links);
    return err;
error:
    ERR_CLEAN;
} //}}}

void btw_free(Btw *btw) { //{{{
    ASSERT_ARG(btw);
    str_free(&btw->content);
    str_free(&btw->basename);
    str_free(&btw->ext);
    vbtwlex_free(&btw->items);
} //}}}

ErrDecl btw_file_prepare(Nexus *nexus, Str *filename, Btw *btw) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(btw);

    btw->filename = filename;
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

    TRYF(btw_file_prepare, nexus, filename, btw);
    TRYF(btw_lex, &btw->items, &btw->content);
    TRYF(btw_parse, nexus, btw);
    /* after prepare we have:
     * - raw content
     * - root note title (basename)
     * next steps:
     * - parse sub notes
     */

    //Node *node = 0;
    //TRYF(nexus_insert_node, nexus, &node, &btw->basename, CMD_NONE, &btw->content, ICON_NONE);
    //printf("%.*s\n", STR_F(&btw->content));
    return 0;
error:
    return -1;
} //}}}

