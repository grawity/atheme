/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * OperServ NOOP command.
 *
 * $Id: os_noop.c 834 2005-07-16 07:44:10Z nenolod $
 */

#include "atheme.h"

typedef struct noop_ noop_t;

struct noop_ {
	char *target;
	char *added_by;
	char *reason;
};

list_t noop_hostmask_list;
list_t noop_server_list;

static void os_cmd_noop(char *origin);
static void check_user(user_t *u);
static BlockHeap *noop_heap;

command_t os_noop = { "NOOP", "Handles NOOP lists.", AC_SRA,
			os_cmd_noop };

extern list_t os_cmdtree;

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;		/* Cannot be safely unloaded, provides a block allocator. */
	command_add(&os_noop, &os_cmdtree);
	hook_add_event("user_oper");
	hook_add_hook("user_oper", (void (*)(void *)) check_user);

	noop_heap = BlockHeapCreate(sizeof(noop_t), 256);

	if (!noop_heap)
	{
		slog(LG_INFO, "os_noop: Block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

void _moddeinit()
{
	command_delete(&os_noop, &os_cmdtree);
	hook_del_hook("user_oper", (void (*)(void *)) check_user);
	hook_del_event("user_oper");
}

static void check_user(user_t *u)
{
	node_t *n;
	char hostbuf[BUFSIZE];

	snprintf(hostbuf, BUFSIZE, "%s!%s@%s", u->nick, u->user, u->host);

	LIST_FOREACH(n, noop_hostmask_list.head)
	{
		noop_t *np = n->data;

		if (!match(np->target, hostbuf))
		{
			skill(opersvs.nick, u->nick, "Operator access denied to hostmask: %s [%s] <%s@%s>",
				np->target, np->reason, np->added_by, opersvs.nick);
			return;
		}
	}

	LIST_FOREACH(n, noop_server_list.head)
	{
		noop_t *np = n->data;

		if (!match(np->target, u->server->name))
		{
			skill(opersvs.nick, u->nick, "Operator access denied to server: %s [%s] <%s@%s>",
				np->target, np->reason, np->added_by, opersvs.nick);
			return;
		}
	}
}

static noop_t *noop_find(char *target, list_t *list)
{
	node_t *n;

	LIST_FOREACH(n, list->head)
	{
		noop_t *np = n->data;

		if (!match(np->target, target))
			return np;
	}

	return NULL;
}

/* NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> [reason] */
static void os_cmd_noop(char *origin)
{
	node_t *n;
	noop_t *np;
	char *action = strtok(NULL, " ");
	char *type = strtok(NULL, " ");
	char *mask = strtok(NULL, " ");
	char *reason = strtok(NULL, "");

	if (!action || !type)
	{
		notice(opersvs.nick, origin, "Insufficient parameters provided for \2NOOP\2.");
		notice(opersvs.nick, origin, "Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]");
		return;
	}

	if (!strcasecmp(action, "ADD"))
	{
		if (!strcasecmp(type, "HOSTMASK"))
		{
			if ((np = noop_find(mask, &noop_hostmask_list)))
			{
				notice(opersvs.nick, origin, "There is already a NOOP entry covering this target.");
				return;
			}

			np = BlockHeapAlloc(noop_heap);

			np->target = sstrdup(mask);
			np->added_by = sstrdup(origin);

			if (reason)
				np->reason = sstrdup(reason);
			else
				np->reason = sstrdup("Abusive operator.");

			n = node_create();
			node_add(np, n, &noop_hostmask_list);

			notice(opersvs.nick, origin, "Added \2%s\2 to the hostmask NOOP list.", mask);

			return;
		}
		else if (!strcasecmp(type, "SERVER"))
		{
			if ((np = noop_find(mask, &noop_server_list)))
			{
				notice(opersvs.nick, origin, "There is already a NOOP entry covering this target.");
				return;
			}

			np = BlockHeapAlloc(noop_heap);

			np->target = sstrdup(mask);
			np->added_by = sstrdup(origin);

			if (reason)
				np->reason = sstrdup(reason);
			else
				np->reason = sstrdup("Abusive operator.");

			n = node_create();
			node_add(np, n, &noop_server_list);

			notice(opersvs.nick, origin, "Added \2%s\2 to the server NOOP list.", mask);

			return;
		}
		else
		{
			notice(opersvs.nick, origin, "Insufficient parameters provided for \2NOOP\2.");
			notice(opersvs.nick, origin, "Syntax: NOOP ADD <HOSTMASK|SERVER> <mask> [reason]");
		}			
	}
	else if (!strcasecmp(action, "DEL"))
	{
		if (!strcasecmp(type, "HOSTMASK"))
		{
			if (!(np = noop_find(mask, &noop_hostmask_list)))
			{
				notice(opersvs.nick, origin, "There is no NOOP hostmask entry for this target.");
				return;
			}

			n = node_find(np, &noop_server_list);

			free(np->target);
			free(np->added_by);
			free(np->reason);

			node_del(n, &noop_hostmask_list);

			notice(opersvs.nick, origin, "Removed \2%s\2 from the hostmask NOOP list.", mask);

			return;
		}
		else if (!strcasecmp(type, "SERVER"))
		{
			if (!(np = noop_find(mask, &noop_server_list)))
			{
				notice(opersvs.nick, origin, "There is no NOOP server entry for this target.");
				return;
			}

			n = node_find(np, &noop_server_list);

			free(np->target);
			free(np->added_by);
			free(np->reason);

			node_del(n, &noop_server_list);

			notice(opersvs.nick, origin, "Removed \2%s\2 from the server NOOP list.", mask);

			return;
		}
		else
		{
			notice(opersvs.nick, origin, "Insufficient parameters provided for \2NOOP\2.");
			notice(opersvs.nick, origin, "Syntax: NOOP DEL <HOSTMASK|SERVER> <mask>");
		}			
	}
	else if (!strcasecmp(action, "LIST"))
	{
		if (!strcasecmp(type, "HOSTMASK"))
		{
			uint16_t i = 1;
			notice(opersvs.nick, origin, "Hostmask NOOP list (%d entries):", noop_hostmask_list.count);
			notice(opersvs.nick, origin, " ");
			notice(opersvs.nick, origin, "Entry Hostmask                        Adder                 Reason");
			notice(opersvs.nick, origin, "----- ------------------------------- --------------------- --------------------------");

			LIST_FOREACH(n, noop_hostmask_list.head)
			{
				np = n->data;

				notice(opersvs.nick, origin, "%-5d %-31s %-21s %s", i, np->target, np->added_by, np->reason);
				i++;
			}

			notice(opersvs.nick, origin, "----- ------------------------------- --------------------- --------------------------");
			notice(opersvs.nick, origin, "End of Hostmask NOOP list.");
		}
		else if (!strcasecmp(type, "SERVER"))
		{
			uint16_t i = 1;
			notice(opersvs.nick, origin, "Server NOOP list (%d entries):", noop_server_list.count);
			notice(opersvs.nick, origin, " ");
			notice(opersvs.nick, origin, "Entry Hostmask                        Adder                 Reason");
			notice(opersvs.nick, origin, "----- ------------------------------- --------------------- --------------------------");

			LIST_FOREACH(n, noop_server_list.head)
			{
				np = n->data;

				notice(opersvs.nick, origin, "%-5d %-31s %-21s %s", i, np->target, np->added_by, np->reason);
				i++;
			}

			notice(opersvs.nick, origin, "----- ------------------------------- --------------------- --------------------------");
			notice(opersvs.nick, origin, "End of Server NOOP list.");
		}
		else
		{
			notice(opersvs.nick, origin, "Insufficient parameters provided for \2NOOP\2.");
			notice(opersvs.nick, origin, "Syntax: NOOP LIST <HOSTMASK|SERVER>");
		}			
	}
}
