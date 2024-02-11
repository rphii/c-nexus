#include "nexus.h"
#include "search.h"

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

ThreadQueue(NexusThreadSearch) /* {{{ */
typedef struct NexusThreadSearch {
    TNode *tnodes;
    size_t i;
    size_t j;
    size_t human;
    Str cmd;
    Str content;
    Str search;
    VrNode findings;
  NexusThreadSearch_ThreadQueue *queue;
} NexusThreadSearch; /* }}} */

static void *nexus_static_thread_search(void *args) /* {{{ */
{
    NexusThreadSearch *arg = args;

    /* thread processing / search */
    Node *node = arg->tnodes->buckets[arg->i].items[arg->j];
    str_clear(&arg->cmd);
    str_clear(&arg->content);
    int found = search_fmt_nofree(true, &arg->cmd, &arg->content, &arg->search, "%s %.*s %.*s", icon_str(node->icon), STR_F(&node->title), STR_F(&node->desc));
    if(found) {
        TRY(vrnode_push_back(&arg->findings, node), ERR_VEC_PUSH_BACK);
    }

    /* finished this thread .. make space for next thread */
    NexusThreadSearch_ThreadQueue *queue = arg->queue;
    pthread_mutex_lock(&queue->mutex);
    queue->q[(queue->i0 + queue->len) % PROC_COUNT] = arg;
    ++queue->len;
    pthread_mutex_unlock(&queue->mutex);

clean:
    return 0;
error:
    goto clean;
} /* }}} */

/* }}} */

#endif /*}}}*/

int nexus_init(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    TRY(tnode_init(&nexus->nodes, 4), ERR_LUTD_INIT);
    return 0;
error:
    return -1;
} //}}}

void nexus_free(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    tnode_free(&nexus->nodes);
    vrnode_free(&nexus->history);
} //}}}

Node *nexus_get(Nexus *nexus, const char *title) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(title, ERR_NULL_ARG);
    Node *result = 0;
    size_t i0 = 0, j0 = 0;
    Node find = {0};
    TRY(node_create(&find, title, "", 0), ERR_NODE_CREATE);
    if(tnode_find(&nexus->nodes, &find, &i0, &j0)) {
        THROW("node does not exist in nexus: '%.*s'", STR_F(&find.title));
    }
    result = nexus->nodes.buckets[i0].items[j0];
clean:
    node_free(&find);
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

    /* set up */
    pthread_mutex_init(&thr_queue.mutex, 0);
    for(size_t i = 0; i < PROC_COUNT; ++i) {
        thr_search[i].tnodes = tnodes;
        thr_search[i].queue = &thr_queue;
        thr_search[i].human = i;
        str_copy(&thr_search[i].search, search);
        /* add to queue */
        thr_queue.q[i] = &thr_search[i];
        ++thr_queue.len;
    }
    assert(thr_queue.len <= PROC_COUNT);

    /* search */
    for(size_t i = 0; i < (1ULL << (tnodes->width - 1)); ++i) {
        size_t len = tnodes->buckets[i].len;
        for(size_t j = 0; j < len; ++j) {
            pthread_mutex_lock(&thr_queue.mutex);
            if(thr_queue.len) {
                NexusThreadSearch *thr = thr_queue.q[thr_queue.i0];
                thr->i = i;
                thr->j = j;
                pthread_create(&thr->queue->id, 0, nexus_static_thread_search, thr);
                thr_queue.i0 = (thr_queue.i0 + 1) % PROC_COUNT;
                --thr_queue.len;
            } else {
                --j;
            }
            pthread_mutex_unlock(&thr_queue.mutex);
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
    /* now join threads and add results; since we *know* all threads finished, we can ignore usage of the lock */
    vrnode_clear(findings);
    for(size_t i = 0; i < PROC_COUNT; ++i) {
        if(thr_search[i].queue->id) {
            /* join */
            pthread_join(thr_search[i].queue->id, 0);
            /* add results */
            for(size_t j = 0; j < vrnode_length(&thr_search[i].findings); ++j) {
                TRY(vrnode_push_back(findings, vrnode_get_at(&thr_search[i].findings, j)), ERR_VEC_PUSH_BACK);
            }
            /* free */
            vrnode_free(&thr_search[i].findings);
            str_free(&thr_search[i].content);
            str_free(&thr_search[i].cmd);
            str_free(&thr_search[i].search);
        }
    }

clean:
    return err;
error:
    ERR_CLEAN;

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

int nexus_follow_sub(Nexus *nexus, Node **current) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(current, ERR_NULL_ARG);
    size_t sO = vrnode_length(&(*current)->outgoing);
    size_t sI = vrnode_length(&(*current)->incoming);
    Node *result = (*current);
    if((*current)->sub_index < sO) {
        result = vrnode_get_at(&(*current)->outgoing, (*current)->sub_index);
    } else if((*current)->sub_index - sO < sI) {
        result = vrnode_get_at(&(*current)->incoming, (*current)->sub_index - sO);
    } else if(sO+sI) {
        THROW("sub_index '%zu' too large", (*current)->sub_index);
    }
    TRY(vrnode_push_back(&nexus->history, *current), ERR_VEC_PUSH_BACK);
    *current = result;
    (*current)->sub_index = result->sub_index;
    return 0;
error:
    return -1;
} //}}}

void nexus_history_back(Nexus *nexus, Node **current) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    ASSERT(current, ERR_NULL_ARG);
    if(vrnode_length(&nexus->history)) {
        vrnode_pop_back(&nexus->history, current);
    }
} //}}}

int nexus_build_physics(Nexus *nexus, Node *anchor) //{{{
{
    Node base;
    NEXUS_INSERT(nexus, anchor, &base, ICON_PHYSICS, "Physics", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &base, NODE_LEAF, ICON_PHYSICS, "Second", "A second is an SI-unit of time.", NODE_LEAF);
    NEXUS_INSERT(nexus, &base, NODE_LEAF, ICON_PHYSICS, "Meter", "A meter is an SI-unit of length.", NODE_LEAF);
    NEXUS_INSERT(nexus, &base, NODE_LEAF, ICON_PHYSICS, "Speed of Light", "The speed of light " F("in vacuum", IT) " is\n" F("c = 299'792'458 meters/second", BOLD) "", "Second", "Meter");

    return 0;
error:
    return -1;
} //}}}

int nexus_build(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);

    Node root;
    TRY(node_create(&root, NEXUS_ROOT, "", ICON_NONE), ERR_NODE_CREATE);
    TRY(nexus_insert_node(nexus, &root), ERR_NEXUS_INSERT_NODE);

    TRY(nexus_build_physics(nexus, &root), ERR_NEXUS_BUILD_PHYSICS);

    return 0;
error:
    return -1;
} //}}}

