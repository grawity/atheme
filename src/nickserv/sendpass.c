/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 * $Id: sendpass.c 357 2002-03-13 17:42:21Z nenolod $
 */

#include "atheme.h"

void ns_sendpass(char *origin)
{
	myuser_t *mu;
	char *name = strtok(NULL, " ");

	if (!name)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2SENDPASS\2.");
		notice(chansvs.nick, origin, "Syntax: SENDPASS <username|#channel>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(chansvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	notice(chansvs.nick, origin, "The password for \2%s\2 has been sent to \2%s\2.", mu->name, mu->email);

	sendemail(mu->name, mu->pass, 2);

	return;
}
