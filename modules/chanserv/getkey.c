/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService GETKEY functions.
 *
 * $Id: getkey.c 6577 2006-09-30 21:17:34Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/getkey", FALSE, _modinit, _moddeinit,
	"$Id: getkey.c 6577 2006-09-30 21:17:34Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_getkey(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_getkey = { "GETKEY", "Returns the key (+k) of a channel.",
                        AC_NONE, 1, cs_cmd_getkey };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_getkey, cs_cmdtree);
	help_addentry(cs_helptree, "GETKEY", "help/cservice/getkey", NULL);
}

void _moddeinit()
{
	command_delete(&cs_getkey, cs_cmdtree);
	help_delentry(cs_helptree, "GETKEY");
}

static void cs_cmd_getkey(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	mychan_t *mc;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GETKEY");
		command_fail(si, fault_needmoreparams, "Syntax: GETKEY <#channel>");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "Cannot GETKEY: \2%s\2 is closed.", chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_INVITE))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	if (!mc->chan)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is currently empty.", mc->name);
		return;
	}

	if (!mc->chan->key)
	{
		command_fail(si, fault_nosuch_key, "\2%s\2 is not keyed.", mc->name);
		return;
	}
	logcommand(si, CMDLOG_GET, "%s GETKEY", mc->name);
	command_success_string(si, mc->chan->key, "Channel \2%s\2 key is: %s",
			mc->name, mc->chan->key);
}
