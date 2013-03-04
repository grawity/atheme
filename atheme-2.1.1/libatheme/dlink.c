/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Linked list stuff.
 *
 * $Id: dlink.c 7273 2006-11-25 00:25:20Z jilles $
 */

#include <org.atheme.claro.base>
#include "claro_internal.h"

static BlockHeap *node_heap;

void init_dlink_nodes(void)
{
        node_heap = BlockHeapCreate(sizeof(node_t), HEAP_NODE);

	if (!node_heap)
	{
		claro_log(LG_INFO, "init_dlink_nodes(): block allocator failure.");
		exit(EXIT_FAILURE);
	}
}

/* creates a new node */
node_t *node_create(void)
{
        node_t *n;

        /* allocate it */
        n = BlockHeapAlloc(node_heap);

        /* initialize */
        n->next = n->prev = n->data = NULL;

        /* up the count */
        claro_state.node++;

        /* return a pointer to the new node */
        return n;
}

/* frees a node */
void node_free(node_t *n)
{
        /* free it */
        BlockHeapFree(node_heap, n);

        /* down the count */
        claro_state.node--;
}

/* adds a node to the end of a list */
void node_add(void *data, node_t *n, list_t *l)
{
        node_t *tn;

        n->next = n->prev = NULL;
        n->data = data;

        /* first node? */
        if (!l->head)
        {
                l->head = n;
                l->tail = n;
                l->count++;
                return;
        }

	/* Speed increase. */
	tn = l->tail;

        /* set the our `prev' to the last node */
        n->prev = tn;

        /* set the last node's `next' to us */
        n->prev->next = n;

        /* set the list's `tail' to us */
        l->tail = n;

        /* up the count */
        l->count++;
}

/* adds a node to the head of a list */
void node_add_head(void *data, node_t *n, list_t *l)
{
	node_t *tn;

	n->next = n->prev = NULL;
	n->data = data;

	/* first node? */
	if (!l->head)
	{
		l->head = n;
		l->tail = n;
		l->count++;
		return;
	}

	tn = l->head;
	n->next = tn;
	tn->prev = n;
	l->head = n;
	l->count++;
}

/* adds a node to a list before another node, or to the end */
void node_add_before(void *data, node_t *n, list_t *l, node_t *before)
{
	if (before == NULL)
		node_add(data, n, l);
	else if (before == l->head)
		node_add_head(data, n, l);
	else
	{
		n->data = data;
		n->prev = before->prev;
		n->next = before;
		before->prev = n;
		l->count++;
	}
}

void node_del(node_t *n, list_t *l)
{
        /* do we even have a node? */
        if (!n)
        {
                claro_log(LG_DEBUG, "node_del(): called with NULL node");
                return;
        }

        /* are we the head? */
        if (!n->prev)
                l->head = n->next;
        else
                n->prev->next = n->next;

        /* are we the tail? */
        if (!n->next)
                l->tail = n->prev;
        else
                n->next->prev = n->prev;

        /* down the count */
        l->count--;
}

/* finds a node by `data' */
node_t *node_find(void *data, list_t *l)
{
        node_t *n;

        LIST_FOREACH(n, l->head) if (n->data == data)
                return n;

        return NULL;
}

void node_move(node_t *m, list_t *oldlist, list_t *newlist)
{
        /* Assumption: If m->next == NULL, then list->tail == m
         *      and:   If m->prev == NULL, then list->head == m
         */
        if (m->next)
                m->next->prev = m->prev;
        else
                oldlist->tail = m->prev;

        if (m->prev)
                m->prev->next = m->next;
        else
                oldlist->head = m->next;

        m->prev = NULL;
        m->next = newlist->head;
        if (newlist->head != NULL)
                newlist->head->prev = m;
        else if (newlist->tail == NULL)
                newlist->tail = m;
        newlist->head = m;

        oldlist->count--;
        newlist->count++;
}

