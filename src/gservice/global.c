/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the GService GLOBAL function.
 *
 * $Id: global.c 139 2005-05-29 04:50:37Z nenolod $
 */

#include "atheme.h"

/* GLOBAL <parameters>|SEND|CLEAR */
void gs_global(char *origin)
{
	static BlockHeap *glob_heap = NULL;
	struct global_ *global;
	static list_t globlist;
	node_t *n, *n2, *tn;
	tld_t *tld;
	char *params = strtok(NULL, "");
	static char *sender = NULL;

	if (!params)
	{
		notice(globsvs.nick, origin, "Insufficient parameters for \2GLOBAL\2.");
		notice(globsvs.nick, origin, "Syntax: GLOBAL <parameters>|SEND|CLEAR");
		return;
	}

	if (!strcasecmp("CLEAR", params))
	{
		if (!globlist.count)
		{
			notice(globsvs.nick, origin, "No message to clear.");
			return;
		}

		/* destroy the list we made */
		LIST_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			node_del(n, &globlist);
			node_free(n);
			free(global->text);
			BlockHeapFree(glob_heap, global);
		}

		BlockHeapDestroy(glob_heap);
		glob_heap = NULL;
		free(sender);
		sender = NULL;

		notice(globsvs.nick, origin, "The pending message has been deleted.");

		return;
	}

	if (!strcasecmp("SEND", params))
	{
		if (!globlist.count)
		{
			notice(globsvs.nick, origin, "No message to send.");
			return;
		}

		LIST_FOREACH(n, globlist.head)
		{
			global = (struct global_ *)n->data;

			/* send to every tld */
			LIST_FOREACH(n2, tldlist.head)
			{
				tld = (tld_t *)n2->data;

				sts(":%s NOTICE %s*%s :[Network Notice] %s", globsvs.nick, tldprefix, tld->name, global->text);
			}
		}

		/* destroy the list we made */
		LIST_FOREACH_SAFE(n, tn, globlist.head)
		{
			global = (struct global_ *)n->data;
			node_del(n, &globlist);
			node_free(n);
			free(global->text);
			BlockHeapFree(glob_heap, global);
		}

		BlockHeapDestroy(glob_heap);
		glob_heap = NULL;
		free(sender);
		sender = NULL;

		snoop("GLOBAL: \2%s\2", origin);

		notice(globsvs.nick, origin, "The global notice has been sent.");

		return;
	}

	if (!glob_heap)
		glob_heap = BlockHeapCreate(sizeof(struct global_), 5);

	if (!sender)
		sender = sstrdup(origin);

	if (irccasecmp(sender, origin))
	{
		notice(globsvs.nick, origin, "There is already a GLOBAL in progress by \2%s\2.", sender);
		return;
	}

	global = BlockHeapAlloc(glob_heap);

	global->text = sstrdup(params);

	n = node_create();
	node_add(global, n, &globlist);

	notice(globsvs.nick, origin,
	       "Stored text to be sent as line %d. Use \2GLOBAL SEND\2 "
	       "to send message, \2GLOBAL CLEAR\2 to delete the pending message, " "or \2GLOBAL\2 to store additional lines.", globlist.count);
}
