/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: clear.c 6661 2006-10-04 23:41:37Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear", FALSE, _modinit, _moddeinit,
	"$Id: clear.c 6661 2006-10-04 23:41:37Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear(sourceinfo_t *si, int parc, char *parv[]);
static void cs_help_clear(sourceinfo_t *si);

command_t cs_clear = { "CLEAR", "Channel removal toolkit.",
                        AC_NONE, 3, cs_cmd_clear };

list_t *cs_cmdtree;
list_t *cs_helptree;
list_t cs_clear_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_clear, cs_cmdtree);
	help_addentry(cs_helptree, "CLEAR", NULL, cs_help_clear);
}

void _moddeinit()
{
	command_delete(&cs_clear, cs_cmdtree);

	help_delentry(cs_helptree,  "CLEAR");
}

static void cs_help_clear(sourceinfo_t *si)
{
	command_success_nodata(si, "Help for \2CLEAR\2:");
	command_success_nodata(si, " ");
	command_success_nodata(si, "CLEAR allows you to clear various aspects of a channel.");
	command_success_nodata(si, " ");
	command_help(si, &cs_clear_cmds);
	command_success_nodata(si, " ");
	command_success_nodata(si, "For more information, use \2/msg %s HELP CLEAR \37command\37\2.", si->service->disp);
}

static void cs_cmd_clear(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CLEAR");
		command_fail(si, fault_needmoreparams, "Syntax: CLEAR <#channel> <command> [parameters]");
		return;
	}
	
	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "CLEAR");
		command_fail(si, fault_badparams, "Syntax: CLEAR <#channel> <command> [parameters]");
		return;
	}

	c = command_find(&cs_clear_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", si->service->disp);
		return;
	}

	parv[1] = chan;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}
