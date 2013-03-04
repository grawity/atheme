/*
 * Copyright (c) 2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ SETPASS function.
 *
 * $Id: setpass.c 7947 2007-03-15 18:47:51Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/setpass", FALSE, _modinit, _moddeinit,
	"$Id: setpass.c 7947 2007-03-15 18:47:51Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void clear_setpass_key(void *vptr);
static void ns_cmd_setpass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_setpass = { "SETPASS", N_("Changes a password using an authcode."), AC_NONE, 3, ns_cmd_setpass };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	hook_add_event("user_identify");
	hook_add_hook("user_identify", clear_setpass_key);
	command_add(&ns_setpass, ns_cmdtree);
	help_addentry(ns_helptree, "SETPASS", "help/nickserv/setpass", NULL);
}

void _moddeinit()
{
	hook_del_hook("user_identify", clear_setpass_key);
	command_delete(&ns_setpass, ns_cmdtree);
	help_delentry(ns_helptree, "SETPASS");
}

static void ns_cmd_setpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	metadata_t *md;
	char *nick = parv[0];
	char *key = parv[1];
	char *password = parv[2];

	if (!nick || !key || !password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (strchr(password, ' '))
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), nick);
		return;
	}

	if (strlen(password) > 32)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		return;
	}

	if (!strcasecmp(password, mu->name))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	md = metadata_find(mu, METADATA_USER, "private:setpass:key");
	if (md != NULL && crypt_verify_password(key, md->value))
	{
		snoop("SETPASS:VS: \2%s\2 by \2%s\2", mu->name, get_source_name(si));
		logcommand(si, CMDLOG_SET, "SETPASS %s", mu->name);
		set_password(mu, password);
		metadata_delete(mu, METADATA_USER, "private:setpass:key");

		command_success_nodata(si, _("The password for \2%s\2 has been changed to \2%s\2. Please write this down for future reference."), mu->name, password);

		return;
	}

	if (md != NULL)
	{
		snoop("SETPASS:VF: \2%s\2 by \2%s\2", mu->name, get_source_name(si));
		logcommand(si, CMDLOG_SET, "failed SETPASS (invalid key)");
	}
	command_fail(si, fault_badparams, _("Verification failed. Invalid key for \2%s\2."), 
		mu->name);

	return;
}

static void clear_setpass_key(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu = u->myuser;
	metadata_t *md;

	if (!(md = metadata_find(mu, METADATA_USER, "private:setpass:key")))
		return;

	metadata_delete(mu, METADATA_USER, "private:setpass:key");
	notice(nicksvs.nick, u->nick, "Warning: SENDPASS had been used to mail you a password recovery "
		"key. Since you have identified, that key is no longer valid.");
}
