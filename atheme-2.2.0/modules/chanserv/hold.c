/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Controls noexpire options for channels.
 *
 * $Id: hold.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/hold", FALSE, _modinit, _moddeinit,
	"$Id: hold.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_hold(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_hold = { "HOLD", N_("Prevents a channel from expiring."),
			PRIV_HOLD, 2, cs_cmd_hold };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	command_add(&cs_hold, cs_cmdtree);
	help_addentry(cs_helptree, "HOLD", "help/cservice/hold", NULL);
}

void _moddeinit()
{
	command_delete(&cs_hold, cs_cmdtree);
	help_delentry(cs_helptree, "HOLD");
}

static void cs_cmd_hold(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *action = parv[1];
	mychan_t *mc;

	if (!target || !action)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HOLD");
		command_fail(si, fault_needmoreparams, _("Usage: HOLD <#channel> <ON|OFF>"));
		return;
	}

	if (*target != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HOLD");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}
	
	if (!strcasecmp(action, "ON"))
	{
		if (mc->flags & MC_HOLD)
		{
			command_fail(si, fault_nochange, _("\2%s\2 is already held."), target);
			return;
		}

		mc->flags |= MC_HOLD;

		wallops("%s set the HOLD option for the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "%s HOLD ON", mc->name);
		command_success_nodata(si, _("\2%s\2 is now held."), target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!(mc->flags & MC_HOLD))
		{
			command_fail(si, fault_nochange, _("\2%s\2 is not held."), target);
			return;
		}

		mc->flags &= ~MC_HOLD;

		wallops("%s removed the HOLD option on the channel \2%s\2.", get_oper_name(si), target);
		logcommand(si, CMDLOG_ADMIN, "%s HOLD OFF", mc->name);
		command_success_nodata(si, _("\2%s\2 is no longer held."), target);
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HOLD");
		command_fail(si, fault_badparams, _("Usage: HOLD <#channel> <ON|OFF>"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
