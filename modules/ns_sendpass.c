/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: ns_sendpass.c 883 2005-07-16 18:05:39Z alambert $
 */

#include "atheme.h"

static void ns_cmd_sendpass(char *origin);

command_t ns_sendpass = { "SENDPASS", "Email registration passwords.",
				AC_IRCOP, ns_cmd_sendpass };

extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
	command_add(&ns_sendpass, &ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_sendpass, &ns_cmdtree);
}

static void ns_cmd_sendpass(char *origin)
{
	myuser_t *mu;
	char *name = strtok(NULL, " ");

	if (!name)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2SENDPASS\2.");
		notice(nicksvs.nick, origin, "Syntax: SENDPASS <username|#channel>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	notice(nicksvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);

	sendemail(mu->name, mu->pass, 2);

	return;
}
