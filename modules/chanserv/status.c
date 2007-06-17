/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 8027 2007-04-02 10:47:18Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/status", FALSE, _modinit, _moddeinit,
	"$Id: status.c 8027 2007-04-02 10:47:18Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_status(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_status = { "STATUS", N_("Displays your status in services."),
                         AC_NONE, 1, cs_cmd_status };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_status, cs_cmdtree);
	help_addentry(cs_helptree, "STATUS", "help/cservice/status", NULL);
}

void _moddeinit()
{
	command_delete(&cs_status, cs_cmdtree);
	help_delentry(cs_helptree, "STATUS");
}

static void cs_cmd_status(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];

	if (chan)
	{
		mychan_t *mc = mychan_find(chan);
		unsigned int flags;

		if (*chan != '#')
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "STATUS");
			return;
		}

		if (!mc)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), chan);
			return;
		}

		logcommand(si, CMDLOG_GET, "%s STATUS", mc->name);
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
			return;
		}

		if (is_founder(mc, si->smu))
			command_success_nodata(si, _("You are founder on \2%s\2."), mc->name);

		flags = chanacs_source_flags(mc, si);
		if (flags & CA_AKICK && !(flags & CA_REMOVE))
			command_success_nodata(si, _("You are banned from \2%s\2."), mc->name);
		else if (flags != 0)
		{
			command_success_nodata(si, _("You have access flags \2%s\2 on \2%s\2."), bitmask_to_flags(flags, chanacs_flags), mc->name);
		}
		else
			command_success_nodata(si, _("You have no special access to \2%s\2."), mc->name);

		return;
	}

	logcommand(si, CMDLOG_GET, "STATUS");
	if (!si->smu)
		command_success_nodata(si, _("You are not logged in."));
	else
	{
		command_success_nodata(si, _("You are logged in as \2%s\2."), si->smu->name);

		if (is_soper(si->smu))
		{
			soper_t *soper = si->smu->soper;

			command_success_nodata(si, _("You are a services operator of class %s."), soper->operclass ? soper->operclass->name : soper->classname);
		}
	}

	if (si->su != NULL)
	{
		mynick_t *mn;

		mn = mynick_find(si->su->nick);
		if (mn != NULL && mn->owner != si->smu &&
				myuser_access_verify(si->su, mn->owner))
			command_success_nodata(si, _("You are recognized as \2%s\2."), mn->owner->name);
	}

	if (si->su != NULL && is_admin(si->su))
		command_success_nodata(si, _("You are a server administrator."));

	if (si->su != NULL && is_ircop(si->su))
		command_success_nodata(si, _("You are an IRC operator."));
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
