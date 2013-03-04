/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 3691 2005-11-09 02:21:33Z alambert $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/sendpass", FALSE, _modinit, _moddeinit,
	"$Id: sendpass.c 3691 2005-11-09 02:21:33Z alambert $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_sendpass(char *origin);

command_t ns_sendpass = { "SENDPASS", "Email registration passwords.",
				AC_IRCOP, ns_cmd_sendpass };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	ns_cmdtree = module_locate_symbol("nickserv/main", "ns_cmdtree");
	ns_helptree = module_locate_symbol("nickserv/main", "ns_helptree");
	command_add(&ns_sendpass, ns_cmdtree);
	help_addentry(ns_helptree, "SENDPASS", "help/nickserv/sendpass", NULL);
}

void _moddeinit()
{
	command_delete(&ns_sendpass, ns_cmdtree);
	help_delentry(ns_helptree, "SENDPASS");
}

static void ns_cmd_sendpass(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *name = strtok(NULL, " ");
	char *newpass = NULL;

	if (u == NULL)
		return;

	if (!name)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2SENDPASS\2.");
		notice(nicksvs.nick, origin, "Syntax: SENDPASS <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (is_sra(mu) && !is_sra(u->myuser))
	{
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "failed SENDPASS %s (is SRA)", name);
		notice(nicksvs.nick, origin, "\2%s\2 belongs to a services root administrator; you must be a services root to send the password.", name);
		return;
	}

	/* this is not without controversy... :) */
	if (mu->flags & MU_CRYPTPASS)
	{
		notice(nicksvs.nick, origin, "The password for the nickname \2%s\2 is encrypted; "
						"a new password will be assigned and sent.", name);
		newpass = gen_pw(12);
		set_password(mu, newpass);
	}

	if (sendemail(u, EMAIL_SENDPASS, mu, (newpass == NULL) ? mu->pass : newpass))
	{
		logcommand(nicksvs.me, u, CMDLOG_ADMIN, "SENDPASS %s", name);
		notice(nicksvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);
	}
	else
		notice(nicksvs.nick, origin, "Email send failed.");

	if (newpass != NULL)
		free(newpass);

	return;
}
