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
    Str *search;
    pthread_mutex_t *findings_mutex;
    VrNode *findings;
  NexusThreadSearch_ThreadQueue *queue;
} NexusThreadSearch; /* }}} */

static void *nexus_static_thread_search(void *args) /* {{{ */
{
    NexusThreadSearch *arg = args;

    /* thread processing / search */
    Node *node = arg->tnodes->buckets[arg->i].items[arg->j];
    str_clear(&arg->cmd);
    str_clear(&arg->content);
    int found = search_fmt_nofree(true, &arg->cmd, &arg->content, arg->search, "%s %.*s %.*s", icon_str(node->icon), STR_F(&node->title), STR_F(&node->desc));
    if(found) {
        pthread_mutex_lock(arg->findings_mutex);
        TRY(vrnode_push_back(arg->findings, node), ERR_VEC_PUSH_BACK);
        pthread_mutex_unlock(arg->findings_mutex);
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

int nexus_init(Nexus *nexus) //{{{
{
    ASSERT(nexus, ERR_NULL_ARG);
    TRY(tnode_init(&nexus->nodes, 12), ERR_LUTD_INIT);
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
    for(size_t i = 0; i < (1ULL << (tnodes->width - 1)); ++i) {
        size_t len = tnodes->buckets[i].len;
        for(size_t j = 0; j < len; ++j) {
            pthread_mutex_lock(&thr_queue.mutex);
            if(thr_queue.len) {
                NexusThreadSearch *thr = thr_queue.q[thr_queue.i0];
                thr->i = i;
                thr->j = j;
                pthread_create(&thr->queue->id, &thr_attr, nexus_static_thread_search, thr);
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

int nexus_build_math(Nexus *nexus, Node *anchor) /*{{{*/
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_MATH, "Math", "", NODE_LEAF);

    NEXUS_INSERT(nexus, &base, &sub, ICON_MATH, "Operations", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, "Addition", "Common terms used:\nsum = term + term\nsum = summand + summand\nsum = addend + addend\nsum = augend + addend", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, "Subtraction", "Common terms used:\ndifference = term - term\ndifference = minuend - subtrahend", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, "Multiplication", "Common terms used:\nproduct = factor * factor\nproduct = multiplier * multiplicand", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, "Division", "Common terms used:\nfraction or quotient or ratio = dividend / divisor\nfraction or quotient or ratio = numerator / denominator", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, "Exponentiation", "Common terms used:\npower = base ^ exponent", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, "nth root", "Common terms used:\n"
            "       degree /--------\n"
            "root =      \\/ radicand", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_MATH, "Logarithm", "Common terms used:\nlogarithm = log_{base} ( anti-logarithm )", NODE_LEAF);
    return 0;
error:
    return -1;
} /*}}}*/

int nexus_build_physics(Nexus *nexus, Node *anchor) //{{{
{
    Node base, sub;
    NEXUS_INSERT(nexus, anchor, &base, ICON_PHYSICS, "Physics", "", NODE_LEAF);

    /* set up units */
    NEXUS_INSERT(nexus, &base, &sub, ICON_PHYSICS, "Physical Units", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Second", "A second is an SI-unit of time.", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Meter", "A meter is an SI-unit of length.", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Speed of Light", "The speed of light " F("in vacuum", IT) " is " F("c = 299'792'458 meters/second", BOLD UL) "", "Second", "Meter");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Volt", "Volt has the unit [V]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Ohm", "Ohm has the unit [Ω]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Ampere", "Ampere has the unit [A]", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Coulomb", "Coulomb has the unit [C]", NODE_LEAF);

    /* set up other stuff */
    NEXUS_INSERT(nexus, &base, &sub, ICON_PHYSICS, "Electrical Engineering", "", NODE_LEAF);
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Voltage", "A common formula for the voltage U is " F("U = R * I", BOLD UL) ", where ..\n"
            "  - R is the resistance in Ohm\n"
            "  - I is the electric current in Ampere", "Volt");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Resistance", "A common formula for the resistance R is " F("R = U / I", BOLD UL) ", where ..\n"
            "  - U is the voltage in Volt\n"
            "  - I is the electric current in Ampere", "Ohm", "Voltage");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Electric Current", "A common formula for the electric current I is " F("I = U / R", BOLD UL) ", where ..\n"
            "  - I is the electric current in Ampere\n"
            "  - R is the resistance in Ohm", "Voltage", "Ampere", "Resistance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Capacitance", "A common formula for the capacitance C is " F("C = Q / U", BOLD UL) ", where ..\n"
            "  - Q is the charge in Coulomb\n"
            "  - U is the voltage in Volts", "Coulomb", "Voltage");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Capacitor", "In electrical engineering, a " F("capacitor", BOLD) " is a device that stores electrical energy by accumulating electric charges on two closely spaced surfaces that are insulated from each other. It is a passive electronic component with two terminals.\n\n"
            "The effect of a capacitor is known as " F("capacitance", UL) ". While some capacitance exists between any two electrical conductors in proximity in a circuit, a capacitor is a component designed to add capacitance to a circuit. The capacitor was originally known as the condenser, a term still encountered in a few compound names, such as the condenser microphone.", "Capacitance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Resistor", "A resistor is a passive two-terminal electrical component that implements electrical resistance as a circuit element. In electronic circuits, resistors are used to reduce current flow, adjust signal levels, to divide voltages, bias active elements, and terminate transmission lines, among other uses.\n\n"
            "The electrical function of a resistor is specified by its " F("resistance", UL), "Resistance");
    NEXUS_INSERT(nexus, &sub, NODE_LEAF, ICON_PHYSICS, "Ohm's law", "Ohm's law is basically any of the common formulas in " F("Resistance", UL) ", " F("Electric Current", UL) " or " F("Capacitance", UL), "Resistance", "Capacitance", "Electric Current");

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
    TRY(nexus_build_math(nexus, &root), ERR_NEXUS_BUILD_MATH);

    return 0;
error:
    return -1;
} //}}}

