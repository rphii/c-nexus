#include "nexus.h"
#include "node.h"
#include "search.h"
#include "cmd.h"
#include "content.h"

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
        IconStr iconstr = {0};
        icon_fmt(iconstr, node->icon);
        int found = search_fmt_nofree(true, &arg->cmd, &arg->content, arg->search, "%s %.*s %.*s %.*s", iconstr, STR_F(&node->title), STR_F(&node->cmd), STR_F(&node->desc));
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

int nexus_create_by_icon(Nexus *nexus) /* {{{ */
{
    TRY(node_create(&nexus->nodeicon, "Browse by icon", 0, 0, ICON_ROOT), ERR_NODE_CREATE);
    TRY(tnodeicon_init(&nexus->nodesicon, 6), ERR_LUTD_INIT);
    for(size_t i = 0; i < (1ULL << (nexus->nodes.width - 1)); ++i) {
        size_t N = nexus->nodes.buckets[i].len;
        for(size_t j = 0; j < N; ++j) {
            Node *ref = nexus->nodes.buckets[i].items[j];
            Node reffind = *ref;
            if(reffind.icon >= 0) reffind.icon = ICON_DATE;
            if(!tnodeicon_has(&nexus->nodesicon, &reffind)) {
                Node niv = {0};
                TRY(node_create(&niv, "Group", 0, 0, reffind.icon), ERR_NODE_CREATE);
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

int nexus_init(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    TRY(tnode_init(&nexus->nodes, 12), ERR_LUTD_INIT);
    TRY(nexus_build(nexus), ERR_NEXUS_BUILD);
    /* set up view */
    View *view = &nexus->view;
    view->id = nexus->config.view;
    switch(view->id) {
        case VIEW_NORMAL: {
            char *title = str_length(&nexus->config.entry) ? str_iter_begin(&nexus->config.entry) : NEXUS_ROOT;
            TRY(!(view->current = nexus_get(nexus, title)), ERR_NEXUS_GET);
        } break;
        case VIEW_SEARCH_ALL: {
            view->edit = true;
            view->current = &nexus->findings;
        } break;
        case VIEW_SEARCH_SUB: {
            view->edit = true;
            view->current = &nexus->findings;
            char *title = str_length(&nexus->config.entry) ? str_iter_begin(&nexus->config.entry) : NEXUS_ROOT;
            TRY(!(view->search_on = nexus_get(nexus, title)), ERR_NEXUS_GET);
        } break;
        case VIEW_ICON: {
            view->current = &nexus->nodeicon;
        } break;
        case VIEW_NONE: THROW("view id should not be NONE");
        default: THROW("unknown view id: %u", view->id);
    }
    TRY(nexus_create_by_icon(nexus), "could not create by-icon view");

    return 0;
error:
    return -1;
} //}}}

void nexus_free(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    tnode_free(&nexus->nodes);
    tnodeicon_free(&nexus->nodesicon);
    vview_free(&nexus->views);
    node_free(&nexus->findings);
    view_free(&nexus->view);
    node_free(&nexus->nodeicon);
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
    if(result) THROW(ERR_NEXUS_REBUILD);
    ViewList id = nexus->view.id;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_CYGWIN)
    Node *current = (id == VIEW_SEARCH_SUB) ? nexus->view.search_on : nexus->view.current;
    Str args[5] = {0};
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
    if(!err) exit(0);
    return;
error:
    platform_getch();
    ERR_CLEAN;
}
/* }}} */

int nexus_userinput(Nexus *nexus, int key) /*{{{*/
{
    ASSERT(nexus, ERR_NULL_ARG);
    View *view = &nexus->view;
    bool disable_default = false;
    ASSERT(view, "view is 0!\n");
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
    return 0;
error:
    return -1;
} /*}}}*/

Node *nexus_get(Nexus *nexus, const char *title) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(title, ERR_NULL_ARG);
    Node *result = 0;
    size_t i0 = 0, j0 = 0;
    Node find = {0};
    TRY(node_create(&find, title, 0, 0, 0), ERR_NODE_CREATE);
    if(tnode_find(&nexus->nodes, &find, &i0, &j0)) {
        THROW("node does not exist in nexus: '%.*s'", STR_F(&find.title));
    }
    result = nexus->nodes.buckets[i0].items[j0];
clean:
    node_free(&find);
    str_free(&nexus->config.entry);
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
            IconStr iconstr = {0};
            icon_fmt(iconstr, node->icon);
            int found = search_fmt_nofree(true, &cmd, &content, search, "%s %.*s %.*s %.*s", iconstr, STR_F(&node->title), STR_F(&node->cmd), STR_F(&node->desc));
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

int nexus_insert_node(Nexus *nexus, Node **ref, char *title, char *cmd, char *desc, Icon icon) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(ref, ERR_NULL_ARG);
    size_t i = 0, j = 0;
    Node find = {
        .title = STR_L(title),
    };
    bool found = !tnode_find(&nexus->nodes, &find, &i, &j);
    if(found) {
        if(nexus->nodes.buckets[i].count[j]) {
            THROW("should not insert node with equal title '%s'", title);
        } else {
            /* node was added in nexus_link, via. add_count(0), meaning we should set the proper description etc. */
            Node *node = nexus->nodes.buckets[i].items[j];
            str_free(&node->title);
            VrNode in = node->incoming, out = node->outgoing;
            TRY(node_create(node, title, cmd, desc, icon), ERR_NODE_CREATE);
            node->incoming = in;
            node->outgoing = out;
        }
    } else {
        Node node;
        TRY(node_create(&node, title, cmd, desc, icon), ERR_NODE_CREATE);
        TRY(tnode_add(&nexus->nodes, &node), ERR_LUTD_ADD);
        tnode_find(&nexus->nodes, &find, &i, &j);
    }
    *ref = nexus->nodes.buckets[i].items[j];
    return 0;
error:
    return -1;
} //}}}

int nexus_link(Nexus *nexus, Node *src, Node *dest) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(src, ERR_NULL_ARG);
    ASSERT(dest, ERR_NULL_ARG);
    //printf("LINK %s ---- %s\n", str_iter_begin(&src->title), str_iter_begin(&dest->title));
    if(!tnode_has(&nexus->nodes, src)) {
        Node temp;
        TRY(node_copy(&temp, src), ERR_NODE_COPY);
        temp.icon = ICON_NONE;
        TRY(tnode_add_count(&nexus->nodes, &temp, 0), ERR_LUTD_ADD);
        //THROW("node does not exist in nexus: '%.*s'", STR_F(&src->title));
    }
    if(!tnode_has(&nexus->nodes, dest)) {
        Node temp;
        TRY(node_copy(&temp, dest), ERR_NODE_COPY);
        temp.icon = ICON_NONE;
        TRY(tnode_add_count(&nexus->nodes, &temp, 0), ERR_LUTD_ADD);
        //THROW("node does not exist in nexus: '%.*s'", STR_F(&dest->title));
    }
    size_t i0 = 0, i1 = 0, j0 = 0, j1 = 0;
    tnode_find(&nexus->nodes, src, &i0, &j0);
    tnode_find(&nexus->nodes, dest, &i1, &j1);
    Node *ev_src = nexus->nodes.buckets[i0].items[j0];
    Node *ev_dest = nexus->nodes.buckets[i1].items[j1];
    Icon i_src = ev_src->icon;
    Icon i_dest = ev_dest->icon;
    if(i_src == i_dest) {
        // ...
    } else if(i_src > i_dest) {
        Node *temp = ev_src;
        ev_src = ev_dest;
        ev_dest = temp;
    }
    TRY(vrnode_push_back(&ev_src->outgoing, ev_dest), ERR_VEC_PUSH_BACK);
    TRY(vrnode_push_back(&ev_dest->incoming, ev_src), ERR_VEC_PUSH_BACK);
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
            view->current = &nexus->nodeicon;
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

int nexus_build(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);

    Node *root;
    TRY(nexus_insert_node(nexus, &root, NEXUS_ROOT, CMD_NONE, "Welcome to " F("c-nexus", BOLD) "\n\n"
                F("basic controls", UL) "\n"
                "  h : back in history\n"
                "  j : move arrow down\n"
                "  k : move arrow up\n"
                "  l : follow the arrow\n\n"
                "more can be found in the " F("controls wiki", UL), ICON_ROOT), ERR_NEXUS_INSERT_NODE);

    NEXUS_INSERT(nexus, root, NODE_LEAF, ICON_WIKI, CMD_NONE, "Test!", "This is proof that I can link to a note, even if it gets created in the future", "Note yet to be created");
    NEXUS_INSERT(nexus, root, NODE_LEAF, ICON_WIKI, CMD_NONE, "Note yet to be created", "This note is created after Test!", NODE_LEAF);

    TRY(content_build(nexus, root), ERR_CONTENT_BUILD);
    tnode_sort_sub(&nexus->nodes);

    return 0;
error:
    return -1;
} //}}}

int nexus_current_view_arg(Nexus *nexus) /* {{{ */
{
    ViewList id = nexus->view.id;
    switch(id) {
        case VIEW_NORMAL: return SPECIFY_NORMAL;
        case VIEW_ICON: return SPECIFY_ICON;
        case VIEW_SEARCH_ALL: return SPECIFY_SEARCH_ALL;
        case VIEW_SEARCH_SUB: return SPECIFY_SEARCH_SUB;
        default: ABORT("can't translate view id (%i) to argument view id! perhaps it's argument's behavior is missing! (this is stupid)", id);
    }
} /* }}} */

