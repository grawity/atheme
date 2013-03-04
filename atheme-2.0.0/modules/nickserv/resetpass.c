/*
 * Copyright (c) 2005 Patrick Fish, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for nickserv RESETPASS
 *
 * $Id: resetpass.c 6631 2006-10-02 10:24:13Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/resetpass", FALSE, _modinit, _moddeinit,
	"$Id: resetpass.c 6631 2006-10-02 10:24:13Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_resetpass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_resetpass = { "RESETPASS", "Resets a nickname password.", PRIV_USER_ADMIN, 1, ns_cmd_resetpass };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_resetpass, ns_cmdtree);
	help_addentry(ns_helptree, "RESETPASS", "help/nickserv/resetpass", NULL);
}

void _moddeinit()
{
	command_delete(&ns_resetpass, ns_cmdtree);
	help_delentry(ns_helptree, "RESETPASS");
}

static void ns_cmd_resetpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char *name = parv[0];
	char *newpass;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "RESETPASS");
		command_fail(si, fault_needmoreparams, "Syntax: RESETPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", name);
		return;
	}

	if (is_soper(mu) && !has_priv(si, PRIV_ADMIN))
	{
		logcommand(si, CMDLOG_ADMIN, "failed RESETPASS %s (is SOPER)", name);
		command_fail(si, fault_badparams, "\2%s\2 belongs to a services operator; you need %s privilege to reset the password.", name, PRIV_ADMIN);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")) && has_priv(si, PRIV_MARK))
	{
		logcommand(si, CMDLOG_ADMIN, "RESETPASS %s (overriding mark by %s)", name, md->value);
		command_success_nodata(si, "Overriding MARK placed by %s on the nickname %s.", md->value, name);
		newpass = gen_pw(12);
		command_success_nodata(si, "The password for the nickname %s has been changed to %s.", name, newpass);
		set_password(mu, newpass);
		free(newpass);
		wallops("%s reset the password for the \2MARKED\2 nickname %s.", get_oper_name(si), name);
		return;
	}

	if ((md = metadata_find(mu, METADATA_USER, "private:mark:setter")))
	{
		logcommand(si, CMDLOG_ADMIN, "failed RESETPASS %s (marked by %s)", name, md->value);
		command_fail(si, fault_badparams, "This operation cannot be performed on %s, because the nickname has been marked by %s.", name, md->value);
		return;
	}

	newpass = gen_pw(12);
	command_success_nodata(si, "The password for the nickname %s has been changed to %s.", name, newpass);
	set_password(mu, newpass);
	free(newpass);

	wallops("%s reset the password for the nickname %s", get_oper_name(si), name);
	snoop("RESETPASS: \2%s\2 reset the password for \2%s\2", get_oper_name(si), name);
	logcommand(si, CMDLOG_ADMIN, "RESETPASS %s", name);
}
