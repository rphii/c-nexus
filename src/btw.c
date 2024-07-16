#include <ctype.h>

#include "err.h"
#include "lookup.h"
#include "nexus.h"
#include "btw.h"
#include "file.h"
#include "cmd.h"
#include "str.h"
#include "vector.h"

#include "info.h"

void btwlex_free(BtwLex *lex) { //{{{
    ASSERT_ARG(lex);
    //str_free(&lex->str);
    memset(lex, 0, sizeof(*lex));
} //}}}

bool btw_lex_is_separator(BtwLexList id) {/*{{{*/
    switch(id) {
        case BTW_LEX_SCOPE_START:
        case BTW_LEX_SCOPE_END:
        case BTW_LEX_LINK_START:
        case BTW_LEX_LINK_END:
        case BTW_LEX_FORMAT_START:
        case BTW_LEX_FORMAT_END:
            break;
        default:
            return false;
    }
    return true;
}/*}}}*/

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
    if(prev && (prev->id == id && !btw_lex_is_separator(prev->id))) {
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
            id_next = BTW_LEX_STRING; /* assume the next id is a string - if it's not, it gets overwritten */
            switch(c) {
                case '{': { id_next = BTW_LEX_SCOPE_START; } break;
                case '}': { id_next = BTW_LEX_SCOPE_END; } break;
                case '[': { id_next = BTW_LEX_LINK_START; } break;
                case ']': { id_next = BTW_LEX_LINK_END; } break;
                case '<': { id_next = BTW_LEX_FORMAT_START; } break;
                case '>': { id_next = BTW_LEX_FORMAT_END; } break;
                default: break;
            }
            if(isspace(c)) {
                id_next = BTW_LEX_WHITESPACE;
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
                    case BTW_LEX_SCOPE_START:
                    case BTW_LEX_SCOPE_END:
                    case BTW_LEX_LINK_START:
                    case BTW_LEX_LINK_END:
                    case BTW_LEX_FORMAT_START:
                    case BTW_LEX_FORMAT_END: {
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
                //printff("id %u:[%zu..%zu]:[%.*s]", id_next, i0,iE, (int)(iE-i0), str_iter_at(str, i0));platform_getch();
                TRYC(btw_lex_append(items, id_next, i0 + str->first, iE + str->first, line_index));
                //printff("i0 %zu / iE %zu", i0, iE);
                ASSERT(i0 < iE, "something went wrong, maybe lexing didn't actually happen? i0=%zu, iE=%zu, id=%u, line=%zu", i0, iE, id_next, line_index);
                /* done, prepare next! */
                i0 = iE;
            }

        //}
        /////printf("\n");
    }
    if(iE > i0 && iE < str_length(str)) {
        TRYC(btw_lex_append(items, id_next, i0 + str->first, iE + str->first, line_index));
    }
    TRYC(btw_lex_append(items, BTW_LEX_END, iE + str->first, iE + str->first, line_index));
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

void btw_parse_free(BtwParse *parse) {/*{{{*/
    ASSERT_ARG(parse);
    vsstr_free(&parse->notes);
    tnode_free(&parse->core.nodes);
    trnode_free(&parse->core.icons);
}/*}}}*/

#define ERR_btw_parse_init(...)   "failed initializing parse struct"
ErrDecl btw_parse_init(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(parse);
    ASSERT_ARG(btw);
    TRY(vsstr_push_back(&parse->notes, &btw->basename), ERR_VEC_PUSH_BACK);
    parse->at_least_one_is_empty |= !(str_cmp(&btw->basename, &STR("")));
    if(!parse->at_least_one_is_empty) {
        TRYC(nexus_create_if_nonexist(&parse->core, &btw->basename)); // TODO:CONTINUE
    }
    TRY(tnode_init(&btw->parsed.nodes, 8), ERR_LUTD_INIT);
    parse->snippet = btw->content;
    parse->pending = btw->content;
    parse->format = btw->content;
    parse->text = btw->content;
    parse->link = btw->content;
    parse->stage_pair = 0;
    parse->stage = BTW_PARSE_STRING;
    parse->format_i0 = 0;
    parse->pending.last = parse->pending.first;
    parse->snippet.last = parse->snippet.first;
    parse->format.last = parse->format.first;
    parse->text.last = parse->text.first;
    parse->i_prev = 0;
    return 0;
error:
    return -1;
}/*}}}*/

void btw_parse_recall(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(parse);
    ASSERT_ARG(btw);
    //parse->format.first = parse->format.last; //crap-code
    parse->i = parse->format_i0 + 0;
    if(parse->i > 1) {
        parse->item = vbtwlex_get_at(&btw->items, parse->i - 1);
        //parse->item = vbtwlex_get_at(&btw->items, parse->i);
        parse->text.first = parse->item->iE;
        //printff("FIRST");
    } else {
        parse->item = vbtwlex_get_front(&btw->items);
        parse->text.first = 0;
        //printff("SECOND");
    }
        //printff("\nrecalled...[%.*s]", 20, &parse->text.s[parse->text.first]);
    parse->stage = BTW_PARSE_STRING; // TODO: here or outside, where I call it?
}/*}}}*/

void btw_parse_string(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(parse);
    ASSERT_ARG(btw);
    if(parse->item->id == BTW_LEX_LINK_START) {
        parse->pending.first = parse->snippet.first;
        parse->stage_pair = 1;
        parse->stage = BTW_PARSE_LINK;
    } else if(parse->item->id == BTW_LEX_FORMAT_START) {
        parse->format.first = parse->snippet.first;
        parse->format_i0 = parse->i;
        parse->stage = BTW_PARSE_FORMAT;
    } else {
        if(parse->item->id != BTW_LEX_WHITESPACE && str_length(&parse->format)) {
            parse->format.first = parse->format.last; //crap-code
            //printff("\nRECALLING");
            btw_parse_recall(btw, parse);
            //printff("RECALLED STRING:[%.*s]", 20, str_iter_at(&parse->text, parse->
        }
        parse->stage = BTW_PARSE_STRING;
    }
}/*}}}*/

#define ERR_btw_parse_text(...)     "failed parsing text"
ErrDecl btw_parse_text(Btw *btw, BtwParse *parse, Str *a, Str *b) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    ASSERT_ARG(a);
    parse->text.last = a->first;
    // ??? printff("\nTEXT: LENGTH %zu", vsstr_length(&parse->notes));
    if(parse->text.last < parse->text.first) THROW("\n>>> last %zu:\n%.200s\n\n>>> first: %zu:\n%.200s\n", parse->text.last, &parse->text.s[parse->text.last], parse->text.first, &parse->text.s[parse->text.first]);
    if(str_length(&parse->text) && !parse->at_least_one_is_empty) {
        info(INFO_parsing_found_text, F("Text:%.*s:", FG_GN_B) "%.*s", STR_F(vsstr_get_back(&parse->notes)), STR_F(&parse->text));
        TRYC(nexus_add_text(&parse->core, vsstr_get_back(&parse->notes), &parse->text));
    }
    parse->text.first = b ? b->last : a->last;
    return 0;
error:
    return -1;
}/*}}}*/

#define ERR_btw_parse_note_end(...)     "failed parsing note end"
ErrDecl btw_parse_note_end(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    if(parse->stage != BTW_PARSE_FORMAT) {
        if(vsstr_length(&parse->notes) > 1 && parse->item->id == BTW_LEX_SCOPE_END) {
            Str title = {0};
            TRYC(btw_parse_text(btw, parse, &parse->snippet, 0));
            vsstr_pop_back(&parse->notes, &title);
            parse->at_least_one_is_empty &= !(str_cmp(&title, &STR("")));
            info(INFO_parsing_found_note, F("NoteEnd (%zu) @ %zu:", FG_MG_B) "%.*s", vsstr_length(&parse->notes), parse->snippet.last, STR_F(&title));
        }
        if(!vsstr_length(&parse->notes)) {
            parse->quit = true;
        }
    }

    return 0;
error:
    return -1;
}/*}}}*/

#define ERR_btw_parse_format_end(...)     "failed parsing note end"
ErrDecl btw_parse_format_end(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    if(str_length(&parse->format)) {
        TRYC(btw_parse_text(btw, parse, &parse->format, 0));
        info(INFO_parsing_found_format, F("Format:", FG_GN_B) "%.*s", STR_F(&parse->format));
        parse->fmt = parse->format;
        parse->format.first = parse->format.last;
    }
    return 0;
error:
    return -1;
}/*}}}*/

void btw_parse_format_begin(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    if(parse->item->id == BTW_LEX_FORMAT_END) {
        parse->format.last = parse->snippet.last;
        parse->stage = BTW_PARSE_STRING;
    } else if(parse->item->id == BTW_LEX_FORMAT_START) {
        parse->format.first = parse->format.last; //crap-code
            //printff("\nRECALLING");
        btw_parse_recall(btw, parse);
    } else if(parse->i + 1 >= vbtwlex_length(&btw->items)) {
            //printff("\nRECALLING");
        btw_parse_recall(btw, parse);
    }
}/*}}}*/

ErrDecl btw_parse_format(BtwFormat *fmt, Str *str) { //{{{
    ASSERT_ARG(fmt);
    ASSERT_ARG(str);
    //if(str_get_front(str) != '<') THROW("Expected a '<'");
    //if(str_get_back(str) != '>') THROW("Expected a '>'");
    fmt->skip_link = (str_ch(str, '!', 0) < str_length(str));
    fmt->bold = (str_ch(str, '*', 0) < str_length(str));
    fmt->italic = (str_ch(str, '/', 0) < str_length(str));
    fmt->underline = (str_ch(str, '_', 0) < str_length(str));
    size_t i0_fg = str_ch(str, '#', 0);
    size_t i0_bg = str_ch(str, '\\', 0);
    if(i0_fg < str_length(str)) {
        Str search = STR_I0(*str, i0_fg + 1);
        size_t iE = str_find_nany(&search, &STR("0123456789abcdefABCDEF"));
        //printff("Checked for valid fg: %zu / %zu", iE, str_length(&search));
        if(iE >= 6) {
            //printff("VALID FG [%.*s]", STR_F(&STR_LL(str_iter_begin(&search), 6)));
            fmt->color_fg = true;
            Str rd = STR_LL(str_iter_at(&search, 0), 2);
            Str gn = STR_LL(str_iter_at(&search, 2), 2);
            Str bl = STR_LL(str_iter_at(&search, 4), 2);
            int a = str_to_u8(&rd, &fmt->fg.red, 16);
            int b = str_to_u8(&gn, &fmt->fg.green, 16);
            int c = str_to_u8(&bl, &fmt->fg.blue, 16);
            if(a || b || c) THROW(ERR_UNREACHABLE);
        }
    }
    if(i0_bg < str_length(str)) {
        Str search = STR_I0(*str, i0_bg + 1);
        size_t iE = str_find_nany(&search, &STR("0123456789abcdefABCDEF"));
        //printff("Checked for valid bg: %zu / %zu", iE, str_length(&search));
        if(iE >= 6) {
            //printff("VALID BG [%.*s]", STR_F(&STR_LL(str_iter_begin(&search), 6)));
            fmt->color_bg = true;
            Str rd = STR_LL(str_iter_at(&search, 0), 2);
            Str gn = STR_LL(str_iter_at(&search, 2), 2);
            Str bl = STR_LL(str_iter_at(&search, 4), 2);
            int a = str_to_u8(&rd, &fmt->bg.red, 16);
            int b = str_to_u8(&gn, &fmt->bg.green, 16);
            int c = str_to_u8(&bl, &fmt->bg.blue, 16);
            if(a || b || c) THROW(ERR_UNREACHABLE);
        }
    }
    /* check which links I may have to do */
    Str splice = {0};
    //str_splice(
    return 0;
error:
    return -1;
} //}}}

#define ERR_btw_parse_link_end(...)     "failed parsing note end"
ErrDecl btw_parse_link_end(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    TRYC(btw_parse_text(btw, parse, &parse->link, 0));
    int err = 0;
    Str scratch = {0}; // TODO move into parse? -> less freeing
    info(INFO_parsing_found_link, F("Link:", FG_BK_B) "%.*s", STR_F(&parse->link));
    if(str_get_front(&parse->link) == '[') ++parse->link.first; /* TODO: this is stupid. should be assert or throw */
    if(str_get_back(&parse->link) == ']') --parse->link.last; /* TODO: this is stupid. should be assert or throw */
    str_trim(&parse->link);
    Str *parent = vsstr_get_back(&parse->notes);
    /* check format */
    if(str_length(&parse->fmt)) {
        //if(str_get_front(&parse->format) == '<') ++parse->format.first; /* TODO: this is stupid. should be assert or throw */
        //if(str_get_back(&parse->format) == '>') --parse->format.last; /* TODO: this is stupid. should be assert or throw */
        BtwFormat fmt = {0};
        TRYC(btw_parse_format(&fmt, &parse->fmt));
        printff("WITH FORMAT [%.*s]", STR_F(&parse->fmt));
        str_clear(&scratch);
        TRYC(str_fmt_fgbg(&scratch, &parse->link, fmt.color_fg ? &fmt.fg : 0, fmt.color_bg ? &fmt.bg : 0, fmt.bold, fmt.italic, fmt.underline));
        if(!fmt.skip_link) {
            /* add */
            printff("LINK [%.*s] .. [%.*s]", STR_F(parent), STR_F(&parse->link));
            TRYC(nexus_link(&parse->core, parent, &scratch, 0));
        }
        TRYC(nexus_add_text(&parse->core, parent, &scratch));
    } else {
        /* add */
        printff("LINK [%.*s] .. [%.*s]", STR_F(parent), STR_F(&parse->link));
        TRYC(nexus_link(&parse->core, parent, &parse->link, 0));
        TRYC(nexus_add_text(&parse->core, parent, &parse->link));
    }
    parse->fmt.first = parse->fmt.last; // clear format
    parse->link.first = parse->link.last;
clean:
    str_free(&scratch);
    return err;
error:
    ERR_CLEAN;
}/*}}}*/

#define ERR_btw_parse_note_begin(...)     "failed parsing note end"
ErrDecl btw_parse_note_begin(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
        if(parse->at_least_one_is_empty) return 0;
        /* was here -> moved down ??? */
        info(INFO_parsing_found_note, F("NoteBegin (%zu) @ %zu:", FG_MG_B) "%.*s", vsstr_length(&parse->notes), parse->snippet.first, STR_F(&parse->link));
        /* try adding to nexus */
        Str title = parse->link;
        if(str_get_front(&title) == '[') ++title.first; /* TODO this is stupid.. */
        if(str_get_back(&title) == ']') --title.last;
        str_trim(&title);
        parse->at_least_one_is_empty |= !(str_cmp(&title, &STR("")));
        //printff("AT LEAST ONE IS EMPTY: %s", parse->at_least_one_is_empty ? "TRUE" : "FALSE");
        if(parse->at_least_one_is_empty) return 0;
        TRYC(btw_parse_text(btw, parse, &parse->link, &parse->snippet));
        TRYC(nexus_create_if_nonexist(&parse->core, &title));
        if(vsstr_length(&parse->notes)) {
            Str *parent = vsstr_get_back(&parse->notes);
            //printff("LINK [%.*s] .. [%.*s]", STR_F(parent), STR_F(&title));
            TRYC(nexus_link(&parse->core, parent, &title, 0));
            //nexus_link();
        }
        /* moved this 2 lines down from above -- is it still correct ?? */
        vsstr_push_back(&parse->notes, &title);
        //TRYC(btw_parse_text(btw, parse, &parse->link, &parse->snippet));
        parse->stage = BTW_PARSE_STRING;
#if 0
        node.title = title;
        //printf("HELLO\n");
        if(str_length(&title) && !tnode_has(&btw->parsed.nodes, &node)) {
            info(parsing_create_note, "Creating Note: %.*s", STR_F(&title));
            //str_clear(&node.title);
            //TRYC(str_fmt(&node.title, "%.*s", STR_F(&parse->link)));
            TRYC(node_create(&node, &title, 0, 0));
            TRY(nexus_insert_node(&btw->parsed.nodes, &node), ERR_LUTD_ADD);
        }
#endif
    return 0;
error:
    return -1;
}/*}}}*/

#define ERR_btw_parse_note(...)     "failed parsing note end"
ErrDecl btw_parse_note(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    if(parse->item->id == BTW_LEX_SCOPE_START) {
        TRYC(btw_parse_note_begin(btw, parse));
    } else if(parse->item->id == BTW_LEX_WHITESPACE) {
        //printf("PENDING:[%.*s]\n", STR_F(&parse->pending));
        if(str_count_ch(&parse->pending, '\n') > 1) {
            parse->stage = BTW_PARSE_STRING;
            TRYC(btw_parse_link_end(btw, parse));
        }
    } else {
        //printf("PENDING:[%.*s]\n", STR_F(&parse->pending));
        //if(parse->item->id == BTW_LEX_WHITESPACE && str_count_ch(&parse->pending, '\n') > 1) {
        //    parse->stage = BTW_PARSE_STRING;
        //}
        TRYC(btw_parse_link_end(btw, parse));
        btw_parse_string(btw, parse);
    }
    return 0;
error:
    return -1;
}/*}}}*/

#define ERR_btw_parse_link_begin(...)     "failed parsing note end"
ErrDecl btw_parse_link_begin(Btw *btw, BtwParse *parse) {/*{{{*/
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    if(parse->item->id == BTW_LEX_LINK_START) {
        ++parse->stage_pair;
    } else if(parse->item->id == BTW_LEX_LINK_END) {
        --parse->stage_pair;
        if(!parse->stage_pair) {
            parse->link = parse->pending;
            parse->stage = BTW_PARSE_NOTE;
            TRYC(btw_parse_format_end(btw, parse));
        }
    } else if(parse->item->id == BTW_LEX_WHITESPACE) {
        if(str_count_ch(&parse->pending, '\n') > 0) {
            parse->stage = BTW_PARSE_STRING;
        }
    }
    return 0;
error:
    return -1;
}/*}}}*/


ErrDecl btw_parse_add(Nexus *nexus, Btw *btw, BtwParse *parse, BtwParseList id) {/*{{{*/
    ASSERT_ARG(nexus);
    ASSERT_ARG(btw);
    ASSERT_ARG(parse);
    switch(id) {
        case BTW_PARSE_LINK: {
        } break;
        case BTW_PARSE_NOTE: {
        } break;
        case BTW_PARSE_FORMAT: {
        } break;
        default: break;
    }
    return 0;
error:
    return -1;
}/*}}}*/

ErrDecl btw_parse(Nexus *nexus, Btw *btw) { //{{{
    ASSERT_ARG(nexus);
    ASSERT_ARG(btw);
    int err = 0;

    /* * * * * structure * * * * *
     * FORMAT : FORMAT_OPEN[2] ... FORMAT_CLOSE[1]
     * LINK   : { FORMAT } LINK_OPEN[1] ... LINK_CLOSE[2]
     * NOTE   : LINK WHITESPACE SCOPE_START[1] ... SCOPE_END[2]
     *
     * * * * * legend * * * * *
     * {}  : optional
     * [n] : order of matches to search
     * ... : literally anything
     *
     */

    BtwParse parse = {0};
    TRYC(btw_parse_init(btw, &parse));
    for(parse.i = 0; parse.i < vbtwlex_length(&btw->items); ++parse.i) {
        if(parse.i_prev > parse.i) { printff(F("RECALL HAPPENED", UL BOLD IT)); }
        parse.i_prev = parse.i;
        //printff("\nSTAGE [%u]", parse.stage);
        //printf("\n");printf("%.20s", &btw->content.s[btw->content.first + parse.snippet.first]);platform_getch();
        /* fetch next item */
        parse.item = vbtwlex_get_at(&btw->items, parse.i);
        parse.snippet.last = parse.item->iE;
        parse.pending.last = parse.item->iE;
        /* ... process ... */
        switch(parse.stage) {
            case BTW_PARSE_STRING: {
                btw_parse_string(btw, &parse);
            } break;
            case BTW_PARSE_LINK: {
                TRYC(btw_parse_link_begin(btw, &parse));
            } break;
            case BTW_PARSE_NOTE: {
                TRYC(btw_parse_note(btw, &parse));
            } break;
            case BTW_PARSE_FORMAT: {
                btw_parse_format_begin(btw, &parse);
            } break;
            default: break;
        }
        TRYC(btw_parse_note_end(btw, &parse));
        if(parse.quit) break;
        /* prepare for next parse.item */
        parse.snippet.first = parse.item->iE;
    } //printf("\n");
    TRYC(nexus_fuse(&nexus->core, &parse.core, &btw->stats.links));
clean:
    btw_parse_free(&parse);
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
    //vbtwlink_free(&btw->parse.refs);
    //vbtwlink_free(&btw->parse.titles);
    tnode_free(&btw->parsed.nodes);
    //vsize_free(&btw->parse.indices);
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
        info(INFO_directory, "directory '%.*s'", STR_F(filename));
    } else {
        bool parse = false;
        Str split = {0};
        while(split = str_splice(&nexus->config.extensions, &split, ','), split.first < str_length(&nexus->config.extensions)) {
            if(!str_cmp_ci(&btw->ext, &split)) {
                // TODO make a flag for this?
                parse = true;
                break;
            }
        }
        if(parse) {
            size_t size = 0;
            if(nexus->config.max_file_size && (size = file_size(filename)) <= nexus->config.max_file_size) {
                info(INFO_parsing_file, "parsing '%.*s'", STR_F(filename));
                TRYC(file_str_read(filename, &btw->content));
                str_trim(&btw->content);
            } else {
                info(INFO_parsing_skip_too_large, "file too large: %zu bytes, not parsing '%.*s'", size, STR_F(filename));
            }
        } else {
            info(INFO_parsing_skip_incorrect_extension, "incorrect extension '%.*s', not parsing '%.*s'", STR_F(&btw->ext), STR_F(filename));
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
    int err = 0;

    TRYC(btw_file_prepare(nexus, filename, btw));
    if(str_length(&btw->content)) {
        TRYC(btw_lex(&btw->items, &btw->content));
        TRYC(btw_parse(nexus, btw));
        info_check(INFO_parsing_file, true);
        ++btw->stats.success;
    }
    //return 0;
clean:
    btw_free(btw); // TODO: do I need to free here or not??? (asking myself in case I process multiple files in a row -> parse_exec )
    return err;
error:
    ERR_CLEAN;
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

