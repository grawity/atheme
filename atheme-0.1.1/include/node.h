/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures and macros for manipulating linked lists.
 *
 * $Id: node.h 156 2005-05-29 06:48:22Z nenolod $
 */

#ifndef NODE_H
#define NODE_H

typedef struct node_ node_t;
typedef struct list_ list_t;

/* macros for linked lists */
#define LIST_FOREACH(n, head) for (n = (head); n; n = n->next)  
#define LIST_FOREACH_NEXT(n, head) for (n = (head); n->next; n = n->next)

#define LIST_LENGTH(list) (list)->count

#define LIST_FOREACH_SAFE(n, tn, head) for (n = (head), tn = n ? n->next : NULL; n != NULL; n = tn, tn = n ? n->next : NULL)

/* list node struct */
struct node_
{
  node_t *next, *prev; 
  void *data;                   /* pointer to real structure */
};

/* node list struct */
struct list_
{
  node_t *head, *tail;
  int count;                    /* how many entries in the list */
};

#endif
