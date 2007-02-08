/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ GHOST function.
 *
 * $Id: ghost.c 7179 2006-11-17 19:58:40Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/ghost", FALSE, _modinit, _moddeinit,
	"$Id: ghost.c 7179 2006-11-17 19:58:40Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_ghost(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_ghost = { "GHOST", "Reclaims use of a nickname.", AC_NONE, 2, ns_cmd_ghost };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_ghost, ns_cmdtree);
	help_addentry(ns_helptree, "GHOST", "help/nickserv/ghost", NULL);
}

void _moddeinit()
{
	command_delete(&ns_ghost, ns_cmdtree);
	help_delentry(ns_helptree, "GHOST");
}

void ns_cmd_ghost(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *target = parv[0];
	char *password = parv[1];
	user_t *target_u;
	mynick_t *mn;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "GHOST");
		command_fail(si, fault_needmoreparams, "Syntax: GHOST <target> [password]");
		return;
	}

	if (nicksvs.no_nick_ownership)
		mn = NULL, mu = myuser_find(target);
	else
	{
		mn = mynick_find(target);
		mu = mn != NULL ? mn->owner : NULL;
	}
	target_u = user_find_named(target);
	if (!mu && (!target_u || !target_u->myuser))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	if (!target_u)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not online.", target);
		return;
	}
	else if (target_u == si->su)
	{
		command_fail(si, fault_badparams, "You may not ghost yourself.");
		return;
	}

	if ((target_u->myuser && target_u->myuser == si->smu) || /* they're identified under our account */
			(!nicksvs.no_nick_ownership && mn && mu == si->smu) || /* we're identified under their nick's account */
			(!nicksvs.no_nick_ownership && password && mn && verify_password(mu, password))) /* we have their nick's password */
	{
		/* If we're ghosting an unregistered nick, mu will be unset,
		 * however if it _is_ registered, we still need to set it or
		 * the wrong user will have their last seen time updated... */
		if(target_u->myuser && target_u->myuser == si->smu)
			mu = target_u->myuser;

		logcommand(si, CMDLOG_DO, "GHOST %s!%s@%s", target_u->nick, target_u->user, target_u->vhost);

		skill(nicksvs.nick, target, "GHOST command used by %s", get_source_mask(si));
		user_delete(target_u);

		command_success_nodata(si, "\2%s\2 has been ghosted.", target);

		/* don't update the nick's last seen time */
		mu->lastlogin = CURRTIME;

		return;
	}

	if (password && mu)
	{
		logcommand(si, CMDLOG_DO, "failed GHOST %s (bad password)", target);
		command_fail(si, fault_authfail, "Invalid password for \2%s\2.", mu->name);
	}
	else
	{
		logcommand(si, CMDLOG_DO, "failed GHOST %s (invalid login)", target);
		command_fail(si, fault_noprivs, "You may not ghost \2%s\2.", target);
	}
}
