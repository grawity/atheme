/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FFLAGS functions.
 *
 * $Id: fflags.c 7171 2006-11-15 19:27:49Z jilles $
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/fflags", FALSE, _modinit, _moddeinit,
	"$Id: fflags.c 7171 2006-11-15 19:27:49Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_fflags(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_fflags = { "FFLAGS", "Forces a flags change on a channel.",
                        PRIV_CHAN_ADMIN, 3, cs_cmd_fflags };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_fflags, cs_cmdtree);
	help_addentry(cs_helptree, "FFLAGS", "help/cservice/fflags", NULL);
}

void _moddeinit()
{
	command_delete(&cs_fflags, cs_cmdtree);
	help_delentry(cs_helptree, "FFLAGS");
}

/* FFLAGS <channel> <user> <flags> */
static void cs_cmd_fflags(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	node_t *n;
	char *channel = parv[0];
	char *target = parv[1];
	char *flagstr = parv[2];
	mychan_t *mc = mychan_find(channel);
	myuser_t *tmu;
	const char *str1;
	uint32_t addflags, removeflags;

	if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FFLAGS");
		command_fail(si, fault_needmoreparams, "Syntax: FFLAGS <channel> <target> <flags>");
		return;
	}


	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", channel);
		return;
	}
	
	if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
	{
		flags_make_bitmasks(flagstr, chanacs_flags, &addflags, &removeflags);
		if (addflags == 0 && removeflags == 0)
		{
			command_fail(si, fault_badparams, "No valid flags given, use /%s%s HELP FLAGS for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp);
			return;
		}
	}
	else
	{
		addflags = get_template_flags(mc, flagstr);
		if (addflags == 0)
		{
			/* Hack -- jilles */
			if (*target == '+' || *target == '-' || *target == '=')
				command_fail(si, fault_badparams, "Usage: FFLAGS %s <target> <flags>", mc->name);
			else
				command_fail(si, fault_badparams, "Invalid template name given, use /%s%s TEMPLATE %s for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp, mc->name);
			return;
		}
		removeflags = CA_ALL & ~addflags;
	}

	if (!validhostmask(target))
	{
		if (!(tmu = myuser_find_ext(target)))
		{
			command_fail(si, fault_nosuch_target, "The nickname \2%s\2 is not registered.", target);
			return;
		}
		target = tmu->name;

		/* This check must stay as it ensures that the founder
		 * is always on access */
		if (tmu == mc->founder && removeflags & CA_FLAGS)
		{
			command_fail(si, fault_noprivs, "You may not remove the founder's +f access.");
			return;
		}

		/* If NEVEROP is set, don't allow adding new entries
		 * except sole +b. Adding flags if the current level
		 * is +b counts as adding an entry.
		 * -- jilles */
		if (MU_NEVEROP & tmu->flags && addflags != CA_AKICK && addflags != 0 && ((ca = chanacs_find(mc, tmu, 0)) == NULL || ca->level == CA_AKICK))
		{
			command_fail(si, fault_noprivs, "\2%s\2 does not wish to be added to access lists (NEVEROP set).", tmu->name);
			return;
		}

		if (!chanacs_change(mc, tmu, NULL, &addflags, &removeflags, CA_ALL))
		{
			/* this shouldn't happen */
			command_fail(si, fault_noprivs, "You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), tmu->name, mc->name);
			return;
		}
	}
	else
	{
		if (!chanacs_change(mc, NULL, target, &addflags, &removeflags, CA_ALL))
		{
			/* this shouldn't happen */
			command_fail(si, fault_noprivs, "You are not allowed to set \2%s\2 on \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), target, mc->name);
			return;
		}
	}

	if ((addflags | removeflags) == 0)
	{
		command_fail(si, fault_nochange, "Channel access to \2%s\2 for \2%s\2 unchanged.", channel, target);
		return;
	}
	flagstr = bitmask_to_flags2(addflags, removeflags, chanacs_flags);
	wallops("\2%s\2 is forcing flags change \2%s\2 on \2%s\2 in \2%s\2.", get_oper_name(si), flagstr, target, mc->name);
	snoop("FFLAGS: \2%s\2 \2%s\2 \2%s\2 by \2%s\2", channel, target, flagstr, get_oper_name(si));
	command_success_nodata(si, "Flags \2%s\2 were set on \2%s\2 in \2%s\2.", flagstr, target, channel);
	logcommand(si, CMDLOG_ADMIN, "%s FFLAGS %s %s", mc->name, target, flagstr);
	verbose(mc, "\2%s\2 forced flags change \2%s\2 on \2%s\2.", get_source_name(si), flagstr, target);
}

