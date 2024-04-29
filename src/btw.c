#include <ctype.h>

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

#define btw_lex_append_ERR(items, id, str, i0, line_index) "failed appending lex item"
ErrDecl btw_lex_append(VBtwLex *items, BtwLexList id, Str *str, size_t i0, size_t line_index) { //{{{
    ASSERT_ARG(items);
    ASSERT_ARG(str);
    // TODO: if link, to a trim? or somewhere else?
    BtwLex item = {
        .line_i0 = i0,
        .line_num = line_index,
        .str = *str,
        .id = id,
    };
    if(item.id == BTW_LEX_STRING) {
        //str_trim(&item->str);
        if(!str_length(&item.str)) {
            return 0;
        }
    }
    if(item.id == BTW_LEX_LINK) {
        str_trim(&item.str);
        //if(!str_length(&item->str)) {
        //    return 0;
        //}
    }
#if 0
    printf(F("%zu", BG_BK_B), vbtwlex_length(items));
    //if(item->flag) {
    //    printf(F("F", BG_WT_B FG_BK));
    //}
    if(item.id == BTW_LEX_STRING) {
        printf(F("[", FG_BK_B) "%.*s" F("]", FG_BK_B), STR_F(&item.str));
    }
    if(item.id == BTW_LEX_LINK) {
        printf(F("'%.*s'", FG_CY_B), STR_F(&item.str));
    }
    if(item.id == BTW_LEX_FORMAT) {
        printf(F("%.*s", FG_RD), STR_F(&item.str));
    }
    if(item.id == BTW_LEX_SEPARATOR) {
        printf(F("%.*s", FG_BL_B), STR_F(&item.str));
    }
#endif
    //item->line = line_index;
    //item->i0 = i0;
    TRY(vbtwlex_push_back(items, &item), ERR_VEC_PUSH_BACK);
    str_zero(str);
    //memset(item, 0, sizeof(*item));
    //printf(F("APPEND:%u:%.*s\n", FG_BK_B), item->id, STR_F(&item->str));
    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_lex(VBtwLex *items, Str *str) { //{{{
    ASSERT_ARG(items);
    ASSERT_ARG(str);
    int err = 0;
    size_t n_emptyline = 0;
    size_t index = 0, line_index = 0, i0 = 0;
    Str temp = {0};
    //temp.id = BTW_LEX_STRING;
    Str line = {0};
    while(index < str_length(str)) {
        i0 = index;
        str_clear(&line);
        ++line_index;
        TRYF(str_fmt_line, &line, str, index, &index);
        str_trim(&line);
        /* go over (trimmed) lines */
        //printf("line %zu:%.*s\n", line_index, STR_F(&line));
#if 1
        bool have_any = false;
        if(!str_length(&line)) {
            have_any = true;
            ++n_emptyline;
        }
        if(!have_any) {
            n_emptyline = 0;
            do {
                have_any = false;
#if 1
                /* check format+link */
                printf("line %zu:" F("%.*s", FG_BK_B) "\n", line_index, STR_F(&line));
                /* matching brackets*/
                size_t i_br0 = str_ch(&line, '[', 0);
                size_t i_brE = str_ch_pair(&STR_I0(line, i_br0), ']') + i_br0;
                /* separators */
                size_t i_sepbeg = 0;//(i_brE < str_length(&line)) ? i_brE : 0;
                size_t i_sep = str_find_any(&STR_I0(line, i_sepbeg), &STR("|{}")) + i_sepbeg;
                /* any whitespace */
                size_t i_wsbeg = i_brE < str_length(&line) ? i_brE : 0;
                size_t i_ws = str_find_ws(&STR_I0(line, i_wsbeg)) + i_wsbeg;
                /* format, begins at bracket end, ends at whitespace or |[{ */
                size_t i_fmt0 = i_brE + 1;
                size_t i_fmtE1 = i_sep < i_ws ? i_sep : i_ws;
                size_t i_fmtE2 = str_find_any(&STR_I0(line, i_fmt0), &STR("|{[.,'\"")) + i_fmt0;
                size_t i_fmtE = i_fmtE1 < i_fmtE2 ? i_fmtE1 : i_fmtE2;
                /* ... */
                size_t done = 0;
                bool have_link = (bool)(i_br0 < i_brE && i_brE < str_length(&line));
                bool have_fmt = (bool)(i_fmt0 < i_fmtE && i_fmt0 < str_length(&line));
                bool have_sep = (bool)(i_sep < str_length(&line) && ((!have_link) || (i_sep < i_br0)));
                have_link &= !have_sep;
                //printf("\nlink %s, fmt %s, sep %s\n", have_link ? "YES" : "no", have_fmt ? "YES" : "no", have_sep ? "YES":"no");
                have_any = (bool)(have_link || have_fmt || have_sep);
                Str s_fmt = {0};
                Str s_link = {0};
                Str s_sep = {0};
                if(have_link) {
                    /* valid link */
                    s_link = STR_LL(str_iter_begin(&STR_I0(line, i_br0+1)), i_brE-i_br0-1);
                    done += str_length(&s_link)+2; // +2 because trimmed []
                    printf("LINK: %.*s (%zu)\n", STR_F(&s_link), str_length(&s_link)+2);
                    str_trim(&s_link);
                }
                if(have_fmt) {
                    /* valid color */
                    s_fmt = STR_LL(str_iter_begin(&STR_I0(line, i_fmt0)), i_fmtE-i_fmt0);
                    done += str_length(&s_fmt);
                    /* should be trimmed!.. let's hope it is */
                    printf("FORMAT: %.*s (%zu)\n", STR_F(&s_fmt), str_length(&s_fmt));
                }
                if(have_sep) {
                    /* valid separator */
                    s_sep = STR_LL(str_iter_begin(&STR_I0(line, i_sep)), 1);
                    done += str_length(&s_sep);
                    printf("SEP: %.*s (%zu)\n", STR_F(&s_sep), str_length(&s_sep));
                }
                /* push all lex items */
                if(have_any) {
                    /* append previous stuff */
                    //handled = true;
                    size_t until = (i_br0 < i_sep) ? i_br0 : i_sep;
                    done += until;
                    TRYF(str_fmt, &temp, "%.*s", (int)until, str_iter_begin(&line));
                    printf("REST: %.*s\n", STR_F(&temp));
                    TRYF(btw_lex_append, items, BTW_LEX_STRING, &temp, i0, line_index);
                }
                if(have_link) {
                    /* append link */
                    TRYF(str_fmt, &temp, "%.*s", STR_F(&s_link));
                    //printf("LINK: %.*s\n", STR_F(&temp));
                    TRYF(btw_lex_append, items, BTW_LEX_LINK, &temp, i0, line_index);
                }
                if(have_fmt) {
                    /* append format */
                    TRYF(str_fmt, &temp, "%.*s", STR_F(&s_fmt));
                    //printf("FMT:  %.*s\n", STR_F(&temp));
                    TRYF(btw_lex_append, items, BTW_LEX_FORMAT, &temp, i0, line_index);
                }
                if(have_sep) {
                    /* append link */
                    TRYF(str_fmt, &temp, "%.*s", STR_F(&s_sep));
                    //printf("FMT:  %.*s\n", STR_F(&temp));
                    TRYF(btw_lex_append, items, BTW_LEX_SEPARATOR, &temp, i0, line_index);
                }
                /* adjust index */
                if(!have_any) {
                    TRYF(str_fmt, &temp, "%.*s", STR_F(&line));
                } else {
                    //printf(" \t\tdone = %zu\n", done);
                    line.first += done;
                    if(line.first > line.last) line.first = line.last;
                    if(!str_length(&line)) break;
                }
#else/*{{{*/
                size_t sep0 = str_find_any(&line, &STR("{}|"));
                size_t f0 = str_ch(&line, '#', 0);
                size_t f1 = str_ch(&line, '[', 0);
                //printf("\n[[sep0 %zu  f0 %zu  f1 %zu:%.*s]]\n", sep0, f0, f1, STR_F(&line));
                if(f1 < str_length(&line) && (f1 < sep0 || f0 < sep0)) {
                    //if(f1 < str_length(&line)) {
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
                            temp.id = BTW_LEX_FORMAT_FG;
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
                        size_t ytag = str_ch(&STR_I0(line, f3), ':', 0) + f3;
                        // TODO: fix this shit // the mess above :)
                        // just put it into format->string?
                        //printf("\n[[[fin11:%zu fin12:%zu fin1:%zu fin2:%zu done%zu b%zu i%zu u%zu !%zu]]]\n", fin11, fin12, fin1, fin2, done, bold, ital, undl, nlnk);
                        if(bold < done) temp.flag |= BTW_FLAG_BOLD;
                        if(ital < done) temp.flag |= BTW_FLAG_ITALIC;
                        if(undl < done) temp.flag |= BTW_FLAG_UNDERLINE;
                        if(nlnk < done) temp.flag |= BTW_FLAG_NOLINK;
                        if(ytag < done) temp.flag |= BTW_FLAG_TAG;
                        /////printf("%.*s", (int)(done-f3-1), str_iter_begin(&STR_I0(line, f3+1)));
                        temp.id = BTW_LEX_LINK;
                        TRYF(btw_lex_append, items, &temp, i0, line_index);
                        /* idk man */
                        if(f2b == true) {
                            temp.id = BTW_LEX_FORMAT_BG;
                            TRYF(str_fmt, &temp.str, "%.*s", (int)(f3-f2+1), str_iter_begin(&STR_I0(line, f2)));
                            TRYF(btw_lex_append, items, &temp, i0, line_index);
                        }
#if 0
                        if(f0b || f2b) {
                            temp.id = BTW_LEX_SEPARATOR;
                            TRYF(str_fmt, &temp.str, "|");
                            TRYF(btw_lex_append, items, &temp, i0, line_index);
                        }
#endif
                        line.first += done; // + (fin2 == done);
                        if(line.first > line.last) line.first = line.last;
                        if(!str_length(&line)) break;
                    }
                } else if(sep0 < str_length(&line)) {
                    //printf(" => sep\n");
                    /* push back previously found string */
                    handled = true;
                    size_t until = sep0;
                    TRYF(str_fmt, &temp.str, "%.*s", (int)(until), str_iter_begin(&line));
                    TRYF(btw_lex_append, BTW_LEX_STRING, items, &temp, i0, line_index);
                    /* push back current separators */
                    TRYF(str_fmt, &temp.str, "%.*s", 1, str_iter_begin(&STR_I0(line, sep0)));
                    TRYF(btw_lex_append, BTW_LEX_SEPARATOR, items, &temp, i0, line_index);
                    line.first += until + 1;
                }
                if(!handled) {
                    TRYF(str_fmt, &temp.str, "%.*s", STR_F(&line));
                }
#if 0
                printf("LINE:%.*s\n", STR_F(&line));
#endif
#endif/*}}}*/
            } while(have_any);
        }
        if(n_emptyline < 2) {
            TRYF(str_fmt, &temp, "\n");
        }
#endif
        /////printf("\n");
    }
    TRYF(btw_lex_append, items, BTW_LEX_STRING, &temp, i0, line_index);
clean:
    //printf("%.*s\n", STR_F(&temp.str));
    str_free(&temp);
    str_free(&line);
    return err;
error:
    ERR_CLEAN;
} //}}}

bool btw_parse_color(Btw *btw, const Str *str, V3u8 col) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(str);
    ASSERT_ARG(col);
    if(!str_length(str)) return false;
    /* maybe it's a number */
    if(str_length(str) == 7) {
        int n = str_find_nany(&STR_I0(*str, 1), &STR("0123456789abcdefABCDEF"));
        if(n == 6) { /* got 6 valid digits */
            char *endptr = 0;
            uint32_t val = strtoul(str_iter_begin(&STR_I0(*str, 1)), &endptr, 16);
            printf(" val:%06x\n", val);
            col[0] = ((val >> 16) & 0xFF);
            col[1] = ((val >>  8) & 0xFF);
            col[2] = ((val >>  0) & 0xFF);
            printf(" 0x%02x%02x%02x <- %.*s\n", col[0], col[1], col[2], STR_F(str));
            return true;
        }
    }
    /* check in lookup table for possible colors TODO */
    return false;
}/*}}}*/

size_t btw_parse_match_pattern(VBtwLex *items, size_t i0, size_t n_pat, const BtwLexList **pat) {/*{{{*/
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
}/*}}}*/

#if 0/*{{{*/
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

#define btw_parse_is_scope_OLD_ERR(items, i0, len, ref) "failed confirming scope"
ErrDecl btw_parse_is_scope_OLD(VBtwLex *items, size_t i0, size_t *len, Btw *btw) {/*{{{*/
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
            line = item->line_num;
            line_i0 = item->line_i0;
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

#define btw_parse_is_note_OLD_ERR(items, i0, note, btw) "could not confirm scope"
ErrDecl btw_parse_is_note_OLD(VBtwLex *items, size_t i0, size_t *len, Btw *btw) {/*{{{*/
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
            TRYF(btw_parse_is_scope_OLD, items, index, &scope, btw);
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
#endif/*}}}*/


size_t btw_parse_is_link(Btw *btw, size_t i0) {/*{{{*/
    ASSERT_ARG(btw);
    size_t result = 0;
    if(i0+0 < vbtwlex_length(&btw->items) && vbtwlex_get_at(&btw->items, i0+0)->id == BTW_LEX_LINK) ++result;
    if(i0+1 < vbtwlex_length(&btw->items) && vbtwlex_get_at(&btw->items, i0+1)->id == BTW_LEX_FORMAT) ++result;
    return result;
}/*}}}*/

size_t btw_parse_is_ws(Btw *btw, size_t i0, size_t *n_newline) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(n_newline);
    if(i0 >= vbtwlex_length(&btw->items)) return 0;
    BtwLex *item = vbtwlex_get_at(&btw->items, i0);
    if(item->id == BTW_LEX_STRING) {
        size_t nws = str_find_nws(&item->str);
        if(nws >= str_length(&item->str)) {
            *n_newline = str_count_ch(&item->str, '\n');
            return 1;
        }
    }
    return 0;
}/*}}}*/

#define btw_parse_is_scope_ERR(btw, i0, len) "failed confirming scope"
ErrDecl btw_parse_is_scope(Btw *btw, size_t i0, size_t *len) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(len);
    if(i0 >= vbtwlex_length(&btw->items)) return 0;
    /* error stuff */
    bool err_scope = false;
    Str err_hint = {0};
    size_t line_i0 = 0;
    size_t line = -1;
    /* non-error stuff */
    size_t index = i0;
    int level = 0;
    bool valid = false;
    do {
        if(index >= vbtwlex_length(&btw->items)) return 0;
        BtwLex *item = vbtwlex_get_at(&btw->items, index);
        if(index == i0) {
            line = item->line_num;
            line_i0 = item->line_i0;
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
    } while(level > 0 && index < vbtwlex_length(&btw->items));
    if(level > 0) { err_scope = true; THROW("brackets { or } mismatch of %i levels on line %zu:", level, line); }
    if(level == 0 && valid) {
        //printf("  ..scope len %zu (%zu-%zu)\n", index-i0, index, i0);
        *len = (index - i0);
    }
    return 0;
error:
    if(err_scope) {
        (void)str_fmt_line(&err_hint, &btw->content, line_i0, 0);
        printf(" %.*s:" F("%zu", FG_WT_B) " | %.*s\n", STR_F(btw->filename), line, STR_F(&err_hint));
        str_free(&err_hint);
    }
    return -1;
}/*}}}*/

#define btw_parse_is_note_ERR(btw, i0, len) "failed confirming note"
ErrDecl btw_parse_is_note(Btw *btw, size_t i0, size_t *len) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(len);
    if(i0 >= vbtwlex_length(&btw->items)) return 0;
    size_t index = i0;
    // TODO icons not handled!!!!!!!! (they get ignored)
    size_t link = btw_parse_is_link(btw, index);
    if(link) {
next:
        //printf("  ..link %zu\n", index);
        index += link;
        size_t n_newline = 0;
        size_t ws = btw_parse_is_ws(btw, index, &n_newline);
        if(n_newline <= 1) {
            index += ws;
            link = btw_parse_is_link(btw, index);
            if(link) goto next; // not too beautiful.. but it.. works? TODO maybe get rid of this goto???
            size_t scope = 0;
            TRYF(btw_parse_is_scope, btw, index, &scope);
            if(scope) {
                *len = (index - i0);
                index += scope;
                TRY(vsize_push_back(&btw->parse.indices, index-1), ERR_VEC_PUSH_BACK);
            }
        }
    }
    return 0;
error:
    return -1;
}/*}}}*/

#define btw_parse_link_ERR(btw, i0, link) "failed parsing link"
ErrDecl btw_parse_link(Btw *btw, size_t i0, BtwLink *link) {/*{{{*/
    ASSERT_ARG(btw);
    //ASSERT_ARG(len);
    ASSERT_ARG(link);
    int err_flag = false;
    Str err_hint = {0};
    BtwLex *item_link = vbtwlex_get_at(&btw->items, i0);
    BtwLex *item_fmt = 0;
    ASSERT(item_link->id == BTW_LEX_LINK, "expected a link (= %u, but is %u)", BTW_LEX_LINK, item_link->id);
    if(i0 + 1 < vbtwlex_length(&btw->items)) {
        item_fmt = vbtwlex_get_at(&btw->items, i0 + 1);
        if(item_fmt->id != BTW_LEX_FORMAT) item_fmt = 0;
    }
    bool has_fg = false;
    bool has_bg = false;
    bool bold = false, italic = false, underline = false;
    V3u8 fg = {0};
    V3u8 bg = {0};
    if(item_fmt) {
        size_t i = 0;
        for(i = 0; i < str_length(&item_fmt->str); ++i) {
            char c = str_get_at(&item_fmt->str, i);
            switch(c) {
                case ':': { link->flags |= BTW_FLAG_TAG; } break;
                case '!': { link->flags |= BTW_FLAG_NOLINK; } break;
                case '*': { bold = true; } break;
                case '/': { italic = true; } break;
                case '_': { underline = true; } break;
                case '#': break;
                case '(': break;
                default: {
                    err_flag = true;
                    THROW("unknown format modifier: '%c' on line %zu", c, item_fmt->line_num);
                } break;
            }
            if(c == '#') break;
            if(c == '(') break;
        }
        //if(str_ch(&item_fmt->str, ':', 0) < str_length(&item_fmt->str)) link->flags |= BTW_FLAG_TAG;
        //if(str_ch(&item_fmt->str, '*', 0) < str_length(&item_fmt->str)) bold = true;
        //if(str_ch(&item_fmt->str, '/', 0) < str_length(&item_fmt->str)) italic = true;
        //if(str_ch(&item_fmt->str, '_', 0) < str_length(&item_fmt->str)) underline = true;
        //printf(" HAS FMT: 0x%x\n", link->flags);
    }
    //TRYF(str_copy, &link->str, &item_link->str);
    str_clear(&link->str); // I think this is redundant
    TRYF(str_fmt_fgbg, &link->str, &item_link->str, 0, 0, bold, italic, underline);
    //*len += (size_t)(bool)(item_link) + (size_t)(bool)(item_fmt);
    return 0;
error:
    if(err_flag) {
        (void)str_fmt_line(&err_hint, &btw->content, item_fmt->line_i0, 0);
        printf(" %.*s:" F("%zu", FG_WT_B) " | %.*s\n", STR_F(btw->filename), item_fmt->line_num, STR_F(&err_hint));
        str_free(&err_hint);
    }
    return -1;
}/*}}}*/

#define btw_parse_link_connect_ERR(nexus, btw, src, dst) "failed connecting links"
ErrDecl btw_parse_link_connect(Nexus *nexus, Btw *btw, BtwLink *src, BtwLink *dst) {/*{{{*/
    ASSERT_ARG(nexus);
    ASSERT_ARG(btw);
    ASSERT_ARG(src);
    ASSERT_ARG(dst);
    Node node_dst = { .title = dst->str };
    Node node_src = { .title = src->str };
    if(dst->flags & BTW_FLAG_NOLINK) {
    } else if(dst->flags & BTW_FLAG_TAG) {
        printf(" tag %.*s .. %.*s\n", STR_F(&node_src.title), STR_F(&node_dst.title));
        TRYF(nexus_tag, nexus, &node_src, &node_dst, &btw->stats.links);
    } else {
        TRYF(nexus_link, nexus, &node_src, &node_dst, &btw->stats.links);
    }
    return 0;
error:
    return -1;
}/*}}}*/

#define btw_parse_note_ERR(nexus, btw, i0) "failed parsing note"
ErrDecl btw_parse_note(Nexus *nexus, Btw *btw, size_t i0) {/*{{{*/
    ASSERT_ARG(btw);
    size_t index = i0;
    BtwLink title = {0};
    size_t is_link = btw_parse_is_link(btw, index);
    for(;;) {
        TRYF(btw_parse_link, btw, index, &title);
        index += is_link;
        size_t n_newline = 0;
        size_t ws = btw_parse_is_ws(btw, index, &n_newline);
        ASSERT(n_newline < 2, "n_newline (%zu) is not < 2", n_newline);
        index += ws;
        is_link = btw_parse_is_link(btw, index);
        if(!is_link) {
            //printf("LINK!\n");
            TRY(vbtwlink_push_back(&btw->parse.titles, &title), ERR_VEC_PUSH_BACK);
            break;
        } else {
            //printf("REF!\n");
            TRY(vbtwlink_push_back(&btw->parse.refs, &title), ERR_VEC_PUSH_BACK);
        }
        /* next ... */
        memset(&title, 0, sizeof(title));
    }
    /* link ... */
    //printf("REF LEN %zu\n", vbtwlink_length(&btw->parse.refs));
    for(size_t i = 0; i < vbtwlink_length(&btw->parse.refs); ++i) {
        BtwLink *ref = vbtwlink_get_at(&btw->parse.refs, i);
        TRYF(btw_parse_link_connect, nexus, btw, &title, ref);
    }
    vbtwlink_clear(&btw->parse.refs);
    return 0;
error:
    return -1;
}/*}}}*/

ErrDecl btw_parse(Nexus *nexus, Btw *btw) { //{{{
    ASSERT_ARG(nexus);
    ASSERT_ARG(btw);
    int err = 0;
    BtwLink link = {0};
    TRYF(str_copy, &link.str, &btw->basename);
    TRY(vbtwlink_push_back(&btw->parse.titles, &link), ERR_VEC_PUSH_BACK);
    TRY(vsize_push_back(&btw->parse.indices, vbtwlex_length(&btw->items)), ERR_VEC_PUSH_BACK);
    for(size_t i = 0; i < vbtwlex_length(&btw->items); ++i) {
        if(!vbtwlink_length(&btw->parse.titles)) THROW("should not have no title anymore");
        memset(&link, 0, sizeof(link));
        size_t len_note = 0;
        size_t len_link = 0;
        BtwLex *item = vbtwlex_get_at(&btw->items, i);
        //printf("i=%zu (indices len %zu / last %zu)\n", i, vsize_length(&btw->parse.indices), vsize_length(&btw->parse.indices) ? vsize_get_back(&btw->parse.indices) : -1);
            ASSERT(vsize_length(&btw->parse.indices) == vbtwlink_length(&btw->parse.titles), "indices length (%zu) not equal to that of titles (%zu)", vsize_length(&btw->parse.indices), vbtwlink_length(&btw->parse.titles));
        TRYF(btw_parse_is_note, btw, i, &len_note);
        len_link = btw_parse_is_link(btw, i);
        /* do the thing */
        if(len_note) {
            printf("found NOTE (%zu)\n", len_note);
            TRYF(btw_parse_note, nexus, btw, i);
            i += (len_note);
        } else if(len_link) {
            printf("found LINK (%zu)\n", len_link);
            TRYF(btw_parse_link, btw, i, &link);
            printf("   link is: %.*s\n", STR_F(&link.str));
            i += (len_link - 1);
            BtwLink *link_title = vbtwlink_get_back(&btw->parse.titles); /* TODO DRY */
            TRYF(btw_parse_link_connect, nexus, btw, link_title, &link);
        } else if(vsize_length(&btw->parse.indices) && vsize_get_back(&btw->parse.indices) == i) {
            /* TODO should probably check if i > back of indices ... */
            if(!(item->id == BTW_LEX_SEPARATOR && str_get_at(&item->str, 0) == '}')) {
                THROW("expected lex item to be }");
            }
            printf("found END note\n");
            BtwLink prev = {0};
            vbtwlink_pop_back(&btw->parse.titles, &prev);
            vsize_pop_back(&btw->parse.indices, 0);
            if(vbtwlink_length(&btw->parse.titles)) {
                BtwLink *title = vbtwlink_get_back(&btw->parse.titles);
                Node node_prev = { .title = prev.str };
                Node node_title = { .title = title->str };
                TRYF(nexus_link, nexus, &node_title, &node_prev, &btw->stats.links);
            }
        } else {
            if(!(item->id == BTW_LEX_SEPARATOR && str_length(&item->str) && str_get_front(&item->str) == '|')) {
                link.str = item->str;
                link.str.cap = 0; /* ugh I hate this */
            }
        }
        if(str_length(&link.str)) {
            BtwLink *link_title = vbtwlink_get_back(&btw->parse.titles);
            printf("   link is: %.*s\n", STR_F(&link.str));
            printf("found STRING .. %.*s\n", STR_F(&link_title->str));
            /* get title's note */
            Node node_title = { .title = link_title->str };
            Node *node = 0;
            TRY(nexus_find_or_create(nexus, &node_title, &node), ERR_NEXUS_FIND_OR_CREATE);
            if(node) {
                /* TODO check flags! */
                TRYF(str_fmt, &node->desc, "%.*s", STR_F(&link.str));
            }
            if(link.str.cap) btwlink_free(&link); /* ugh I hate this */
            else memset(&link, 0, sizeof(link));
        }
    }
clean:
    btwlink_free(&link);
    return err;
error:
    ERR_CLEAN;
} //}}}

void btw_free(Btw *btw) { //{{{
    ASSERT_ARG(btw);
//    for(int i = 0; i < ICON_BUNDLE_MAX; ++i) {
//        str_free(&btw->icons.items[i].str);
//    }
    str_free(&btw->content);
    str_free(&btw->basename);
    str_free(&btw->ext);
    vbtwlex_free(&btw->items);
    vbtwlink_free(&btw->parse.refs);
    vbtwlink_free(&btw->parse.titles);
    tnode_free(&btw->parse.nodes);
    vsize_free(&btw->parse.indices);
    //vsize_free(&btw->indices);
    //vsize_free(&btw->flags);
    //vstr_free(&btw->titles);
    //vstr_free(&btw->links);
    vstr_free(&btw->dirfiles);
} //}}}

ErrDecl btw_file_prepare(Nexus *nexus, Str *filename, Btw *btw) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(filename);
    ASSERT_ARG(btw);

    ++btw->stats.attempts;
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
#if 1
        const Str *ok[] = {
            &STR(".btw1"), &STR(".md"),// &STR(".txt"),
        };
        if(str_cmp_ci_any(&btw->ext, ok, sizeof(ok)/sizeof(*ok))) {
#else
        if(str_cmp_ci(&btw->ext, &STR(".btw1"))) {
#endif
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

void btwlink_free(BtwLink *link) {/*{{{*/
    ASSERT_ARG(link);
    str_free(&link->str);
    memset(link, 0, sizeof(*link));
}/*}}}*/

