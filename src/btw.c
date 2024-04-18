#include "lookup.h"
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
    if(item->id == BTW_LEX_STRING) {
        //str_trim(&item->str);
        if(!str_length(&item->str)) {
            return 0;
        }
    }
    if(item->id == BTW_LEX_LINK) {
        str_trim(&item->str);
        //if(!str_length(&item->str)) {
        //    return 0;
        //}
    }
#if 0
    printf(F("%zu", BG_BK_B), vbtwlex_length(items));
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
            size_t sep0 = str_find_any(&line, &STR("{}|"));
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
                    size_t fin12 = str_find_any(&STR_I0(line, f3), &STR("[#{}|")) + f3; // TODO: this is shit.. make it less shit. e.g. [a]] -> the ] gets swallowed whole, but we can't put it into here because (again) this is shit
                    size_t fin1 = fin11 < fin12 ? fin11 : fin12;
                    //   size_t fin2 = str_ch(&STR_I0(line, f3), '|', 0) + f3;
                    //size_t fin3 = str_ch(&STR_I0(line, f3), ']', 0) + f3;
                    size_t done = fin1; // < fin2 ? fin1 : fin2;
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
                    line.first += done; // + (fin2 == done);
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

size_t btw_parse_match_pattern(VBtwLex *items, size_t i0, size_t n_pat, const BtwLexList **pat)
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

static const BtwLexList *static_btw_pat_link[] = {
    (BtwLexList []){3, BTW_LEX_FORMAT, BTW_LEX_LINK, BTW_LEX_FORMAT},  // #[#]
    (BtwLexList []){2, BTW_LEX_FORMAT, BTW_LEX_LINK},                  // #[ ]
    (BtwLexList []){2, BTW_LEX_LINK, BTW_LEX_FORMAT},                  //  [#]
    (BtwLexList []){1, BTW_LEX_LINK},                                  //  [ ]
};

size_t btw_parse_is_link(VBtwLex *items, size_t i0) { //{{{
    ASSERT_ARG(items);
#if 0
    BtwLexList *pat[] = {
        (BtwLexList []){3, BTW_LEX_FORMAT, BTW_LEX_LINK, BTW_LEX_FORMAT},  // #[#]
        (BtwLexList []){2, BTW_LEX_LINK, BTW_LEX_FORMAT},                  //  [#]
        (BtwLexList []){2, BTW_LEX_FORMAT, BTW_LEX_LINK},                  // #[ ]
        (BtwLexList []){1, BTW_LEX_LINK},                                  //  [ ]
    };
#endif
    size_t n_pat = sizeof(static_btw_pat_link)/sizeof(*static_btw_pat_link);
    size_t i_pat = btw_parse_match_pattern(items, i0, n_pat, static_btw_pat_link);
    size_t len_pat = (i_pat < n_pat) ? static_btw_pat_link[i_pat][0] : 0;
#if 0
    if(i0 + len_pat < vbtwlex_length(items)) {
        BtwLex *item = vbtwlex_get_at(items, i0 + len_pat);
        if(item->id == BTW_LEX_SEPARATOR && !str_cmp(&item->str, &STR("|"))) {
            len_pat++;
        }
    }
#endif
    //printf("i_pat %zu/%zu\n", i_pat, n_pat);
    return len_pat;
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
    bool valid = false;
    do {
        if(index >= vbtwlex_length(items)) return 0;
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
        if(level > 0) valid = true;
    } while(level > 0 && index < vbtwlex_length(items));
    if(level > 0) { err_scope = true; THROW("brackets { or } mismatch of %i levels on line %zu:", level, line); }
    if(level == 0 && valid) {
        //printf("  ..scope len %zu (%zu-%zu)\n", index-i0, index, i0);
        *len = (index - i0);
    }
    return 0;
error:
    if(err_scope) {
        (void)str_fmt_line(&hint, &btw->content, &line_i0);
        printf(" %.*s:" F("%zu", FG_WT_B) " | %.*s\n", STR_F(btw->filename), line, STR_F(&hint));
        str_free(&hint);
    }
    return -1;
}/*}}}*/

#define btw_parse_is_note_ERR(items, i0, note, btw) "could not confirm scope"
ErrDecl btw_parse_is_note(VBtwLex *items, size_t i0, size_t *len, Btw *btw) {/*{{{*/
    ASSERT_ARG(items);
    ASSERT_ARG(len);
    ASSERT_ARG(btw);
    // DONE/TODO: fix [b] [c] [a] { => b an c get "ignored"; only a is counted to the scope
    if(i0 >= vbtwlex_length(items)) return 0;
    size_t index = i0;
    // TODO icons not handled!!!!!!!! (they get ignored)
    size_t link = btw_parse_is_link(items, index);
    if(link) {
next:
        //printf("  ..link %zu\n", index);
        index += link;
        size_t n_newline = 0;
        size_t ws = btw_parse_is_ws(items, index, &n_newline);
        if(n_newline <= 1) {
            index += ws;
            link = btw_parse_is_link(items, index);
            if(link) goto next; // not too beautiful.. but it.. works? TODO maybe get rid of this goto???
            size_t scope = 0;
            TRYF(btw_parse_is_scope, items, index, &scope, btw);
            if(scope) {
                index += scope;
                *len = (index - i0);
            }
        }
    }
    return 0;
error:
    return -1;
}/*}}}*/

#define btw_parse_link_ERR(btw, i0, formatted, iE) "failed parsing link"
ErrDecl btw_parse_link(Btw *btw, size_t i0, Str *pending, size_t *len)
{
    ASSERT_ARG(btw);
    ASSERT_ARG(pending);
    ASSERT_ARG(len);
    if(i0 >= vbtwlex_length(&btw->items)) return 0;
    size_t n_pat = sizeof(static_btw_pat_link)/sizeof(*static_btw_pat_link);
    size_t i_pat = btw_parse_match_pattern(&btw->items, i0, n_pat, static_btw_pat_link);
    BtwLex *item = 0;
    Str copy = {0};
    if(i_pat < n_pat) {
        if(i_pat == 0 || i_pat == 1) {
            item = vbtwlex_get_at(&btw->items, i0+1);
        } else if(i_pat == 2 || i_pat == 3) {
            item = vbtwlex_get_at(&btw->items, i0+0);
        }
        if(item) {
            /* properly format the string (TODO) */
            Str *p = &item->str;
            if(str_length(p)) {
                TRYF(str_fmt, pending, F("%.*s", FG_YL_B), STR_F(p));
                if(!(item->flag & BTW_FLAG_NOLINK)) {
                    //printf("  LINK: %.*s\n", STR_F(p));
                    TRYF(str_copy, &copy, pending);
                    TRY(vstr_push_back(&btw->links, &copy), ERR_VEC_PUSH_BACK);
                }
            }
        }
        size_t delta = static_btw_pat_link[i_pat][0];
        /* filter separator */
        if(i0 + delta < vbtwlex_length(&btw->items)) {
            BtwLex *item = vbtwlex_get_at(&btw->items, i0 + delta);
            if(item->id == BTW_LEX_SEPARATOR && !str_cmp(&item->str, &STR("|"))) {
                ++delta;
            }
        }
        //printf("   DELTA %zu\n", delta);
        /* transfer length to end index */
        *len = delta; // TODO: I hate this I HATE THIS... why need do minus one???
    }
    return 0;
error:
    return -1;
}

#define btw_parse_note_ERR(btw, i0, pending, iE) "failed parsing note"
ErrDecl btw_parse_note(Btw *btw, size_t i0, Str *pending, size_t *len) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(pending);
    ASSERT_ARG(len);
    if(i0 >= vbtwlex_length(&btw->items)) return 0;

    size_t link_len = 0, n_newline = 0;
    Str p = {0};
    size_t index = i0;
    size_t i_link = 0;
    do {
        str_clear(&p);
        if(i_link) {
            TRYF(btw_parse_link, btw, index, &p, &link_len);
            index += link_len;
            size_t ws = btw_parse_is_ws(&btw->items, index, &n_newline);
            if(ws && n_newline <= 1 && index < vbtwlex_length(&btw->items)) {
                index += ws;
            }
        }
        i_link = btw_parse_is_link(&btw->items, index);
    } while(i_link);
    if(n_newline <= 1 && index < vbtwlex_length(&btw->items)) {
        BtwLex *item = vbtwlex_get_at(&btw->items, index);
        if(item->id == BTW_LEX_SEPARATOR && !str_cmp(&item->str, &STR("{"))) {
            ++index;
        }
    }
    *len = (index - i0);
    TRY(vstr_push_back(&btw->titles, &p), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}/*}}}*/

ErrDecl btw_parse(Nexus *nexus, Btw *btw) { //{{{
    ASSERT_ARG(nexus);
    ASSERT_ARG(btw);
    int err = 0;
    VBtwLex *items = &btw->items;
    Str pending = {0};
    TRYF(str_copy, &pending, &btw->basename); /* to avoid double free; I know, it's a bit stupid */
    TRY(vstr_push_back(&btw->titles, &pending), ERR_VEC_PUSH_BACK);
    str_zero(&pending);
    TRY(vsize_push_back(&btw->indices, vbtwlex_length(items)), ERR_VEC_PUSH_BACK);
    size_t index = 0;
    while(vstr_length(&btw->titles)) {
        str_clear(&pending);
        str_clear(&pending);
        //printf("   INDEX %zu\n", index);
        if(vstr_length(&btw->titles) != vsize_length(&btw->indices)) {
            THROW("vector length titles (%zu) != indices (%zu) mismatch!", vstr_length(&btw->titles), vsize_length(&btw->indices));
        }
        size_t note_len = 0, note_len2 = 0;
        TRYF(btw_parse_is_note, items, index, &note_len, btw);
        if(note_len) {
            /////printf(" !!! NOTE (%zu) !!!\n", note_len);
            // TODO: parse note; update context -> get title; store title+note_end
            TRY(vsize_push_back(&btw->indices, index + note_len - 1), ERR_VEC_PUSH_BACK);
            TRYF(btw_parse_note, btw, index, &pending, &note_len2);
            str_clear(&pending); // TODO: do I need those two clears?
            str_clear(&pending);
            index += (note_len2 - 1);
        } else {
            size_t link = btw_parse_is_link(items, index);
            if(link) {
                /////printf(" !!! LINK !!!\n");
                // TODO: format colored text -> add to current note (below)
                size_t link_len = 0;
                TRYF(btw_parse_link, btw, index, &pending, &link_len);
                index += (link_len - 1);
                //index += (link - 1);
            } else /* TODO: this else is temporary, until the thing above properly works */ {
                /////printf(" !!! DEFAULT !!!\n");
                // TODO: add text to current note / title
                if(index < vbtwlex_length(&btw->items)) {
                    Str *p = &vbtwlex_get_at(&btw->items, index)->str;
                    TRYF(str_fmt, &pending, "%.*s", STR_F(p));
                }
            }
        }
        {
            /* add pending + link */
            //printf("  link len1 %zu / titles len %zu\n", vstr_length(&btw->links), vstr_length(&btw->titles));
            if(!vstr_length(&btw->titles)) THROW("expecting titles... but have none!");
            Str *title = vstr_get_back(&btw->titles);
            //printf(" GOT A TITLE %.*s\n", STR_F(title));
            if(!str_length(title)) goto notitle;
            Node tempnode = {
                .title = *title,
            };
            if(!tnode_has(&nexus->nodes, &tempnode)) {
                TRYF(node_create, &tempnode, title, CMD_NONE, 0, ICON_NONE);
                tnode_add(&nexus->nodes, &tempnode);
                //printf("ADDED: %.*s\n", STR_F(&tempnode.title));
            }
            size_t i = 0, j = 0;
            if(tnode_find(&nexus->nodes, &tempnode, &i, &j)) {
                THROW(ERR_UNREACHABLE);
            }
            Node *fill = nexus->nodes.buckets[i].items[j];
            //printf("FOUND: %.*s\n", STR_F(&fill->title));
            //BtwLex *item = vbtwlex_get_at(&btw->items, index);
            if(str_length(&pending)) {
                /* trim ending newlines, up to max. 1 */
                size_t end = str_find_rnws(&fill->desc);
                size_t end2 = str_ch(&STR_I0(fill->desc, end), '\n', 2) + end;
                size_t start = str_find_nws(&pending);
                size_t start2 = str_rch(&STR_IE(pending, start), '\n', 1);
                if(start2 >= str_length(&STR_IE(pending, start))) start2 = 0;
                //printf(" start %zu, start2 %zu: [[[%.*s]]]\n", start, start2, STR_F(&STR_IE(pending, start)));
                //printf(" e %zu/%zu .. s %zu/%zu\n", end2+fill->desc.first, fill->desc.last, start2+pending.first, pending.first);
                if((fill->desc.last != fill->desc.first + end2) || (start2)) {
                    fill->desc.last = fill->desc.first + end2;
                    pending.first += start2;
                }
                TRYF(str_fmt, &fill->desc, "%.*s", STR_F(&pending));
                //printf(" (%.*s) [%.*s]\n", STR_F(title), STR_F(&pending));
            }
            /* do the links */
            for(size_t i_link = 0; i_link < vstr_length(&btw->links); ++i_link) {
                Str *s_link = vstr_get_at(&btw->links, i_link);
                if(str_length(s_link)) {
                    Node n_link = {
                        .title = *s_link,
                    };
                    //printf("LINK %.*s <<>> %.*s\n", STR_F(&fill->title_link), STR_F(&n_link.title_link));
                    TRYF(nexus_link, nexus, &n_link, fill);
                }
            }
notitle:
            vstr_clear(&btw->links);
        }
        ++index;
        while(index >= vsize_get_back(&btw->indices)) { // TODO: what's cleaner? while or the if?
            /*if(index >= vsize_get_back(&btw->indices)) */
            // TODO trim the current node... or something... and append a newline??
            size_t iE = vsize_get_back(&btw->indices);
            Str s_child = {0}, *s_parent = 0;
            Node child = {0}, parent = {0};
            //printf(" current titles len %zu / last %.*s\n", vstr_length(&btw->titles), STR_F(vstr_get_back(&btw->titles)));
            vsize_pop_back(&btw->indices, 0);
            vstr_pop_back(&btw->titles, &s_child);
            //printf(" decreased titles len %zu / last %*.s\n", vstr_length(&btw->titles), STR_F(vstr_get_back(&btw->titles)));
            if(!vsize_length(&btw->indices)) break;
            if(str_length(&s_child)) {
                s_parent = vstr_get_back(&btw->titles);
                if(str_length(s_parent)) {
                    child.title = s_child;
                    parent.title = *s_parent;
                    //printf("  link ... %.*s ... %.*s\n", STR_F(&s_child), STR_F(s_parent));
                    TRYF(nexus_link, nexus, &parent, &child);
                }
            }
            if(vsize_length(&btw->indices)) {
                //printf("   CONTEXT %.*s\n", STR_F(vstr_get_back(&btw->titles)));
                /* post-cleanup for scopes! get rid of '}' */
                if(iE < vbtwlex_length(&btw->items)) {
                    BtwLex *item = vbtwlex_get_at(&btw->items, iE);
                    if(item->id == BTW_LEX_SEPARATOR && !str_cmp(&item->str, &STR("}"))) {
                        ++index;
                    } else {
                        THROW("expected a } ... something went wrong while parsing");
                    }
                }
            }
        }
    }
    ++btw->success;
clean:
    str_free(&pending);
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
    vsize_free(&btw->indices);
    vstr_free(&btw->titles);
    vstr_free(&btw->links);
    printf("dirfiles len %zu\n", vstr_length(&btw->dirfiles));
    vstr_free(&btw->dirfiles);
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
    vbtwlex_clear(&btw->items);
    TRYF(str_fmt_basename, &btw->basename, filename);
    TRYF(str_fmt_ext, &btw->ext, filename);

    if(file_is_dir(filename)) {
        THROW("don't expect dir!");
        int recursive = 0; // TODO make a flag for this
        TRYF(file_dir_read, filename, &btw->dirfiles);
        //printf("\r%.*s", *n, "");
        //*n = printf("[DIR]  %.*s", STR_F(filename));
        INFO("directory '%.*s'", STR_F(filename));
        //INFO("directory encountered, not parsing '%.*s'", STR_F(filename));
    } else {
        bool skip = false;
#if 0
        const Str *ok[] = {
            &STR(".btw1"), &STR(".md")
        };
#endif
        if(str_cmp_ci(&btw->ext, &STR(".btw1"))) {
            // TODO make a flag for this?
            skip = true;
            INFO("incorrect extension '%.*s', not parsing '%.*s'", STR_F(&btw->ext), STR_F(filename));
        }
        if(!skip) {
            //if(*n) printf("\n");
            //*n = printf("[FILE] %.*s", STR_F(filename));
            INFO("parsing '%.*s'", STR_F(filename));
            TRYF(file_str_read, filename, &btw->content);
            str_trim(&btw->content);
        }
    }

    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_parse_file(Nexus *nexus, Str *filename, Btw *btw) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(btw);

    TRYF(btw_file_prepare, nexus, filename, btw);
    if(str_length(&btw->content)) {
        TRYF(btw_lex, &btw->items, &btw->content);
        TRYF(btw_parse, nexus, btw);
    }
    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_parse_exec(Str *filename, void *args) {/*{{{*/
    ASSERT_ARG(filename);
    ASSERT_ARG(args);
    BtwExec *a = (BtwExec *)args;
    TRYF(btw_parse_file, a->nexus, filename, a->btw);
    return 0;
error:
    return -1;
}/*}}}*/

