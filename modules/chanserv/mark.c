/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for channels.
 *
 * $Id: mark.c 6639 2006-10-02 15:44:53Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/mark", FALSE, _modinit, _moddeinit,
	"$Id: mark.c 6639 2006-10-02 15:44:53Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_mark(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_mark = { "MARK", "Adds a note to a channel.",
			PRIV_MARK, 3, cs_cmd_mark };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_mark, cs_cmdtree);
	help_addentry(cs_helptree, "MARK", "help/cservice/mark", NULL);
}

void _moddeinit()
{
	command_delete(&cs_mark, cs_cmdtree);
	help_delentry(cs_helptree, "MARK");
}

static void cs_cmd_mark(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	char *info = parv[2];
	mychan_t *mc;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
		command_fail(si, fault_needmoreparams, "Usage: MARK <#channel> <ON|OFF> [note]");
		return;
	}

	if (target[0] != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", target);
		return;
	}
	
	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "MARK");
			command_fail(si, fault_needmoreparams, "Usage: MARK <#channel> ON <note>");
			return;
		}

		if (metadata_find(mc, METADATA_CHANNEL, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, "\2%s\2 is already marked.", target);
			return;
		}

		metadata_add(mc, METADATA_CHANNEL, "private:mark:setter", get_oper_name(si));
		metadata_add(mc, METADATA_CHANNEL, "private:mark:reason", info);
		metadata_add(mc, METADATA_CHANNEL, "private:mark:timestamp", itoa(CURRTIME));

		wallops("%s marked the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "%s MARK ON", mc->name);
		command_success_nodata(si, "\2%s\2 is now marked.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, METADATA_CHANNEL, "private:mark:setter"))
		{
			command_fail(si, fault_nochange, "\2%s\2 is not marked.", target);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, "private:mark:setter");
		metadata_delete(mc, METADATA_CHANNEL, "private:mark:reason");
		metadata_delete(mc, METADATA_CHANNEL, "private:mark:timestamp");

		wallops("%s unmarked the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "%s MARK OFF", mc->name);
		command_success_nodata(si, "\2%s\2 is now unmarked.", target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MARK");
		command_fail(si, fault_badparams, "Usage: MARK <#channel> <ON|OFF> [note]");
	}
}
