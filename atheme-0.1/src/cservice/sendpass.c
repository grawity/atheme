/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 74 2005-05-23 02:33:37Z nenolod $
 */

#include "atheme.h"

void cs_sendpass(char *origin)
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
			notice(chansvs.nick, origin, "No such channel: \2%s\2.", name);
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
			notice(chansvs.nick, origin, "No such username: \2%s\2.", name);
			return;
		}

		notice(chansvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);

		sendemail(mu->name, mu->pass, 2);

		return;
	}
}
