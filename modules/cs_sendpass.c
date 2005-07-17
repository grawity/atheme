/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: cs_sendpass.c 826 2005-07-16 06:22:04Z w00t $
 */

#include "atheme.h"

static void cs_cmd_sendpass(char *origin);

command_t cs_sendpass = { "SENDPASS", "Email registration passwords.",
                           AC_IRCOP, cs_cmd_sendpass };

extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_sendpass, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_sendpass, &cs_cmdtree);
}

static void cs_cmd_sendpass(char *origin)
{
	myuser_t *mu;
	mychan_t *mc;
	char *name = strtok(NULL, " ");

	if (!name)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2SENDPASS\2.");
		notice(chansvs.nick, origin, "Syntax: SENDPASS <username|#channel>");
		return;
	}

	if (*name == '#')
	{
		if (!(mc = mychan_find(name)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
			return;
		}

		if (mc->founder)
		{
			notice(chansvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mc->name, mc->founder->email);

			sendemail(mc->name, mc->pass, 4);

			return;
		}
	}
	else
	{
		if (!(mu = myuser_find(name)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
			return;
		}

		notice(chansvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);

		sendemail(mu->name, mu->pass, 2);

		return;
	}
}
