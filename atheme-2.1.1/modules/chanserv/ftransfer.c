/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FTRANSFER function.
 *
 * $Id: ftransfer.c 7175 2006-11-16 18:15:27Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/ftransfer", FALSE, _modinit, _moddeinit,
	"$Id: ftransfer.c 7175 2006-11-16 18:15:27Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_ftransfer(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_ftransfer = { "FTRANSFER", "Forces foundership transfer of a channel.",
                           PRIV_CHAN_ADMIN, 2, cs_cmd_ftransfer };
                                                                                   
list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_ftransfer, cs_cmdtree);
	help_addentry(cs_helptree, "FTRANSFER", "help/cservice/ftransfer", NULL);
}

void _moddeinit()
{
	command_delete(&cs_ftransfer, cs_cmdtree);
	help_delentry(cs_helptree, "FTRANSFER");
}

static void cs_cmd_ftransfer(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *tmu;
	mychan_t *mc;
	char *name = parv[0];
	char *newfndr = parv[1];

	if (!name || !newfndr)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FTRANSFER");
		command_fail(si, fault_needmoreparams, "Syntax: FTRANSFER <#channel> <newfounder>");
		return;
	}

	if (!(tmu = myuser_find_ext(newfndr)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", newfndr);
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", name);
		return;
	}
	
	if (tmu == mc->founder)
	{
		command_fail(si, fault_nochange, "\2%s\2 is already the founder of \2%s\2.", tmu->name, name);
		return;
	}

	/* no maxchans check (intentional -- this is an oper command) */

	snoop("FTRANSFER: %s transferred %s from %s to %s", get_oper_name(si), name, mc->founder->name, tmu->name);
	wallops("%s transferred foundership of %s from %s to %s", get_oper_name(si), name, mc->founder->name, tmu->name);
	logcommand(si, CMDLOG_ADMIN, "%s FTRANSFER from %s to %s", mc->name, mc->founder->name, tmu->name);
	verbose(mc, "Foundership transfer from \2%s\2 to \2%s\2 forced by %s administration.", mc->founder->name, tmu->name, me.netname);
	command_success_nodata(si, "Foundership of \2%s\2 has been transferred from \2%s\2 to \2%s\2.",
		name, mc->founder->name, tmu->name);

	mc->founder = tmu;
	mc->used = CURRTIME;
	chanacs_change_simple(mc, tmu, NULL, CA_FOUNDER_0, 0, CA_ALL);

	/* delete transfer metadata -- prevents a user from stealing it back */
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
	metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");
}
