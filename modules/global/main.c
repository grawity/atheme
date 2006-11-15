/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 6657 2006-10-04 21:22:47Z jilles $
 */

#include "atheme.h"
#include "uplink.h" /* XXX direct use of sts() */

DECLARE_MODULE_V1
(
	"global/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 6657 2006-10-04 21:22:47Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

/* global list struct */
struct global_ {
	char *text;
};

list_t gs_cmdtree;
list_t *os_cmdtree;
list_t gs_helptree;
list_t *os_helptree;

static void gs_cmd_global(sourceinfo_t *si, const int parc, char *parv[]);
static void gs_cmd_help(sourceinfo_t *si, const int parc, char *parv[]);

command_t gs_help = {
	"HELP",
	"Displays contextual help information.",
	PRIV_GLOBAL,
	1,
	gs_cmd_help
};
command_t gs_global = {
	"GLOBAL",
	"Sends a global notice.",
	PRIV_GLOBAL,
        1,
	gs_cmd_global
};

/* *INDENT-ON* */

/* HELP <command> [params] */
static void gs_cmd_help(sourceinfo_t *si, const int parc, char *parv[])
{
	char *command = parv[0];

	if (!command)
	{
		command_help(si, &gs_cmdtree);
		command_success_nodata(si, "For more specific help use \2HELP \37command\37\2.");

		return;
	}

	/* take the command through the hash table */
	help_display(si, command, &gs_helptree);
}

/* GLOBAL <parameters>|SEND|CLEAR */
static void gs_cmd_global(sourceinfo_t *si, const int parc, char *parv[])
{
	static BlockHeap *glob_heap = NULL;
	struct global_ *global;
	static list_t globlist;
	node_t *n, *tn;
	tld_t *tld;
	char *params = parv[0];
	static char *sender = NULL;
	boolean_t isfirst;
	char buf[BUFSIZE];

	if (!params)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GLOBAL");
		command_fail(si, fault_needmoreparams, "Syntax: GLOBAL <parameters>|SEND|CLEAR");
		return;
	}

	if (!strcasecmp("CLEAR", params))
	{
		if (!globlist.count)
		{
			command_fail(si, fault_nochange, "No message to clear.");
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

		command_success_nodata(si, "The pending message has been deleted.");

		return;
	}

	if (!strcasecmp("SEND", params))
	{
		if (!globlist.count)
		{
			command_fail(si, fault_nosuch_target, "No message to send.");
			return;
		}

		isfirst = TRUE;
		LIST_FOREACH(n, globlist.head)
		{
			global = (struct global_ *)n->data;

			snprintf(buf, sizeof buf, "[Network Notice] %s%s%s",
					isfirst ? si->su->nick : "",
					isfirst ? " - " : "",
					global->text);
			/* Cannot use si->service->me here, global notices
			 * should come from global even if /os global was
			 * used. */
			notice_global_sts(globsvs.me->me, "*", buf);
			isfirst = FALSE;
			/* log everything */
			logcommand(si, CMDLOG_ADMIN, "GLOBAL %s", global->text);
		}
		logcommand(si, CMDLOG_ADMIN, "GLOBAL (%d lines sent)", LIST_LENGTH(&globlist));

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

		snoop("GLOBAL: \2%s\2", get_oper_name(si));

		command_success_nodata(si, "The global notice has been sent.");

		return;
	}

	if (!glob_heap)
		glob_heap = BlockHeapCreate(sizeof(struct global_), 5);

	if (!sender)
		sender = sstrdup(si->su->nick);

	if (irccasecmp(sender, si->su->nick))
	{
		command_fail(si, fault_noprivs, "There is already a GLOBAL in progress by \2%s\2.", sender);
		return;
	}

	global = BlockHeapAlloc(glob_heap);

	global->text = sstrdup(params);

	n = node_create();
	node_add(global, n, &globlist);

	command_success_nodata(si,
		"Stored text to be sent as line %d. Use \2GLOBAL SEND\2 "
		"to send message, \2GLOBAL CLEAR\2 to delete the pending message, " "or \2GLOBAL\2 to store additional lines.", globlist.count);
}

/* main services client routine */
static void gservice(sourceinfo_t *si, int parc, char *parv[])
{
	char orig[BUFSIZE];
	char *cmd;
        char *text;

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
        text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(cmd, text, si->su->nick, globsvs.nick);
		return;
	}

	command_exec_split(si->service, si, cmd, text, &gs_cmdtree);
}

static void global_config_ready(void *unused)
{
	if (globsvs.me)
		del_service(globsvs.me);

	globsvs.me = add_service(globsvs.nick, globsvs.user,
			globsvs.host, globsvs.real,
			gservice, &gs_cmdtree);
	globsvs.disp = globsvs.me->disp;

	hook_del_hook("config_ready", global_config_ready);
}

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	hook_add_event("config_ready");
	hook_add_hook("config_ready", global_config_ready);

	if (!cold_start)
	{
		globsvs.me = add_service(globsvs.nick, globsvs.user,
				globsvs.host, globsvs.real,
				gservice, &gs_cmdtree);
		globsvs.disp = globsvs.me->disp;
	}

	command_add(&gs_global, &gs_cmdtree);

	if (os_cmdtree)
		command_add(&gs_global, os_cmdtree);

	if (os_helptree)
		help_addentry(os_helptree, "GLOBAL", "help/gservice/global", NULL);

	help_addentry(&gs_helptree, "HELP", "help/help", NULL);
	help_addentry(&gs_helptree, "GLOBAL", "help/gservice/global", NULL);

	command_add(&gs_help, &gs_cmdtree);
}

void _moddeinit(void)
{
	if (globsvs.me)
	{
		del_service(globsvs.me);
		globsvs.me = NULL;
	}

	command_delete(&gs_global, &gs_cmdtree);

	if (os_cmdtree)
		command_delete(&gs_global, os_cmdtree);

	if (os_helptree)
		help_delentry(os_helptree, "GLOBAL");

	help_delentry(&gs_helptree, "GLOBAL");
	help_delentry(&gs_helptree, "HELP");

	command_delete(&gs_help, &gs_cmdtree);
}

