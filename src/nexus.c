#include "nexus.h"
#include "icon.h"
#include "node.h"
#include "search.h"
#include "cmd.h"
#include "content.h"
#include "str.h"
#include "btw.h"
#include "file.h"
#include "vector.h"

#define nexus_fmt_search_ERR(str, node) "failed searching node"
ErrDecl nexus_fmt_search(Str *str, Node *node) {
    ASSERT_ARG(str);
    ASSERT_ARG(node);
    TRYF(icons_fmt, str, &node->icons);
    TRYF(str_fmt, str, "%.*s %.*s %.*s", STR_F(&node->title), STR_F(&node->cmd), STR_F(&node->desc));
    return 0;
error:
    return -1;
}

#if PROC_COUNT /* local threading {{{ */
#include <pthread.h>

#define ThreadQueue(X)      \
    typedef struct X##ThreadQueue { \
        pthread_t id; \
        pthread_mutex_t mutex; \
        size_t i0; \
        size_t len; \
        struct X *q[PROC_COUNT]; \
    } X##ThreadQueue;

/* local thread: search {{{ */

#define SEARCH_THREAD_BATCH     16

typedef struct NexusThreadSearchJob {
    size_t i;
    size_t j;
} NexusThreadSearchJob;

ThreadQueue(NexusThreadSearch) /* {{{ */
typedef struct NexusThreadSearch {
    TNode *tnodes;
    Node *anchor;
    size_t human;
    Str cmd;
    Str content;
    Str *search;
    pthread_mutex_t *results_mutex;
    Node *results;
  NexusThreadSearchJob job[SEARCH_THREAD_BATCH];
  NexusThreadSearchThreadQueue *queue;
} NexusThreadSearch; /* }}} */

static void *nexus_static_thread_search(void *args) /* {{{ */
{
    NexusThreadSearch *arg = args;
    Node *anchor = arg->anchor;
    size_t sO = 0; //, sI = 0;
    if(anchor) {
        sO = vrnode_length(&anchor->outgoing);
        //sI = vrnode_length(&anchor->incoming);
    }

    /* thread processing / search */
    for(size_t ib = 0; ib < SEARCH_THREAD_BATCH; ib++) {
        size_t i = arg->job[ib].i;
        size_t j = arg->job[ib].j;
        if(SIZE_IS_NEG(i) || SIZE_IS_NEG(j)) continue;
        Node *node = 0;
        if(anchor) {
            if(j < sO) {
                node = vrnode_get_at(&anchor->outgoing, j);
            } else {
                node = vrnode_get_at(&anchor->incoming, j-sO);
            }
        } else {
            node = arg->tnodes->buckets[i].items[j];
        }
        str_clear(&arg->cmd);
        str_clear(&arg->content);
        TRYF(nexus_fmt_search, &arg->content, node);
        int found = search_nofree(true, &arg->cmd, arg->search, &arg->content);
        //IconStr iconstr = {0};
        //icon_fmt(iconstr, node->icon);
        //int found = search_fmt_nofree(true, &arg->cmd, &arg->content, arg->search, "%s %.*s %.*s %.*s", iconstr, STR_F(&node->title), STR_F(&node->cmd), STR_F(&node->desc));
        if(found) {
            VrNode *findings = &arg->results->outgoing;
            if(anchor) findings = j < sO ? &arg->results->outgoing : &arg->results->incoming;
            pthread_mutex_lock(arg->results_mutex);
            TRY(vrnode_push_back(findings, node), ERR_VEC_PUSH_BACK);
            pthread_mutex_unlock(arg->results_mutex);
        }
    }

clean:
    /* finished this thread .. make space for next thread */
    pthread_mutex_lock(&arg->queue->mutex);
    arg->queue->q[(arg->queue->i0 + arg->queue->len) % PROC_COUNT] = arg;
    ++arg->queue->len;
    pthread_mutex_unlock(&arg->queue->mutex);

    return 0;
error:
    goto clean;
} /* }}} */


/* }}} */

#endif /*}}}*/

int nexus_arg(Nexus *nexus, Arg *arg) /*{{{*/
{
    ASSERT(arg, ERR_NULL_ARG);
    ASSERT(nexus, ERR_NULL_ARG);
    nexus->args = arg;
    nexus->config.max_preview = arg->max_list;
    nexus->config.files = &arg->files;
    TRY(str_copy(&nexus->config.entry, &arg->entry), ERR_STR_COPY);
    switch(arg->view) {
        case SPECIFY_NONE:
        case SPECIFY_NORMAL: nexus->config.view = VIEW_NORMAL; break;
        case SPECIFY_SEARCH_ALL: nexus->config.view = VIEW_SEARCH_ALL; break;
        case SPECIFY_SEARCH_SUB: nexus->config.view = VIEW_SEARCH_SUB; break;
        case SPECIFY_ICON: nexus->config.view = VIEW_ICON; break;
        default: THROW(ERR_UNREACHABLE ", %u", arg->view);
    }
    switch(arg->show_preview) {
        case SPECIFY_YES: case SPECIFY_TRUE: {
            nexus->config.show_preview = true;
        } break;
        case SPECIFY_NO: case SPECIFY_FALSE: {
            nexus->config.show_preview = false;
        } break;
        default: THROW(ERR_UNREACHABLE ", %u", arg->view);
    }
    switch(arg->show_description) {
        case SPECIFY_YES: case SPECIFY_TRUE: {
            nexus->config.show_desc = true;
        } break;
        case SPECIFY_NO: case SPECIFY_FALSE: {
            nexus->config.show_desc = false;
        } break;
        default: THROW(ERR_UNREACHABLE ", %u", arg->view);
    }
    return 0;
error:
    return -1;
} /*}}}*/

#if 0
int nexus_create_by_icon(Nexus *nexus) /* {{{ */
{
    TRY(node_create(&nexus->nodeicon, &STR("Browse by icon"), 0, 0, ICON_ROOT), ERR_NODE_CREATE);
    TRY(tnodeicon_init(&nexus->nodesicon, 6), ERR_LUTD_INIT);
    for(size_t i = 0; i < (1ULL << (nexus->nodes.width - 1)); ++i) {
        size_t N = nexus->nodes.buckets[i].len;
        for(size_t j = 0; j < N; ++j) {
            Node *ref = nexus->nodes.buckets[i].items[j];
            Node reffind = *ref;
            if(reffind.icon >= 0) reffind.icon = ICON_DATE;
            if(!tnodeicon_has(&nexus->nodesicon, &reffind)) {
                Node niv = {0};
                TRY(node_create(&niv, &STR("Group"), 0, 0, reffind.icon), ERR_NODE_CREATE);
                TRY(tnodeicon_add(&nexus->nodesicon, &niv), ERR_LUTD_ADD);
            }
            size_t ii = 0, jj = 0;
            TRY(tnodeicon_find(&nexus->nodesicon, &reffind, &ii, &jj), "failed finding node with date '%zi'", ref->icon);
            Node *ni = nexus->nodesicon.buckets[ii].items[jj];
            ASSERT(ni, ERR_UNREACHABLE);
            TRY(vrnode_push_back(&ni->outgoing, ref), ERR_VEC_PUSH_BACK);
        }
    }

    VrNode *nio = &nexus->nodeicon.outgoing;
    for(size_t i = 0; i < (1ULL << (nexus->nodesicon.width - 1)); ++i) {
        size_t N = nexus->nodesicon.buckets[i].len;
        for(size_t j = 0; j < N; ++j) {
            Node *ni = nexus->nodesicon.buckets[i].items[j];
            vrnode_sort(&ni->outgoing);
            TRY(vrnode_push_back(nio, ni), ERR_VEC_PUSH_BACK);
        }
    }
    vrnode_sort(&nexus->nodeicon.outgoing);
    return 0;
error:
    return -1;
} /* }}} */
#endif

int nexus_init(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    TRY(tnode_init(&nexus->nodes, 12), ERR_LUTD_INIT);
    TRY(trnode_init(&nexus->icons, 8), ERR_LUTD_INIT);
    TRY(nexus_build(nexus, nexus->config.files), ERR_NEXUS_BUILD);
    tnode_sort_sub(&nexus->nodes);
    /* set up view */
    TRYF(str_fmt, &nexus->tags.title, "List of tags");
    View *view = &nexus->view;
    view->id = nexus->config.view;
    switch(view->id) {
        case VIEW_NORMAL: {
            Str *title = str_length(&nexus->config.entry) ? &nexus->config.entry : &STR(NEXUS_ROOT);
            TRY(!(view->current = nexus_get(nexus, title)), ERR_NEXUS_GET);
        } break;
        case VIEW_SEARCH_ALL: {
            view->edit = true;
            view->current = &nexus->findings;
        } break;
        case VIEW_SEARCH_SUB: {
            view->edit = true;
            view->current = &nexus->findings;
            Str *title = str_length(&nexus->config.entry) ? &nexus->config.entry : &STR(NEXUS_ROOT);
            TRY(!(view->search_on = nexus_get(nexus, title)), ERR_NEXUS_GET);
        } break;
        case VIEW_ICON: {
            view->current = &nexus->tags;
        } break;
        case VIEW_NONE: THROW("view id should not be NONE");
        default: THROW("unknown view id: %u", view->id);
    }
#if 0
    TRY(nexus_create_by_icon(nexus), "could not create by-icon view");
#endif

    return 0;
error:
    return -1;
} //}}}

void nexus_free(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    tnode_free(&nexus->nodes);
    trnode_free(&nexus->icons);
    //tnodeicon_free(&nexus->nodesicon);
    vview_free(&nexus->views);
    node_free(&nexus->findings);
    node_free(&nexus->tags);
    view_free(&nexus->view);
    //node_free(&nexus->nodeicon);
    str_free(&nexus->config.entry);
} //}}}

/* rebuild yourself {{{ */
#if defined(PLATFORM_LINUX) || defined(PLATFORM_CYGWIN)
#include <pthread.h>
#include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
#include <windows.h>
#endif

#define ERR_NEXUS_REBUILD "could not rebuild, press a key to resmume in current state"
void nexus_rebuild(Nexus *nexus)
{
    int err = 0;
    ASSERT(nexus, ERR_NULL_ARG);
    Str cmd = {0};
#if PROC_COUNT
    TRY(str_fmt(&cmd, "make -j %u", PROC_COUNT), ERR_STR_FMT)
#else
    TRY(str_fmt(&cmd, "make"), ERR_STR_FMT);
#endif
    int result = cmd_run(&cmd);
    Str args[5] = {0};
    if(result) {
        INFO(ERR_NEXUS_REBUILD);
        platform_getch();
        goto clean;
    }
    ViewList id = nexus->view.id;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_CYGWIN)
    Node *current = (id == VIEW_SEARCH_SUB) ? nexus->view.search_on : nexus->view.current;
    Str *title = current ? &current->title : &STR(NEXUS_ROOT);
    TRY(str_fmt(&args[0], "--entry=%.*s", STR_F(title)), ERR_STR_FMT);
    TRY(str_fmt(&args[1], "--view=%s", specify_str(nexus_current_view_arg(nexus))), ERR_STR_FMT);
    TRY(str_fmt(&args[2], "--show-preview=%s", nexus->config.show_preview ? "yes" : "no"), ERR_STR_FMT);
    TRY(str_fmt(&args[3], "--show-description=%s", nexus->config.show_desc ? "yes" : "no"), ERR_STR_FMT);
    TRY(str_fmt(&args[4], "--max-list=%zu", nexus->args->max_list), ERR_STR_FMT);
    if(!(id == VIEW_NORMAL || id == VIEW_SEARCH_SUB)) str_clear(&args[0]);
    printf("%s %.*s %.*s %.*s %.*s %.*s\n", nexus->args->name, STR_F(&args[0]), STR_F(&args[1]), STR_F(&args[2]), STR_F(&args[3]), STR_F(&args[4]));
    char *const argv[] = {(char *)nexus->args->name,
        str_length(&args[0]) ? str_iter_begin(&args[0]) : "",
        str_length(&args[1]) ? str_iter_begin(&args[1]) : "",
        str_length(&args[2]) ? str_iter_begin(&args[2]) : "",
        str_length(&args[3]) ? str_iter_begin(&args[3]) : "",
        str_length(&args[4]) ? str_iter_begin(&args[4]) : "",
        0};
    execv(nexus->args->name, argv);
clean:
    for(size_t i = 0; i < sizeof(args)/sizeof(*args); ++i) {
        str_free(&args[i]);
    }
#elif defined(PLATFORM_WINDOWS)
    Node *current = id == VIEW_SEARCH_SUB ? nexus->view.search_on : nexus->view.current;
    Str args = {0};
    if(id == VIEW_NORMAL || id == VIEW_SEARCH_SUB) {
        TRY(str_fmt(&args, "--entry=\"%.*s\"", STR_F(&current->title)), ERR_STR_FMT);
    }
    TRY(str_fmt(&args, "--view=%s", specify_str(nexus_current_view_arg(nexus))), ERR_STR_FMT);
    TRY(str_fmt(&args, "--show-preview=%s", nexus->config.show_preview ? "yes" : "no"), ERR_STR_FMT);
    TRY(str_fmt(&args, "--show-description=%s", nexus->config.show_desc ? "yes" : "no"), ERR_STR_FMT);
    TRY(str_fmt(&args, "--max-list=%zu", nexus->args->max_list), ERR_STR_FMT);
    STARTUPINFO info_startup = {0};
    PROCESS_INFORMATION info_process = {0};
    LPCTSTR c = nexus->args->name;
    LPCTSTR c2 = str_iter_begin(&args);
    result = CreateProcess(c, c2, 0, 0, FALSE, 0, 0, 0, &info_startup, &info_process);
    if(result) THROW(ERR_NEXUS_REBUILD);
    CloseHandle(info_process.hProcess);
    CloseHandle(info_process.hThread);
clean:
    str_free(&args);
#else
clean:
    THROW("rebuild not yet implemented on '%s'", PLATFORM_NAME);
#endif
    str_free(&cmd);
    if(err) exit(0);
    return;
error:
    platform_getch();
    ERR_CLEAN;
}
/* }}} */

int nexus_userinput(Nexus *nexus, int key) /*{{{*/
{
    ASSERT(nexus, ERR_NULL_ARG);
    int err = 0;
    View *view = &nexus->view;
    bool disable_default = false;
    ASSERT(view, "view is 0!\n");
    Str reenter = {0};
    switch(view->id) {
        case VIEW_NORMAL: {
        } break;
        case VIEW_SEARCH_SUB: // fallthrough
        case VIEW_SEARCH_ALL: {
            if(view->edit) {
                disable_default = true;
            }
        } break;
        case VIEW_ICON: {
        } break;
        default: THROW("unknown view id: %u", view->id);
    }
    if(!disable_default) {
        switch(key) {
            case ' ': { nexus->config.show_desc ^= true; } break; /* TODO: this is stupid. make it so that each view has the show_desc, show_preview etc. saved for itself... */
            case 'i': { nexus->config.show_preview ^= true; } break;
            case 'Q': { nexus_rebuild(nexus); } break;
            case 'q': { nexus->quit = true; } break;
            case 'h': { TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK); } break;
            case 'j': { node_set_sub(view->current, &view->sub_sel, view->sub_sel + 1); } break;
            case 'k': { node_set_sub(view->current, &view->sub_sel, view->sub_sel - 1); } break;
            case 'l': {
                TRY(nexus_change_view(nexus, view, VIEW_NORMAL), ERR_NEXUS_CHANGE_VIEW);
                TRY(nexus_follow_sub(nexus, view), ERR_NEXUS_FOLLOW_SUB);
            } break;
            case 'H': {
                do {
                    TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK);
                } while(!(view->id == VIEW_SEARCH_ALL || view->id == VIEW_SEARCH_SUB) && vview_length(&nexus->views));
            } break;
            case 'c': { cmd_run(&view->current->cmd); } break;
            case 'C': {
                Node *sub = node_get_sub_sel(view->current, view->sub_sel);
                if(sub) cmd_run(&sub->cmd);
            } break;
            case 'r': {
                TRYF(str_copy, &reenter, &nexus->view.current->title);
                nexus_free(nexus);
                nexus->config.entry = reenter;
                str_zero(&reenter);
                TRYF(nexus_init, nexus);
            } break;
                      /* TODO : jump to random note! */
            default: break;
        }
    }
    switch(view->id) {
        case VIEW_NORMAL: {
            switch(key) {
                case 't': { TRY(nexus_change_view(nexus, view, VIEW_ICON), ERR_NEXUS_CHANGE_VIEW); } break;
                case 'f': { TRY(nexus_change_view(nexus, view, VIEW_SEARCH_ALL), ERR_NEXUS_CHANGE_VIEW); } break;
                case 'F': { TRY(nexus_change_view(nexus, view, VIEW_SEARCH_SUB), ERR_NEXUS_CHANGE_VIEW); } break;
                default: break;
            }
        } break;
        case VIEW_SEARCH_SUB: // fallthrough
        case VIEW_SEARCH_ALL: {
            size_t len_search = str_length(&view->search);
            if(view->edit) {
                if(key >= 0x20 && key != 127) {
                    TRY(str_fmt(&view->search, "%c", key), ERR_STR_FMT);
                } else if(key == 127) {
                    if(len_search) str_pop_back_char(&view->search);
                } else if(key == 8) {
                    str_pop_back_word(&view->search);
                } else if(key == '\n' || key == 27) {
                    view->edit = false;
                }
            } else {
                switch(key) {
                    case '\n': { view->edit = true; } break;
                    case 't': { TRY(nexus_change_view(nexus, view, VIEW_ICON), ERR_NEXUS_CHANGE_VIEW); } break;
                    case 'f': { view->edit = true; } break;
                    case 'F': {
                        str_clear(&view->search);
                        view->edit = true;
                    } break;
                    case 27: { TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK); } break;
                    default: break;
                }
            }
            /* post processing */
            if(len_search != str_length(&view->search)) {
                nexus->findings_updated = false;
            }

        } break;
        case VIEW_ICON: {
            switch(key) {
                case 'f': { TRY(nexus_change_view(nexus, view, VIEW_SEARCH_ALL), ERR_NEXUS_CHANGE_VIEW); } break;
                case 'F': { TRY(nexus_change_view(nexus, view, VIEW_SEARCH_SUB), ERR_NEXUS_CHANGE_VIEW); } break;
                case 't':
                case 27: { TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK); } break;
                default: break;
            }
        } break;
        default: THROW("unknown view id: %u", view->id);
    }
clean:
    str_free(&reenter);
    return err;
error:
    ERR_CLEAN;
} /*}}}*/

Node *nexus_get(Nexus *nexus, Str *title) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(title, ERR_NULL_ARG);
    Node *result = 0;
    size_t i0 = 0, j0 = 0;
    Node find = {0};
    TRY(node_create(&find, title, 0, 0, 0), ERR_NODE_CREATE);
    if(tnode_find(&nexus->nodes, &find, &i0, &j0)) {
        Node *alternative = 0;
        INFO("node does not exist in nexus: '%.*s'", STR_F(&find.title));
        for(i0 = 0; i0 < (1ULL << (nexus->nodes.width - 1)); ++i0) {
            for(j0 = 0; j0 < nexus->nodes.buckets[i0].len; ++j0) {
                alternative = nexus->nodes.buckets[i0].items[j0];
                goto alt;
            }
        }
alt:
        if(!alternative) THROW("could not find any node in nexus");
    }
    result = nexus->nodes.buckets[i0].items[j0];
clean:
    node_free(&find);
    return result;
error:
    goto clean;
} //}}}

int nexus_search(Nexus *nexus, Node *anchor, Str *search, Node *results) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(search, ERR_NULL_ARG);
    ASSERT(results, ERR_NULL_ARG);
    int err = 0;

#if PROC_COUNT /* {{{ */

    /* declarations */
    TNode *tnodes = &nexus->nodes;
    NexusThreadSearch thr_search[PROC_COUNT] = {0};
    NexusThreadSearchThreadQueue thr_queue = {0};
    NexusThreadSearchJob job[SEARCH_THREAD_BATCH] = {0};
    size_t job_counter = 0;
    pthread_attr_t thr_attr;
    pthread_mutex_t results_mutex;

    /* set up */
    vrnode_clear(&results->outgoing);
    vrnode_clear(&results->incoming);
    pthread_mutex_init(&thr_queue.mutex, 0);
    pthread_mutex_init(&results_mutex, 0);
    for(size_t i = 0; i < PROC_COUNT; ++i) {
        thr_search[i].results = results;
        thr_search[i].results_mutex = &results_mutex;
        thr_search[i].tnodes = tnodes;
        thr_search[i].queue = &thr_queue;
        thr_search[i].human = i;
        thr_search[i].search = search;
        thr_search[i].anchor = anchor;
        /* add to queue */
        thr_queue.q[i] = &thr_search[i];
        ++thr_queue.len;
    }
    pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_DETACHED);
    assert(thr_queue.len <= PROC_COUNT);

    /* search */
    size_t len_t = (1ULL << (tnodes->width - 1));
    size_t last_t = 0;
    size_t sO = 0, sI = 0;
    if(anchor) {
        len_t = 1;
        sO = vrnode_length(&anchor->outgoing);
        sI = vrnode_length(&anchor->incoming);
    } else {
        for(size_t i = 0; i < len_t; i++) {
            if(tnodes->buckets[i].len) last_t = i;
        }
    }
    for(size_t i = 0; i < len_t; ++i) {
        size_t len = anchor ? sO+sI : tnodes->buckets[i].len;
        for(size_t j = 0; j < len || job_counter == SEARCH_THREAD_BATCH; ++j) {
            if(job_counter < SEARCH_THREAD_BATCH) {
                /* set ub jobs */
                if(job_counter == 0) {
                    memset(&job, 0xFF, sizeof(job));
                }
                if(job_counter < SEARCH_THREAD_BATCH) {
                    job[job_counter].i = i;
                    job[job_counter].j = j;
                    ++job_counter;
                }
                if(j + 1 == len && i == last_t) {
                    job_counter = SEARCH_THREAD_BATCH;
                }
            } else {
                /* this section is responsible for starting the thread */
                pthread_mutex_lock(&thr_queue.mutex);
                if(thr_queue.len) {
                    NexusThreadSearch *thr = thr_queue.q[thr_queue.i0];
                    /* load job */
                    memcpy(thr->job, &job, sizeof(job));
                    job_counter = 0;
                    /* create thread */
                    pthread_create(&thr->queue->id, &thr_attr, nexus_static_thread_search, thr);
                    thr_queue.i0 = (thr_queue.i0 + 1) % PROC_COUNT;
                    --thr_queue.len;
                } else {
                    --j;
                }
                pthread_mutex_unlock(&thr_queue.mutex);
            }

        }
    }

    /* wait until all threads finished */
    for(;;) {
        pthread_mutex_lock(&thr_queue.mutex);
        if(thr_queue.len == PROC_COUNT) {
            pthread_mutex_unlock(&thr_queue.mutex);
            break;
        }
        pthread_mutex_unlock(&thr_queue.mutex);
    }
    /* now free since we *know* all threads finished, we can ignore usage of the lock */
    for(size_t i = 0; i < PROC_COUNT; ++i) {
        if(thr_search[i].queue->id) {
            str_free(&thr_search[i].content);
            str_free(&thr_search[i].cmd);
        }
    }

    return err;

    /* }}} */
#else /* is active when : PROC_COUNT == 0 {{{*/

    vrnode_clear(&results->outgoing);
    vrnode_clear(&results->incoming);
    VrNode *findings = &results->outgoing;
    TNode *tnodes = &nexus->nodes;
    Str cmd = {0}, content = {0};
    size_t len_t = (1ULL << (tnodes->width - 1));
    size_t sO = 0, sI = 0;
    if(anchor) {
        len_t = 1;
        sO = vrnode_length(&anchor->outgoing);
        sI = vrnode_length(&anchor->incoming);
    }
    for(size_t i = 0; i < len_t; i++) {
        size_t len = anchor ? sO+sI : tnodes->buckets[i].len;
        for(size_t j = 0; j < len; j++) {
            Node *node = 0;
            if(anchor) {
                if(j < sO) {
                    node = vrnode_get_at(&anchor->outgoing, j);
                } else {
                    node = vrnode_get_at(&anchor->incoming, j-sO);
                }
            } else {
                node = tnodes->buckets[i].items[j];
            }
            str_clear(&cmd);
            str_clear(&content);
            //IconStr iconstr = {0};
            //icon_fmt(iconstr, node->icon);
            TRYF(nexus_fmt_search, &content, node);
            int found = search_nofree(true, &cmd, search, &content);
            //int found = search_fmt_nofree(true, &cmd, &content, search, "%s %.*s %.*s %.*s", iconstr, STR_F(&node->title), STR_F(&node->cmd), STR_F(&node->desc));
            if(found) {
                if(anchor && !(j < sO)) findings = &results->incoming;
                TRY(vrnode_push_back(findings, node), ERR_VEC_PUSH_BACK);
            }
        }
    }
clean:
    str_free(&cmd);
    str_free(&content);
    return err;
error:
    ERR_CLEAN;

#endif /*}}}*/

} //}}}

ErrDecl nexus_tag_node(Nexus *nexus, Node *node, Node *temp, IconBundle icon) {/*{{{*/
    ASSERT_ARG(nexus);
    ASSERT_ARG(node);
    /* TODO: maybe ... not make this a ... throw ... but ... like ... do it proper */
    if(node->icons.len >= ICON_BUNDLE_MAX) {
        return 0;
        THROW("too many icons: %u/%u", node->icons.len, ICON_BUNDLE_MAX);
    }
    /* do some tagging */
    IconBundle *ib = &node->icons.items[node->icons.len++];
    switch(icon.id) {
        case ICON_BUNDLE_NONE: break;
        case ICON_BUNDLE_STR: {
            TRYF(str_fmt, &ib->str, "%.*s", STR_F(&icon.str));
            INFO("Add icon %.*s ... %.*s", STR_F(&ib->str), STR_F(&node->title));
        } break;
        case ICON_BUNDLE_TIME: {
            ib->time = icon.time;
            INFO("Add icon %s ... %.*s", icon_str(ib->time), STR_F(&node->title));
        } break;
        default: THROW("unknown id: %u", icon.id);
    }
    ib->id = icon.id;
    /* TODO maybe don't exit early here, but change nexus->tags to a pointer and
     * point to the node here,... I've written some long text here before, things
     * happened and now the text is gone, I think I'll remember what I mean
     * when I stumble across this comment in 2 weeks (I'll probably forget, eh,
     * whatever. the text is about the same length now)
     */
    if(icon.id == ICON_BUNDLE_TIME && icon.time == ICON_TAG) return 0;
    /* do some linking */
    size_t ii, jj;
    str_clear(&temp->title);
    TRYF(icon_fmt_tag, &temp->title, icon);
    if(!str_length(&temp->title)) return 0;
    bool found = !tnode_find(&nexus->nodes, temp, &ii, &jj); /* TODO: this is not a reliable way to check if an icon already exists or not. */
    bool istag = trnode_has(&nexus->icons, temp);
    if(!found) {
        TRY(tnode_add(&nexus->nodes, temp), ERR_LUTD_ADD);
    }
    printf("ICONFIND '%.*s' -> %s\n", STR_F(&temp->title), found ? "found" : "new node");
    printf("ICONFIND '%.*s' -> %s\n", STR_F(&temp->title), istag ? "istag" : "new icon");
    TRY(tnode_find(&nexus->nodes, temp, &ii, &jj), ERR_LUTD_FIND ": '%.*s'", STR_F(&temp->title));
    Node *iconfound = nexus->nodes.buckets[ii].items[jj];
    //printf("%zu/%zu\n", ii, jj);
    //printf("%.*s is %s\n", STR_F(&temp->title), istag ? "a tag!" : "no tag");
    if(!found || !istag) {
        IconBundle ciscool = (IconBundle){.id = ICON_BUNDLE_TIME, .time = ICON_TAG};
        TRYF(nexus_tag_node, nexus, iconfound, temp, ciscool); /* dangerous !*/
    }
    if(!istag || !found) {
        //printf("ADD %.*s TO TAGS!\n", STR_F(&iconfound->title));
        TRY(trnode_add(&nexus->icons, iconfound), ERR_LUTD_ADD);
        //if(!(icon.id == ICON_BUNDLE_TIME && icon.time == ICON_TAG)) {
        TRY(vrnode_push_back(&nexus->tags.outgoing, iconfound), ERR_VEC_PUSH_BACK);
        //}
    }
    if(found) {
        str_clear(&temp->title); // TODO this is a bit whack (clearing AFTER we find?)
    } else {
        str_zero(&temp->title);
    }
    if(!str_length(&iconfound->title)) THROW("i don't want to think about what's better right now; return 0 or throw?"); // TODO
    //INFO("Added Icon %.*s ... %.*s", STR_F(&iconfound->title), STR_F(&node->title));
    TRYF(nexus_link, nexus, iconfound, node);
    return 0;
error:
    return -1;
}/*}}}*/

int nexus_insert_node(Nexus *nexus, Node **ref, Str *title, Str *cmd, Str *desc, VIcon *icons) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(ref);
    ASSERT_ARG(title);
    ASSERT_ARG(icons);
    //ASSERT(desc, ERR_NULL_ARG);
    //ASSERT(cmd, ERR_NULL_ARG);
    int err = 0;
    size_t i = 0, j = 0;
    Node iconfind = {0}; // TODO this is ugly
    Node find = {
        .title = *title
    };
    bool found = !tnode_find(&nexus->nodes, &find, &i, &j);
    if(found) {
        if(nexus->nodes.buckets[i].count[j]) {
            THROW("should not insert node with equal title '%.*s'", STR_F(title));
        } else {
            /* node was added in nexus_link, via. add_count(0), meaning we should set the proper description etc. */
            Node *node = nexus->nodes.buckets[i].items[j];
            str_free(&node->title); /* TODO this is sketchy */
            VrNode in = node->incoming, out = node->outgoing;
            TRY(node_create(node, title, cmd, desc, icons), ERR_NODE_CREATE);
            node->incoming = in;
            node->outgoing = out;
        }
    } else {
        Node node;
        TRY(node_create(&node, title, cmd, desc, icons), ERR_NODE_CREATE);
        TRY(tnode_add(&nexus->nodes, &node), ERR_LUTD_ADD);
        TRY(tnode_find(&nexus->nodes, &find, &i, &j), ERR_LUTD_FIND ": '%.*s'", STR_F(&find.title));
    }
    *ref = nexus->nodes.buckets[i].items[j];
    /* icon stuff */
    //printf("icons : %u for %.*s\n", icons->len, STR_F(&(*ref)->title));
    for(int i = 0; i < ((icons->len < ICON_BUNDLE_MAX) ? icons->len : ICON_BUNDLE_MAX); ++i) {
        TRYF(nexus_tag_node, nexus, *ref, &iconfind, icons->items[i]);
    }
clean:
    node_free(&iconfind);
    return err;
error:
    ERR_CLEAN;
} //}}}

int nexus_link(Nexus *nexus, Node *src, Node *dest) //{{{
{
    ASSERT_ARG(nexus);
    ASSERT_ARG(src);
    ASSERT_ARG(dest);
    if(!tnode_has(&nexus->nodes, src)) {
        Node temp;
        TRY(node_copy(&temp, src), ERR_NODE_COPY);
#if 0
        temp.icon = ICON_NONE;
#else
#endif
        TRY(tnode_add_count(&nexus->nodes, &temp, 0), ERR_LUTD_ADD);
        //THROW("node does not exist in nexus: '%.*s'", STR_F(&src->title));
    }
    if(!str_cmp_ci(&src->title, &dest->title)) return 0;
    if(!tnode_has(&nexus->nodes, dest)) {
        Node temp;
        TRY(node_copy(&temp, dest), ERR_NODE_COPY);
#if 0
        temp.icon = ICON_NONE;
#else
#endif
        TRY(tnode_add_count(&nexus->nodes, &temp, 0), ERR_LUTD_ADD);
        //THROW("node does not exist in nexus: '%.*s'", STR_F(&dest->title));
    }
    size_t i0 = 0, i1 = 0, j0 = 0, j1 = 0;
    TRY(tnode_find(&nexus->nodes, src, &i0, &j0), "couldn't find '%.*s'", STR_F(&src->title));
    TRY(tnode_find(&nexus->nodes, dest, &i1, &j1), "couldn't find '%.*s'", STR_F(&src->title));
    Node *ev_src = nexus->nodes.buckets[i0].items[j0];
    Node *ev_dest = nexus->nodes.buckets[i1].items[j1];
#if 0 // TODO: sort by icons!!!
    Icon i_src = ev_src->icon;
    Icon i_dest = ev_dest->icon;
    if(i_src == i_dest) {
        // ...
    } else if(i_src > i_dest) {
        Node *temp = ev_src;
        ev_src = ev_dest;
        ev_dest = temp;
    }
#endif
    /* check for duplicates - we should be fine to only check one half */
    bool duplicate = false;
    for(size_t i = 0; i < vrnode_length(&ev_src->outgoing); ++i) {
        Node *node = vrnode_get_at(&ev_src->outgoing, i);
        if(!str_cmp_ci(&node->title, &ev_dest->title)) {
            duplicate = true;
            break;
        }
    }
#if 0
    for(size_t i = 0; i < vrnode_length(&ev_dest->incoming); ++i) {
        Node *node = vrnode_get_at(&ev_dest->incoming, i);
        if(!str_cmp_ci(&node->title, &ev_src->title)) {
            duplicate = true;
            break;
        }
    }
#endif
    /* finally, add the nodes */
    if(!duplicate) {
        TRY(vrnode_push_back(&ev_src->outgoing, ev_dest), ERR_VEC_PUSH_BACK);
        TRY(vrnode_push_back(&ev_dest->incoming, ev_src), ERR_VEC_PUSH_BACK);
    }
    return 0;
error:
    return -1;
} //}}}

int nexus_follow_sub(Nexus *nexus, View *view) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(view, ERR_NULL_ARG);
    Node *current = view->current;
    ASSERT(current, ERR_NULL_ARG);
    size_t sub_sel = view->sub_sel;
    size_t sO = vrnode_length(&current->outgoing);
    size_t sI = vrnode_length(&current->incoming);
    Node *result = current;
    if(sub_sel >= sO+sI) sub_sel = sO+sI - 1;
    if(sub_sel < sO) {
        result = vrnode_get_at(&current->outgoing, sub_sel);
    } else if(sub_sel - sO < sI) {
        result = vrnode_get_at(&current->incoming, sub_sel - sO);
    } else if(sO+sI) {
        //THROW("sub_index '%zu' too large", sub_sel);
    }
    view->current = result;
    view->sub_sel = 0;
    return 0;
} //}}}

int nexus_change_view(Nexus *nexus, View *view, ViewList id) /*{{{*/
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(view, ERR_NULL_ARG);
    if(nexus->views.cap > nexus->views.last) {
        view_free(&nexus->views.items[nexus->views.last]);
    }
    View ref;
    TRY(view_copy(&ref, view), ERR_VIEW_COPY);
    TRY(vview_push_back(&nexus->views, ref), ERR_VEC_PUSH_BACK);
    /* check history if we maybe have one item to use */
    //ViewList id_post = VIEW_NONE;
    /* init view to be changed into */
    view_free(view);
    memset(view, 0, sizeof(*view));
    view->current = ref.current;
    view->sub_sel = ref.sub_sel;
    view->id = id;
    /* init the different views */
    switch(id) {
        case VIEW_NORMAL: {
            view->edit = false;
        } break;
        case VIEW_SEARCH_ALL: {
            view->sub_sel = 0;
            //if(id_post != VIEW_SEARCH_ALL) {
                str_clear(&view->search);
                view->edit = true;
            //}
            view->current = &nexus->findings;
            nexus->findings_updated = false;
        } break;
        case VIEW_SEARCH_SUB: {
            ASSERT(ref.id == VIEW_NORMAL || ref.id == VIEW_ICON, "Cannot search for id %i, maybe we don't have a node to search off of!", ref.id);
            view->sub_sel = 0;
            view->search_on = ref.current;
            //if(id_post != VIEW_SEARCH_ALL) {
                str_clear(&view->search);
                view->edit = true;
            //}
            view->current = &nexus->findings;
            nexus->findings_updated = false;
        } break;
        case VIEW_ICON: {
            view->sub_sel = 0;
            view->current = &nexus->tags;
        } break;
        case VIEW_NONE: THROW("view id should not be NONE");
        default: THROW("unknown view id: %u", id);
    }
    return 0;
error:
    return -1;
} /*}}}*/

int nexus_history_back(Nexus *nexus, View *view) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(view, ERR_NULL_ARG);
    nexus->findings_updated = false;
    if(vview_length(&nexus->views)) {
        view_free(view);
        View ref;
        vview_pop_back(&nexus->views, &ref);
        TRY(view_copy(view, &ref), ERR_VIEW_COPY);
    }
    return 0;
error:
    return -1;
} //}}}

int nexus_build(Nexus *nexus, VsStr *files) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(files, ERR_NULL_ARG);
    int err = 0;
    Str filename = {0};
    Btw btw = {0};
    if (!vsstr_length(files)) {
        Node *root;
        IconBundle iconb = {.id = ICON_BUNDLE_TIME, .time = ICON_ROOT};
        VIcon rooticons = {.items = {iconb}, .len = 1}; //{.items = &iconb, .len = 1};
        TRY(nexus_insert_node(nexus, &root, &STR(NEXUS_ROOT), CMD_NONE, &STR("Welcome to " F("c-nexus", BOLD) "\n\n"
                    F("basic controls", UL) "\n"
                    "  h : back in history\n"
                    "  j : move arrow down\n"
                    "  k : move arrow up\n"
                    "  l : follow the arrow\n\n"
                    "more can be found in the " F("controls wiki", UL)), &rooticons), ERR_NEXUS_INSERT_NODE);

        NEXUS_INSERT(nexus, root, NODE_LEAF, ICON_WIKI, CMD_NONE, "Test!", "This is proof that I can link to a note, even if it gets created in the future", "Note yet to be created", "shit");
        NEXUS_INSERT(nexus, root, NODE_LEAF, ICON_WIKI, CMD_NONE, "Note yet to be created", "This note is created after Test!", NODE_LEAF);
        NEXUS_INSERT(nexus, root, NODE_LEAF, ICON_WIKI, CMD_NONE, "Shit", "This note is created after Test!", NODE_LEAF);

        //TRY(nexus_insert_node(nexus, &root, &STR("🍃"), CMD_NONE, &STR("Welcome to " F("c-nexus", BOLD) "\n\n"

        TRY(content_build(nexus, root), ERR_CONTENT_BUILD);
    } else {
        BtwExec exec_args = {.nexus = nexus, .btw = &btw};
        for(size_t i = 0; i < vsstr_length(files); ++i) {
            Str *filename = vsstr_get_at(files, i);
            TRYF(file_exec, filename, &btw.dirfiles, btw_parse_exec, &exec_args);
            //TRYF(file_exec, filename, &btw.dirfiles, btw_parse_exec, 0);
            //TRYF(btw_parse_file, nexus, filename, &btw);
#if 0
            char *ext = strrchr(file->s, '.');
            if((ext && (ext - file->s > 0)) || !ext) {
                Str base = STR_LL(file->s, ext ? ext - file->s : file->last);
                Str content = {0};
                TRY(file_str_read(file, &content), ERR_FILE_STR_READ);
                str_trim(&content);
                Node *node = 0;
                printf("process: %.*s [%zu]\n", STR_F(&base), str_length(&base));
                TRY(nexus_insert_node(nexus, &node, &base, CMD_NONE, &content, ICON_NONE), ERR_NEXUS_INSERT_NODE);
                str_free(&content);
            } else {
                THROW("can't operate on hidden files");
            }
#endif
        }
#if 1
        while(vstr_length(&btw.dirfiles)) {
        //for(size_t i = 0; i < vstr_length(&btw.dirfiles); ++i) {
            //Str *filename = vstr_get_at(&btw.dirfiles, i);
            vstr_pop_back(&btw.dirfiles, &filename);
            memset(btw.dirfiles.items[vstr_length(&btw.dirfiles)], 0, sizeof(Str));
            TRYF(file_exec, &filename, &btw.dirfiles, btw_parse_exec, &exec_args);
            str_free(&filename);
#else
        while(vstr_length(&btw.dirfiles)) {
            Str filename = {0};
            vstr_pop_front(&btw.dirfiles, &filename);
            TRYF(file_exec, &filename, &btw.dirfiles, btw_parse_exec, &exec_args);
            if(!((++btw.direxec) % 256)) {
                vstr_shrink(&btw.dirfiles);
            }

#endif
            ++btw.stats.direxec;
            size_t res = vstr_reserved(&btw.dirfiles);
            if(res > btw.stats.maxres) btw.stats.maxres = res;
            //printf("%zu bytes max. reserved\n", btw.maxres);
            //if(!(btw.direxec % 64)) {
            //vstr_shrink(&btw.dirfiles);
            //}
            //TRYF(btw_parse_file, nexus, &filename, &btw);
#if 0
            if(!(n % 256)) {
                vstr_shrink(&btw.dirfiles);
            }
#endif
        }
        size_t res = vstr_reserved(&btw.dirfiles);
        if(res > btw.stats.maxres) btw.stats.maxres = res;
        //printf("%zu bytes max. reserved\n", btw.maxres);
        //printf("read %u files\n", n);
        INFO("Loaded %zu of %zu checked files", btw.stats.success, btw.stats.attempts);
        //getchar();
    }
    /* trim all descriptions */
    for(size_t i = 0; i < (1ULL << (nexus->nodes.width - 1)); ++i) {
        for(size_t j = 0; j < nexus->nodes.buckets[i].len; ++j) {
            Node *node = nexus->nodes.buckets[i].items[j];
            str_trim(&node->desc);
        }
    }

clean:
    str_free(&filename);
    btw_free(&btw);
    return err;
error:
    ERR_CLEAN;
} //}}}

int nexus_current_view_arg(Nexus *nexus) /* {{{ */
{
    ViewList id = nexus->view.id;
    switch(id) {
        case VIEW_NORMAL: return SPECIFY_NORMAL;
        case VIEW_ICON: return SPECIFY_ICON;
        case VIEW_SEARCH_ALL: return SPECIFY_SEARCH_ALL;
        case VIEW_SEARCH_SUB: return SPECIFY_SEARCH_SUB;
        default: ABORT("can't translate view id (%i) to argument view id! perhaps it's argument's behavior is missing! (this is stupid)", id); return SPECIFY_NONE; /* tcc warns me if I don't have this */
    }
} /* }}} */

