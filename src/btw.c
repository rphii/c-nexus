#include <ctype.h>

#include "lookup.h"
#include "nexus.h"
#include "btw.h"
#include "file.h"
#include "cmd.h"
#include "str.h"
#include "vector.h"

#include "info.h"

#if 0/*{{{*/

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
            TRYC(btw_parse_is_scope_OLD, items, index, &scope, btw);
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
            TRYC(btw_parse_is_scope, btw, index, &scope);
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
    //TRYC(str_copy, &link->str, &item_link->str);
    str_clear(&link->str); // I think this is redundant
    TRYC(str_fmt_fgbg, &link->str, &item_link->str, 0, 0, bold, italic, underline);
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
        TRYC(nexus_tag, nexus, &node_src, &node_dst, &btw->stats.links);
    } else {
        TRYC(nexus_link, nexus, &node_src, &node_dst, &btw->stats.links);
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
        TRYC(btw_parse_link, btw, index, &title);
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
        TRYC(btw_parse_link_connect, nexus, btw, &title, ref);
    }
    vbtwlink_clear(&btw->parse.refs);
    return 0;
error:
    return -1;
}/*}}}*/
#endif/*}}}*/

void btwlex_free(BtwLex *lex) { //{{{
    ASSERT_ARG(lex);
    //str_free(&lex->str);
    memset(lex, 0, sizeof(*lex));
} //}}}

#define ERR_btw_lex_append(items, id, str, i0, line_index) "failed appending lex item"
ErrDecl btw_lex_append(VBtwLex *items, BtwLexList id, size_t i0, size_t iE, size_t line_index) { //{{{
    ASSERT_ARG(items);
    BtwLex *prev = vbtwlex_length(items) ? vbtwlex_get_back(items) : 0;
    if(id == BTW_LEX_NONE) {
        THROW("id is NONE, we don't want to append that!");
    }
    if(prev) {
        if(prev->iE != i0) {
            THROW(F("!!! consistency breakage !!!", FG_RD) " previous iE(%zu) + 1 != i0(%zu)!", prev->iE, i0);
        }
    }
    if(prev && (prev->id == id && prev->id != BTW_LEX_SEPARATOR)) {
        vbtwlex_get_back(items)->iE = iE;
    } else {
        // TODO: if link, to a trim? or somewhere else?
        BtwLex item = {
            .line_num = line_index,
            //.i0 = i0,
            .iE = iE,
            .id = id,
        };
#if 0
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
#endif
        //item->line = line_index;
        //item->i0 = i0;
        TRY(vbtwlex_push_back(items, &item), ERR_VEC_PUSH_BACK);
        //str_zero(str);
        //memset(item, 0, sizeof(*item));
        //printf(F("APPEND:%u:%.*s\n", FG_BK_B), item->id, STR_F(&item->str));
    }
    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_lex(VBtwLex *items, Str *str) { //{{{
    ASSERT_ARG(items);
    ASSERT_ARG(str);
    bool err_matching_bracket = false, err_matching_angle = false;
    int err = 0;
    size_t line_index = 1, i0 = 0, iE = 0;
    BtwLexList id_next = BTW_LEX_NONE;//, id_prev = BTW_LEX_NONE;
    Str temp = {0};
    bool done_next = false;
    while(i0 < str_length(str) && iE < str_length(str)) {
        /* find out next id */
            //id_prev = id_next;
            //ASSERT(id_prev < BTW_LEX__COUNT, "id_perv should not exceed '%u'", BTW_LEX__COUNT);
            ASSERT(id_next < BTW_LEX__COUNT, "id_next should not exceed '%u'", BTW_LEX__COUNT);
            char c = str_get_at(str, iE);
#if 0
            switch(c) {
                case '<': {
                    id_next = BTW_LEX_FORMAT;
                } break;
                case '[': {
                    id_next = BTW_LEX_LINK;
                } break;
                case '{': case '}': {
                    id_next = BTW_LEX_SEPARATOR;
                } break;
                default: {
                    id_next = BTW_LEX_STRING;
                } break;
            }
#endif
            if(strchr("{}<>[]", c)) {
                id_next = BTW_LEX_SEPARATOR;
            } else if(isspace(c)) {
                id_next = BTW_LEX_WHITESPACE;
            } else {
                id_next = BTW_LEX_STRING;
            }
            /* handling first id */
#if 0
            if(id_prev == BTW_LEX_NONE) {
                id_prev = id_next;
            }
            if(id_prev != id_next && id_prev == BTW_LEX_STRING) {
                done_next = true;
            }
#endif

            /* handle next id */
            if(!done_next) {
                size_t iE_prev = iE;
                switch(id_next) {
                    case BTW_LEX_STRING: {
                    //printff("STRING");
                        ++iE;
                        done_next = true;
                    } break;
                    case BTW_LEX_WHITESPACE: {
                    //printff("WHITESPACE");
                        char c = str_get_at(str, iE);
                        while(iE < str_length(str) && (c = str_get_at(str, iE), isspace(c))) {
                            if(str_get_at(str, iE) == '\n') {
                                ++line_index;
                            }
                            ++iE;
                        }
                        done_next = true;
                    } break;
                    case BTW_LEX_FORMAT: {
                    //printff("FORMAT");
                        Str search = STR_I0(*str, i0);
                        size_t find = str_ch_pair(&search, '>');
                        if(find >= str_length(&search)) {
                            err_matching_angle = true;
                            THROW("did not find matching angle bracket");
                        }
                        iE = find + i0 + 1;
                        done_next = true;
                    } break;
                    case BTW_LEX_LINK: {
                    //printff("LINK");
                        Str search = STR_I0(*str, i0);
                        size_t find = str_ch_pair(&search, ']');
                        if(find >= str_length(&search)) {
                            err_matching_bracket = true;
                            THROW("did not find matching bracket");
                        }
                        iE = find + i0 + 1;
                        done_next = true;
                    } break;
                    case BTW_LEX_SEPARATOR: {
                    //printff("SEPARATOR");
                        ++iE;
                        done_next = true;
                    } break;
                    case BTW_LEX__COUNT:
                    case BTW_LEX_NONE: {
                        THROW(ERR_UNREACHABLE);
                    } break;
                }
                ASSERT(iE_prev < iE, "did not process case '%u' at index %zu", id_next, iE);
            }

            /* append if next doesn't match previous */
            if(done_next) {
                done_next = false;
                //printff("id %u:[%zu-%zu]:[%.*s]", id_next, iE,i0, (int)(iE-i0), str_iter_at(str, i0));
                TRYC(btw_lex_append(items, id_next, i0, iE, line_index));
                //printff("i0 %zu / iE %zu", i0, iE);
                ASSERT(i0 < iE, "something went wrong, maybe lexing didn't actually happen? i0=%zu, iE=%zu, id=%u, line=%zu", i0, iE, id_next, line_index);
                /* done, prepare next! */
                i0 = iE;
            }

        //}
        /////printf("\n");
    }
    if(iE > i0 && iE < str_length(str)) {
        TRYC(btw_lex_append(items, id_next, i0, iE, line_index));
    }
#if 0
    size_t ii0 = 0;
    for(size_t i = 0; i < vbtwlex_length(items); ++i) {
        BtwLex *item = vbtwlex_get_at(items, i);
        printf("[" F("%.*s", FG_BK_B) "]", (int)(item->iE-ii0), str_iter_at(str, ii0));
        ii0 = item->iE;
    } printf("\n");
#endif
clean:
    //printf("%.*s\n", STR_F(&temp.str));
    str_free(&temp);
    //str_free(&line);
    return err;
error:
    if(err_matching_bracket || err_matching_angle) {
        err_matching_bracket = false;
        err_matching_angle = false;
        THROW("expected matching bracket (line %zu)", line_index);
    }
    ERR_CLEAN;
} //}}}

ErrDecl btw_parse(Nexus *nexus, Btw *btw) { //{{{
    ASSERT_ARG(nexus);
    ASSERT_ARG(btw);
    int err = 0;
    //printff("got %zu..", vbtwlex_length(&btw->items));
#if 0/*{{{*/
    BtwLink link = {0};
    TRYC(str_copy, &link.str, &btw->basename);
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
        TRYC(btw_parse_is_note, btw, i, &len_note);
        len_link = btw_parse_is_link(btw, i);
        /* do the thing */
        if(len_note) {
            printf("found NOTE (%zu)\n", len_note);
            TRYC(btw_parse_note, nexus, btw, i);
            i += (len_note);
            /* special case: empty note? -> skip entirely */
            /* TODO should probably make this a bit more sleek; I just hacked this in because I wanted to see if this works. (and should probably also check *just in case* if we have titles */
            if(!str_length(&vbtwlink_get_back(&btw->parse.titles)->str)) {
                i = vsize_get_back(&btw->parse.indices) - 1;
            }
        } else if(len_link) {
            printf("found LINK (%zu)\n", len_link);
            TRYC(btw_parse_link, btw, i, &link);
            printf("   link is: %.*s\n", STR_F(&link.str));
            i += (len_link - 1);
            BtwLink *link_title = vbtwlink_get_back(&btw->parse.titles); /* TODO DRY */
            TRYC(btw_parse_link_connect, nexus, btw, link_title, &link);
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
                TRYC(nexus_link, nexus, &node_title, &node_prev, &btw->stats.links);
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
                TRYC(str_fmt, &node->desc, "%.*s", STR_F(&link.str));
            }
            if(link.str.cap) btwlink_free(&link); /* ugh I hate this */
            else memset(&link, 0, sizeof(link));
        }
    }
#endif/*}}}*/
clean:
    //btwlink_free(&link);
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
    TRYC(str_fmt_basename(&btw->basename, filename));
    TRYC(str_fmt_ext(&btw->ext, filename));

    if(file_is_dir(filename)) {
        THROW("don't expect dir!");
        int recursive = 0; // TODO make a flag for this
        TRYC(file_dir_read(filename, &btw->dirfiles));
        //printf("\r%.*s", *n, "");
        //*n = printf("[DIR]  %.*s", STR_F(filename));
        info(directory, "directory '%.*s'", STR_F(filename));
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
            info(parsing_skip_incorrect_extension, "incorrect extension '%.*s', not parsing '%.*s'", STR_F(&btw->ext), STR_F(filename));
        }
        if(!skip) {
            //if(*n) printf("\n");
            //*n = printf("[FILE] %.*s", STR_F(filename));
            info(parsing_file, "parsing '%.*s'", STR_F(filename));
            TRYC(file_str_read(filename, &btw->content));
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

    TRYC(btw_file_prepare(nexus, filename, btw));
    if(str_length(&btw->content)) {
        TRYC(btw_lex(&btw->items, &btw->content));
        TRYC(btw_parse(nexus, btw));
    }
    return 0;
error:
    return -1;
} //}}}

ErrDecl btw_parse_exec(Str *filename, void *args) {/*{{{*/
    ASSERT_ARG(filename);
    ASSERT_ARG(args);
    BtwExec *a = (BtwExec *)args;
    TRYC(btw_parse_file(a->nexus, filename, a->btw));
    return 0;
error:
    return -1;
}/*}}}*/

void btwlink_free(BtwLink *link) {/*{{{*/
    ASSERT_ARG(link);
    str_free(&link->str);
    memset(link, 0, sizeof(*link));
}/*}}}*/

