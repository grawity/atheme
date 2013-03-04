/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv IGNORE functions
 *
 * $Id: ignore.c 8027 2007-04-02 10:47:18Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/ignore", FALSE, _modinit, _moddeinit,
	"$Id: ignore.c 8027 2007-04-02 10:47:18Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_ignore(sourceinfo_t *si, int parc, char *parv[]);
static void ms_cmd_ignore_add(sourceinfo_t *si, int parc, char *parv[]);
static void ms_cmd_ignore_del(sourceinfo_t *si, int parc, char *parv[]);
static void ms_cmd_ignore_clear(sourceinfo_t *si, int parc, char *parv[]);
static void ms_cmd_ignore_list(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_ignore = { "IGNORE", N_(N_("Ignores memos.")), AC_NONE, 2, ms_cmd_ignore };
command_t ms_ignore_add = { "ADD", N_(N_("Ignores memos from a user.")), AC_NONE, 1, ms_cmd_ignore_add };
command_t ms_ignore_del = { "DEL", N_(N_("Stops ignoring memos from a user.")), AC_NONE, 1, ms_cmd_ignore_del };
command_t ms_ignore_clear = { "CLEAR", N_(N_("Clears your memo ignore list.")), AC_NONE, 1, ms_cmd_ignore_clear };
command_t ms_ignore_list = { "LIST", N_(N_("Shows all users you are ignoring memos from.")), AC_NONE, 1, ms_cmd_ignore_list };

list_t *ms_cmdtree;
list_t *ms_helptree;
list_t ms_ignore_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

	command_add(&ms_ignore, ms_cmdtree);
	help_addentry(ms_helptree, "IGNORE", "help/memoserv/ignore", NULL);

	/* Add sub-commands */
	command_add(&ms_ignore_add, &ms_ignore_cmds);
	command_add(&ms_ignore_del, &ms_ignore_cmds);
	command_add(&ms_ignore_clear, &ms_ignore_cmds);
	command_add(&ms_ignore_list, &ms_ignore_cmds);
}

void _moddeinit()
{
	command_delete(&ms_ignore, ms_cmdtree);
	help_delentry(ms_helptree, "IGNORE");

	/* Delete sub-commands */
	command_delete(&ms_ignore_add, &ms_ignore_cmds);
	command_delete(&ms_ignore_del, &ms_ignore_cmds);
	command_delete(&ms_ignore_clear, &ms_ignore_cmds);
	command_delete(&ms_ignore_list, &ms_ignore_cmds);
}

static void ms_cmd_ignore(sourceinfo_t *si, int parc, char *parv[])
{
	/* Grab args */
	char *cmd = parv[0];
	command_t *c;

	/* Bad/missing arg */
	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "IGNORE");

		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR [account]"));
		return;
	}

	/* User logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	c = command_find(&ms_ignore_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == FALSE) ? "msg " : "", memosvs.me->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void ms_cmd_ignore_add(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *tmu;
	node_t *n;
	char *temp;

	/* Arg check */
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");

		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>"));
		return;
	}

	/* User attempting to ignore themself? */
	if (!irccasecmp(parv[0], si->smu->name))
	{
		command_fail(si, fault_badparams, _("Silly wabbit, you can't ignore yourself."));
		return;
	}

	/* Does the target account exist? */
	if (!(tmu = myuser_find_ext(parv[0])))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
		return;
	}

	/* Ignore list is full */
	if (si->smu->memo_ignores.count >= MAXMSIGNORES)
	{
		command_fail(si, fault_toomany, _("Your ignore list is full, please DEL an account."));
		return;
	}

	/* Iterate through list, make sure target not in it, if last node append */
	LIST_FOREACH(n, si->smu->memo_ignores.head)
	{
		temp = (char *)n->data;

		/* Already in the list */
		if (!irccasecmp(temp, parv[0]))
		{
			command_fail(si, fault_nochange, _("Account \2%s\2 is already in your ignore list."), temp);
			return;
		}
	}

	/* Add to ignore list */
	temp = sstrdup(tmu->name);
	node_add(temp, node_create(), &si->smu->memo_ignores);
	logcommand(si, CMDLOG_SET, "IGNORE ADD %s", tmu->name);
	command_success_nodata(si, _("Account \2%s\2 added to your ignore list."), tmu->name);
	return;
}

static void ms_cmd_ignore_del(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n, *tn;
	char *temp;

	/* Arg check */
	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "IGNORE");
		command_fail(si, fault_needmoreparams, _("Syntax: IGNORE ADD|DEL|LIST|CLEAR <account>"));
		return;
	}

	/* Iterate through list, make sure they're not in it, if last node append */
	LIST_FOREACH_SAFE(n, tn, si->smu->memo_ignores.head)
	{
		temp = (char *)n->data;

		/* Not using list or clear, but we've found our target in the ignore list */
		if (!irccasecmp(temp, parv[0]))
		{
			logcommand(si, CMDLOG_SET, "IGNORE DEL %s", temp);
			command_success_nodata(si, _("Account \2%s\2 removed from ignore list."), temp);
			node_del(n, &si->smu->memo_ignores);
			node_free(n);
			free(temp);

			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 is not in your ignore list."), parv[0]);
	return;
}

static void ms_cmd_ignore_clear(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n, *tn;

	if (LIST_LENGTH(&si->smu->memo_ignores) == 0)
	{
		command_fail(si, fault_nochange, _("Ignore list already empty."));
		return;
	}

	LIST_FOREACH_SAFE(n, tn, si->smu->memo_ignores.head)
	{
		free(n->data);
		node_del(n,&si->smu->memo_ignores);
		node_free(n);
	}

	/* Let them know list is clear */
	command_success_nodata(si, _("Ignore list cleared."));
	logcommand(si, CMDLOG_SET, "IGNORE CLEAR");
	return;
}

static void ms_cmd_ignore_list(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	unsigned int i = 1;

	/* Throw in list header */
	command_success_nodata(si, _("Ignore list:"));
	command_success_nodata(si, "-------------------------");

	/* Iterate through list, make sure they're not in it, if last node append */
	LIST_FOREACH(n, si->smu->memo_ignores.head)
	{
		command_success_nodata(si, "%d - %s", i, (char *)n->data);
		i++;
	}

	/* Ignore list footer */
	if (i == 1)
		command_success_nodata(si, _("list empty"));

	command_success_nodata(si, "-------------------------");
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
