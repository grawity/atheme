/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * OperServ NOOP command.
 *
 * $Id: noop.c 8027 2007-04-02 10:47:18Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/noop", TRUE, _modinit, _moddeinit,
	"$Id: noop.c 8027 2007-04-02 10:47:18Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

typedef struct noop_ noop_t;

struct noop_ {
	char *target;
	char *added_by;
	char *reason;
};

list_t noop_hostmask_list;
list_t noop_server_list;

static void os_cmd_noop(sourceinfo_t *si, int parc, char *parv[]);
static void check_user(user_t *u);
static BlockHeap *noop_heap;

command_t os_noop = { "NOOP", N_("Restricts IRCop access."), PRIV_NOOP, 4, os_cmd_noop };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_noop, os_cmdtree);
	help_addentry(os_helptree, "NOOP", "help/oservice/noop", NULL);
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
	command_delete(&os_noop, os_cmdtree);
	help_delentry(os_helptree, "NOOP");
	hook_del_hook("user_oper", (void (*)(void *)) check_user);
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
			user_delete(u);
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
			user_delete(u);
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
static void os_cmd_noop(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	noop_t *np;
	char *action = parv[0];
	char *type = parv[1];
	char *mask = parv[2];
	char *reason = parv[3];

	if (!action || !type)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
		command_fail(si, fault_needmoreparams, _("Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]"));
		return;
	}

	if (!strcasecmp(action, "ADD"))
	{
		if (!strcasecmp(type, "HOSTMASK"))
		{
			if (!mask)
                        {
				command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
				command_fail(si, fault_needmoreparams, _("Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]"));
				return;
			}
			if ((np = noop_find(mask, &noop_hostmask_list)))
			{
				command_fail(si, fault_nochange, _("There is already a NOOP entry covering this target."));
				return;
			}

			np = BlockHeapAlloc(noop_heap);

			np->target = sstrdup(mask);
			np->added_by = sstrdup(get_oper_name(si));

			if (reason)
				np->reason = sstrdup(reason);
			else
				np->reason = sstrdup("Abusive operator.");

			n = node_create();
			node_add(np, n, &noop_hostmask_list);

			logcommand(si, CMDLOG_ADMIN, "NOOP ADD HOSTMASK %s %s", np->target, np->reason);
			command_success_nodata(si, _("Added \2%s\2 to the hostmask NOOP list."), mask);

			return;
		}
		else if (!strcasecmp(type, "SERVER"))
		{
			if (!mask)
			{
				command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
				command_fail(si, fault_needmoreparams, _("Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]"));
				return;
			}
			if ((np = noop_find(mask, &noop_server_list)))
			{
				command_fail(si, fault_nochange, _("There is already a NOOP entry covering this target."));
				return;
			}

			np = BlockHeapAlloc(noop_heap);

			np->target = sstrdup(mask);
			np->added_by = sstrdup(get_oper_name(si));

			if (reason)
				np->reason = sstrdup(reason);
			else
				np->reason = sstrdup("Abusive operator.");

			n = node_create();
			node_add(np, n, &noop_server_list);

			logcommand(si, CMDLOG_ADMIN, "NOOP ADD SERVER %s %s", np->target, np->reason);
			command_success_nodata(si, _("Added \2%s\2 to the server NOOP list."), mask);

			return;
		}
		else
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
			command_fail(si, fault_needmoreparams, _("Syntax: NOOP ADD <HOSTMASK|SERVER> <mask> [reason]"));
		}			
	}
	else if (!strcasecmp(action, "DEL"))
	{
		if (!strcasecmp(type, "HOSTMASK"))
		{
			if (!mask)
			{
				command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
				command_fail(si, fault_needmoreparams, _("Syntax: NOOP <ADD|DEL|LIST> <HOSTMASK|SERVER> <mask> [reason]"));
				return;
			}
			if (!(np = noop_find(mask, &noop_hostmask_list)))
			{
				command_fail(si, fault_nosuch_target, _("There is no NOOP hostmask entry for this target."));
				return;
			}

			logcommand(si, CMDLOG_ADMIN, "NOOP DEL HOSTMASK %s", np->target);
			command_success_nodata(si, _("Removed \2%s\2 from the hostmask NOOP list."), np->target);

			n = node_find(np, &noop_hostmask_list);

			free(np->target);
			free(np->added_by);
			free(np->reason);

			node_del(n, &noop_hostmask_list);
			node_free(n);
			BlockHeapFree(noop_heap, np);

			return;
		}
		else if (!strcasecmp(type, "SERVER"))
		{
			if (!(np = noop_find(mask, &noop_server_list)))
			{
				command_fail(si, fault_nosuch_target, _("There is no NOOP server entry for this target."));
				return;
			}

			logcommand(si, CMDLOG_ADMIN, "NOOP DEL SERVER %s", np->target);
			command_success_nodata(si, _("Removed \2%s\2 from the server NOOP list."), np->target);

			n = node_find(np, &noop_server_list);

			free(np->target);
			free(np->added_by);
			free(np->reason);

			node_del(n, &noop_server_list);
			node_free(n);
			BlockHeapFree(noop_heap, np);

			return;
		}
		else
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
			command_fail(si, fault_needmoreparams, _("Syntax: NOOP DEL <HOSTMASK|SERVER> <mask>"));
		}			
	}
	else if (!strcasecmp(action, "LIST"))
	{
		if (!strcasecmp(type, "HOSTMASK"))
		{
			unsigned int i = 1;
			logcommand(si, CMDLOG_GET, "NOOP LIST HOSTMASK");
			command_success_nodata(si, _("Hostmask NOOP list (%d entries):"), noop_hostmask_list.count);
			command_success_nodata(si, " ");
			command_success_nodata(si, _("Entry Hostmask                        Adder                 Reason"));
			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");

			LIST_FOREACH(n, noop_hostmask_list.head)
			{
				np = n->data;

				command_success_nodata(si, "%-5d %-31s %-21s %s", i, np->target, np->added_by, np->reason);
				i++;
			}

			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");
			command_success_nodata(si, _("End of Hostmask NOOP list."));
		}
		else if (!strcasecmp(type, "SERVER"))
		{
			unsigned int i = 1;
			logcommand(si, CMDLOG_GET, "NOOP LIST SERVER");
			command_success_nodata(si, _("Server NOOP list (%d entries):"), noop_server_list.count);
			command_success_nodata(si, " ");
			command_success_nodata(si, _("Entry Hostmask                        Adder                 Reason"));
			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");

			LIST_FOREACH(n, noop_server_list.head)
			{
				np = n->data;

				command_success_nodata(si, "%-5d %-31s %-21s %s", i, np->target, np->added_by, np->reason);
				i++;
			}

			command_success_nodata(si, "----- ------------------------------- --------------------- --------------------------");
			command_success_nodata(si, _("End of Server NOOP list."));
		}
		else
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
			command_fail(si, fault_needmoreparams, _("Syntax: NOOP LIST <HOSTMASK|SERVER>"));
		}			
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
