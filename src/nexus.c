#include "nexus.h"
#include "node.h"
#include "search.h"
#include "cmd.h"

#if PROC_COUNT /* local threading {{{*/
#include <pthread.h>

#define ThreadQueue(X)      \
    typedef struct X##_ThreadQueue { \
        pthread_t id; \
        pthread_mutex_t mutex; \
        size_t i0; \
        size_t len; \
        struct X *q[PROC_COUNT]; \
    } X##_ThreadQueue;

/* local thread: search {{{ */

#define SEARCH_THREAD_BATCH     16

typedef struct NexusThreadSearchJob {
    size_t i;
    size_t j;
} NexusThreadSearchJob;

ThreadQueue(NexusThreadSearch) /* {{{ */
typedef struct NexusThreadSearch {
    TNode *tnodes;
    size_t human;
    Str cmd;
    Str content;
    Str *search;
    pthread_mutex_t *findings_mutex;
    VrNode *findings;
  NexusThreadSearchJob job[SEARCH_THREAD_BATCH];
  NexusThreadSearch_ThreadQueue *queue;
} NexusThreadSearch; /* }}} */

static void *nexus_static_thread_search(void *args) /* {{{ */
{
    NexusThreadSearch *arg = args;

    /* thread processing / search */
    for(size_t ib = 0; ib < SEARCH_THREAD_BATCH; ib++) {
        size_t i = arg->job[ib].i;
        size_t j = arg->job[ib].j;
        if(SIZE_IS_NEG(i) || SIZE_IS_NEG(j)) continue;
        Node *node = arg->tnodes->buckets[i].items[j];
        str_clear(&arg->cmd);
        str_clear(&arg->content);
        int found = search_fmt_nofree(true, &arg->cmd, &arg->content, arg->search, "%s %.*s %.*s", icon_str(node->icon), STR_F(&node->title), STR_F(&node->desc));
        if(found) {
            pthread_mutex_lock(arg->findings_mutex);
            TRY(vrnode_push_back(arg->findings, node), ERR_VEC_PUSH_BACK);
            pthread_mutex_unlock(arg->findings_mutex);
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


int nexus_arg(Nexus *nexus, Arg *arg)
{
    ASSERT(arg, ERR_NULL_ARG);
    ASSERT(nexus, ERR_NULL_ARG);
    nexus->args = arg;
    TRY(str_copy(&nexus->config.entry, &arg->entry), ERR_STR_COPY);
    switch(arg->view) {
        case SPECIFY_NONE:
        case SPECIFY_NORMAL: nexus->config.view = VIEW_NORMAL; break;
        case SPECIFY_SEARCH: nexus->config.view = VIEW_SEARCH; break;
        default: THROW(ERR_UNREACHABLE ", %u", arg->view);
    }
    return 0;
error:
    return -1;
}

int nexus_init(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    TRY(tnode_init(&nexus->nodes, 12), ERR_LUTD_INIT);
    TRY(nexus_build(nexus), ERR_NEXUS_BUILD);
    /* configure settings */
    nexus->show_desc = true;
    nexus->show_preview = false;
    nexus->max_preview = 20;
    /* set up view */
    View *view = &nexus->view;
    view->id = nexus->config.view;
    switch(view->id) {
        case VIEW_NORMAL: {
            char *title = str_length(&nexus->config.entry) ? str_iter_begin(&nexus->config.entry) : NEXUS_ROOT;
            TRY(!(view->current = nexus_get(nexus, title)), ERR_NEXUS_GET);
        } break;
        case VIEW_SEARCH: {
            view->edit = true;
        } break;
        case VIEW_NONE: THROW("view id should not be NONE");
        default: THROW("unknown view id: %u", view->id);
    }
    return 0;
error:
    return -1;
} //}}}

void nexus_free(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    tnode_free(&nexus->nodes);
    vview_free(&nexus->views);
    vrnode_free(&nexus->findings);
    view_free(&nexus->view);
} //}}}

/* rebuild yourself {{{ */
#if defined(PLATFORM_LINUX)
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
    Node *current = nexus->view.current;
    Str cmd = {0}, cmd2 = {0};
#if PROC_COUNT
    TRY(str_fmt(&cmd, "make -j %u", PROC_COUNT), ERR_STR_FMT)
#else
    TRY(str_fmt(&cmd, "make"), ERR_STR_FMT);
#endif
    int result = cmd_run(&cmd);
    if(result) THROW(ERR_NEXUS_REBUILD);
#if defined(PLATFORM_LINUX)
    str_clear(&cmd);
    Str *title = current ? &current->title : &STR(NEXUS_ROOT);
    TRY(str_fmt(&cmd, "--entry=%.*s", STR_F(title)), ERR_STR_FMT);
    TRY(str_fmt(&cmd2, "--view=%s", specify_str(nexus->args->view)), ERR_STR_FMT);
    printf("%s %.*s %.*s\n", nexus->args->name, STR_F(&cmd), STR_F(&cmd2));
    char *const argv[] = {(char *)nexus->args->name, str_iter_begin(&cmd), str_iter_begin(&cmd2), 0};
    execv(nexus->args->name, argv);
#elif defined(PLATFORM_WINDOWS)
    TRY(str_fmt(&cmd2, "--entry=\"%.*s\" --view=%s", STR_F(&current->title), specify_str(nexus->args->view)), ERR_STR_FMT);
    STARTUPINFO info_startup = {0};
    PROCESS_INFORMATION info_process = {0};
    LPCTSTR c = nexus->args->name;
    LPCTSTR c2 = str_iter_begin(&cmd2);
    result = CreateProcess(c, c2, 0, 0, FALSE, 0, 0, 0, &info_startup, &info_process);
    if(result) THROW(ERR_NEXUS_REBUILD);
    CloseHandle(info_process.hProcess);
    CloseHandle(info_process.hThread);
#else
    THROW("rebuild not yet implemented on '%s'", PLATFORM_NAME);
#endif
clean:
    str_free(&cmd);
    str_free(&cmd2);
    if(!err) exit(0);
    return;
error:
    platform_getch();
    ERR_CLEAN;
}
/* }}} */

int nexus_userinput(Nexus *nexus, int key)
{
    ASSERT(nexus, ERR_NULL_ARG);
    View *view = &nexus->view;
    ASSERT(view, "view is 0!\n");
    switch(view->id) {
        case VIEW_NORMAL: {
            switch(key) {
                case ' ': {
                    nexus->show_desc ^= true;
                } break;
                case 'i': {
                    nexus->show_preview ^= true;
                } break;
                case 'j': {
                    node_set_sub(view->current, &view->sub_sel, view->sub_sel + 1);
                } break;
                case 'k': {
                    node_set_sub(view->current, &view->sub_sel, view->sub_sel - 1);
                } break;
                case 'l': {
                    TRY(nexus_change_view(nexus, view, VIEW_NORMAL), ERR_NEXUS_CHANGE_VIEW);
                    TRY(nexus_follow_sub(nexus, view), ERR_NEXUS_FOLLOW_SUB);
                } break;
                case 'h': {
                    TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK);
                } break;
                case 'H': {
                    do {
                        TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK);
                    } while(view->id != VIEW_SEARCH && vview_length(&nexus->views));
                } break;
                case 'Q': {
                    nexus_rebuild(nexus);
                } break;
                case 'q': {
                    nexus->quit = true;
                } break;
                case 'f': {
                    TRY(nexus_change_view(nexus, view, VIEW_SEARCH), ERR_NEXUS_CHANGE_VIEW);
                } break;
                case 'c': {
                    cmd_run(&view->current->cmd);
                } break;
                case 'C': {
                    Node *sub = node_get_sub_sel(view->current, view->sub_sel);
                    if(sub) cmd_run(&sub->cmd);
                } break;
                default: break;
            }
        } break;
        case VIEW_SEARCH: {
            size_t len_search = str_length(&view->search);
            if(view->edit) {
                if(key >= 0x20 && key != 127) {
                    TRY(str_fmt(&view->search, "%c", key), ERR_STR_FMT);
                } else if(key == 127) {
                    if(len_search) str_pop_back(&view->search, 0);
                } else if(key == 8) {
                    str_pop_back_word(&view->search);
                } else if(key == '\n') {
                    view->edit = false;
                } else if(key == 27) {
                    TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK);
                }
            } else {
                size_t len = vrnode_length(&nexus->findings);
                switch(key) {
                    case ' ': {
                        nexus->show_desc ^= true;
                    } break;
                    case 'j': {
                        if(SIZE_IS_NEG(view->sub_sel)) view->sub_sel = 0;
                        ++view->sub_sel;
                        if(view->sub_sel >= len) {
                            view->sub_sel = 0;
                        }
                    } break;
                    case 'k': {
                        if(view->sub_sel > len) view->sub_sel = len ? len - 1 : 0;
                        --view->sub_sel;
                        if(SIZE_IS_NEG(view->sub_sel)) {
                            view->sub_sel = len ? len - 1 : 0;
                        }
                    } break;
                    case 'l': {
                        if(SIZE_IS_NEG(view->sub_sel)) view->sub_sel = 0;
                        if(view->sub_sel > len) view->sub_sel = len ? len - 1 : 0;
                        if(view->sub_sel < vrnode_length(&nexus->findings)) {
                            Node *target = vrnode_get_at(&nexus->findings, view->sub_sel);
                            TRY(nexus_change_view(nexus, view, VIEW_NORMAL), ERR_NEXUS_CHANGE_VIEW);
                            TRY(!(view->current = nexus_get(nexus, target->title.s)), ERR_NEXUS_GET); /* risky */
                        }
                    } break;
                    case '\n':
                    case 'f': {
                        view->edit = true;
                    } break;
                    case 'F': {
                        str_clear(&view->search);
                        view->edit = true;
                        view->sub_sel = 0;
                    } break;
                    case 'Q': {
                        nexus_rebuild(nexus);
                    } break;
                    case 'q': {
                        nexus->quit = true;
                    } break;
                    case 'h':
                    case 27: {
                        TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK);
                    } break;
                    case 'H': {
                        do {
                            TRY(nexus_history_back(nexus, view), ERR_NEXUS_HISTORY_BACK);
                        } while(view->id != VIEW_SEARCH && vview_length(&nexus->views));
                    } break;
                    case 'C': {
                        Node *sub = node_get_sub_sel(view->current, view->sub_sel);
                        if(sub) cmd_run(&sub->cmd);
                    } break;
                    default: break;
                }
            }
            /* post processing */
            if(len_search != str_length(&view->search)) {
                nexus->findings_updated = false;
            }

        } break;
        default: THROW("unknown view id: %u", view->id);
    }
    return 0;
error:
    return -1;
}

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

int nexus_search(Nexus *nexus, Str *search, VrNode *findings) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(search, ERR_NULL_ARG);
    ASSERT(findings, ERR_NULL_ARG);
    int err = 0;

#if PROC_COUNT

    /* declarations */
    TNode *tnodes = &nexus->nodes;
    NexusThreadSearch thr_search[PROC_COUNT] = {0};
    NexusThreadSearch_ThreadQueue thr_queue = {0};
    NexusThreadSearchJob job[SEARCH_THREAD_BATCH] = {0};
    size_t job_counter = 0;
    pthread_attr_t thr_attr;
    pthread_mutex_t findings_mutex;

    /* set up */
    vrnode_clear(findings);
    pthread_mutex_init(&thr_queue.mutex, 0);
    pthread_mutex_init(&findings_mutex, 0);
    for(size_t i = 0; i < PROC_COUNT; ++i) {
        thr_search[i].findings = findings;
        thr_search[i].findings_mutex = &findings_mutex;
        thr_search[i].tnodes = tnodes;
        thr_search[i].queue = &thr_queue;
        thr_search[i].human = i;
        thr_search[i].search = search;
        /* add to queue */
        thr_queue.q[i] = &thr_search[i];
        ++thr_queue.len;
    }
    pthread_attr_init(&thr_attr);
    pthread_attr_setdetachstate(&thr_attr, PTHREAD_CREATE_DETACHED);
    assert(thr_queue.len <= PROC_COUNT);

    /* search */
    size_t len_t = (1UL << (tnodes->width - 1));
    size_t last_t = 0;
    for(size_t i = 0; i < len_t; i++) {
        if(tnodes->buckets[i].len) last_t = i;
    }
    for(size_t i = 0; i < len_t; ++i) {
        size_t len = tnodes->buckets[i].len;
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
            /* free */
            str_free(&thr_search[i].content);
            str_free(&thr_search[i].cmd);
        }
    }

    return err;

#else

    vrnode_clear(findings);
    TNode *tnodes = &nexus->nodes;
    Str cmd = {0}, content = {0};
    for(size_t i = 0; i < (1ULL << tnodes->width); i++) {
        size_t len = tnodes->buckets[i].len;
        for(size_t j = 0; j < len; j++) {
            Node *node = tnodes->buckets[i].items[j];
            str_clear(&cmd);
            str_clear(&content);
            int found = search_fmt_nofree(true, &cmd, &content, search, "%s %.*s %.*s", icon_str(node->icon), STR_F(&node->title), STR_F(&node->desc));
            if(found) {
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

#endif

} //}}}

int nexus_insert_node(Nexus *nexus, Node *node) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(node, ERR_NULL_ARG);
    if(tnode_has(&nexus->nodes, node)) THROW("should not insert node with equal title");
    TRY(tnode_add(&nexus->nodes, node), ERR_LUTD_ADD);
    return 0;
error:
    return -1;
} //}}}

int nexus_link(Nexus *nexus, Node *src, Node *dest) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(src, ERR_NULL_ARG);
    ASSERT(dest, ERR_NULL_ARG);
    if(!tnode_has(&nexus->nodes, src)) THROW("node does not exist in nexus: '%.*s'", STR_F(&src->title));
    if(!tnode_has(&nexus->nodes, dest)) THROW("node does not exist in nexus: '%.*s'", STR_F(&dest->title));
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
    size_t sO = vrnode_length(&current->outgoing);
    size_t sI = vrnode_length(&current->incoming);
    Node *result = current;
    if(view->sub_sel < sO) {
        result = vrnode_get_at(&current->outgoing, view->sub_sel);
    } else if(view->sub_sel - sO < sI) {
        result = vrnode_get_at(&current->incoming, view->sub_sel - sO);
    } else if(sO+sI) {
        THROW("sub_index '%zu' too large", view->sub_sel);
    }
    view->current = result;
    view->sub_sel = 0;
    return 0;
error:
    return -1;
} //}}}

int nexus_change_view(Nexus *nexus, View *view, ViewList id)
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
    ViewList id_post = VIEW_NONE;
    /* init view to be changed into */
    view_free(view);
    memset(view, 0, sizeof(*view));
    view->current = ref.current;
    view->sub_sel = ref.sub_sel;
    view->id = id;
    /* init the different views */
    switch(id) {
        case VIEW_NORMAL: {
            if(ref.id == VIEW_SEARCH) view->sub_sel = 0;
            view->edit = false;
        } break;
        case VIEW_SEARCH: {
            view->sub_sel = 0;
            if(id_post != VIEW_SEARCH) {
                str_clear(&view->search);
                view->edit = true;
            }
        } break;
        case VIEW_NONE: THROW("view id should not be NONE");
        default: THROW("unknown view id: %u", id);
    }
    return 0;
error:
    return -1;
}

int nexus_history_back(Nexus *nexus, View *view) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(view, ERR_NULL_ARG);
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

int nexus_build_math(Nexus *nexus, Node *anchor) /*{{{*/
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_MATH, CMD_NONE, "Math", "", NODE_LEAF);

    NEXUS_INSERT(nexus, &base, &sub, ICON_MATH, CMD_NONE, "Operations", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Addition", "Common terms used:\nsum = term + term\nsum = summand + summand\nsum = addend + addend\nsum = augend + addend", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Subtraction", "Common terms used:\ndifference = term - term\ndifference = minuend - subtrahend", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Multiplication", "Common terms used:\nproduct = factor * factor\nproduct = multiplier * multiplicand", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Division", "Common terms used:\nfraction or quotient or ratio = dividend / divisor\nfraction or quotient or ratio = numerator / denominator", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Exponentiation", "Common terms used:\npower = base ^ exponent", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "nth root", "Common terms used:\n"
            "       degree /--------\n"
            "root =      \\/ radicand", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, CMD_NONE, "Logarithm", "Common terms used:\nlogarithm = log_{base} ( anti-logarithm )", NODE_LEAF);
    return 0;
error:
    return -1;
} /*}}}*/

int nexus_build_physics(Nexus *nexus, Node *anchor) //{{{
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_PHYSICS, CMD_NONE, "Physics", "", NODE_LEAF);

    /* set up units */
    NEXUS_INSERT(nexus, &base, &sub, ICON_PHYSICS, CMD_NONE, "Physical Units", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Second", "A second is an SI-unit of time.", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Meter", "A meter is an SI-unit of length.", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Speed of Light", "The speed of light " F("in vacuum", IT) " is " F("c = 299'792'458 meters/second", BOLD UL) "", "Second", "Meter");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Volt", "Volt has the unit [V]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Ohm", "Ohm has the unit [Ω]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Ampere", "Ampere has the unit [A]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Coulomb", "Coulomb has the unit [C]", NODE_LEAF);

    /* set up other stuff */
    NEXUS_INSERT(nexus, &base, &sub, ICON_PHYSICS, CMD_NONE, "Electrical Engineering", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Voltage", "A common formula for the voltage U is " F("U = R * I", BOLD UL) ", where ..\n"
            "  - R is the resistance in Ohm\n"
            "  - I is the electric current in Ampere", "Volt");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Resistance", "A common formula for the resistance R is " F("R = U / I", BOLD UL) ", where ..\n"
            "  - U is the voltage in Volt\n"
            "  - I is the electric current in Ampere", "Ohm", "Voltage");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Electric Current", "A common formula for the electric current I is " F("I = U / R", BOLD UL) ", where ..\n"
            "  - I is the electric current in Ampere\n"
            "  - R is the resistance in Ohm", "Voltage", "Ampere", "Resistance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Capacitance", "A common formula for the capacitance C is " F("C = Q / U", BOLD UL) ", where ..\n"
            "  - Q is the charge in Coulomb\n"
            "  - U is the voltage in Volts", "Coulomb", "Voltage");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Capacitor", "In electrical engineering, a " F("capacitor", BOLD) " is a device that stores electrical energy by accumulating electric charges on two closely spaced surfaces that are insulated from each other. It is a passive electronic component with two terminals.\n\n"
            "The effect of a capacitor is known as " F("capacitance", UL) ". While some capacitance exists between any two electrical conductors in proximity in a circuit, a capacitor is a component designed to add capacitance to a circuit. The capacitor was originally known as the condenser, a term still encountered in a few compound names, such as the condenser microphone.", "Capacitance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Resistor", "A resistor is a passive two-terminal electrical component that implements electrical resistance as a circuit element. In electronic circuits, resistors are used to reduce current flow, adjust signal levels, to divide voltages, bias active elements, and terminate transmission lines, among other uses.\n\n"
            "The electrical function of a resistor is specified by its " F("resistance", UL), "Resistance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, CMD_NONE, "Ohm's law", "Ohm's law is basically any of the common formulas in " F("Resistance", UL) ", " F("Electric Current", UL) " or " F("Capacitance", UL), "Resistance", "Capacitance", "Electric Current");

    return 0;
error:
    return -1;
} //}}}

int nexus_build_controls(Nexus *nexus, Node *anchor)
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_WIKI, CMD_NONE, "Controls", "Guide to all the various controls in c-nexus." , NODE_LEAF);

    /* normal view {{{ */
    NEXUS_INSERT(nexus, &base, &sub, ICON_WIKI, CMD_NONE, "Normal View", "In this mode you can browse the notes.\n"
            "\n" F("basic controls", UL) "\n"
            "  for the normal mode are listed in the root, so I'm not going to list them again.\n"
            "\n" F("other controls", UL) "\n"
            "  H                : go back to most recent search\n"
            "  f                : enter " F("search mode", FG_YL_B) "\n"
            "  q                : quit and return to the terminal\n"
            "  Q                : rebuild nexus\n"
            "  C                : run command associated to note pointed at by the arrow\n"
            "  SPACE            : toggle showing note descriptions on/off\n"
            "  i                : toggle previewing note descriptions on/off", NODE_LEAF);
    /* }}} */

    /* search view {{{ */
    NEXUS_INSERT(nexus, &base, &sub, ICON_WIKI, CMD_NONE, "Search View", "In this mode you can search notes for substrings.\n"
            "\n" F("what gets searched?", UL) "\n"
            "  the exact pattern of each note you can search are (in this order): " F("ICON title description", FG_GN) " (e.g. search for: 'wiki normal' -> you will find normal view note)\n"
            "  searches are case insensitive. any newlines are removed. whitespaces are condensed into one space (e.g. XYZ    ABC -> XYZ ABC)\n"
            "\n" F("controls while editing search string", UL) "\n"
            "  ENTER            : browse found notes\n"
            "  ESC              : abort search and go back to " F("normal view", FG_YL_B) "\n"
            "  [type anything]  : search notes for substring\n"
            "  CTRL+BACK        : erase a word/until word\n"
            "\n" F("controls while browsing found notes", UL) "\n"
            "  ENTER            : edit search string\n"
            "  f                : same as above\n"
            "  F                : clear search string and edit it\n"
            "  ESC              : abort search and go back to " F("normal view", FG_YL_B) "\n"
            "  hjkl             : same as the basic controls\n"
            "  H                : go back to most recent search\n"
            "  q                : quit and return to the terminal\n"
            "  Q                : rebuild nexus\n"
            "  c                : run command associated to current note\n"
            "  C                : run command associated to note pointed at by the arrow\n"
            "  SPACE            : toggle showing note descriptions on/off\n"
            , "Normal View");
    /* }}} */


    return 0;
error:
    return -1;
}

int nexus_build_cmds(Nexus *nexus, Node *anchor) /* {{{ */
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(anchor, ERR_NULL_ARG);
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_WIKI, "", "Notes with Commands", "Every note can have a command attached. See controls on how to use it.", "Controls");
    /* linux {{{ */
    NEXUS_INSERT(nexus, &base, &sub, ICON_WIKI, CMD_NONE, "Linux Commands", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_WIKI, CMD_SUDO("shutdown -h now"), "Shutdown linux", "This command lets you shut down your linux system", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_WIKI, CMD_SUDO("reboot"), "Reboot linux", "This command lets you reboot your linux system", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_WIKI, CMD_IMG("images/stickman.png"), "A stickman", "Execute the command on this note to show an image with the program imv!", NODE_LEAF);
    /* }}} */
    /* windows {{{ */
    /* tbd }}} */
    return 0;
error:
    return -1;
} /* }}} */

int nexus_build(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);

    Node root;
    TRY(node_create(&root, NEXUS_ROOT, CMD_NONE, "Welcome to " F("c-nexus", BOLD) "\n\n"
                F("basic controls", UL) "\n"
                "  h : back in history\n"
                "  j : move arrow down\n"
                "  k : move arrow up\n"
                "  l : follow the arrow\n\n"
                "more can be found in the " F("controls wiki", UL) "", ICON_NONE), ERR_NODE_CREATE);
    TRY(nexus_insert_node(nexus, &root), ERR_NEXUS_INSERT_NODE);

    TRY(nexus_build_controls(nexus, &root), ERR_NEXUS_BUILD_CONTROLS);
    TRY(nexus_build_cmds(nexus, &root), ERR_NEXUS_BUILD_CMDS);
    TRY(nexus_build_physics(nexus, &root), ERR_NEXUS_BUILD_PHYSICS);
    TRY(nexus_build_math(nexus, &root), ERR_NEXUS_BUILD_MATH);
    //NEXUS_INSERT(nexus, &root, NODE_LEAF, ICON_WIKI, "make -j \"$(nproc --all)\" && ./a --entry='Rebuild'", "Rebuild", "!!!", NODE_LEAF);

    return 0;
error:
    return -1;
} //}}}

