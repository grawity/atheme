/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGIN functions.
 *
 * $Id: ghost.c 333 2005-06-04 22:05:33Z nenolod $
 */

#include "atheme.h"

void ns_ghost(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *target = strtok(NULL, " ");
	char *password = strtok(NULL, " ");

	if (!password)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2GHOST\2.");
		notice(nicksvs.nick, origin, "Syntax: GHOST <target> <password>");
		return;
	}

	mu = myuser_find(target);
	if (!mu)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	if (!strcmp(password, mu->pass))
	{
		snoop("GHOST: \2%s\2 by \2%s\2", mu->name, u->nick);

		mu->user = NULL;

		skill(nicksvs.nick, target, "GHOSTed by %s", origin);

		notice(nicksvs.nick, origin, "\2%s\2 has been ghosted.", origin);

		mu->lastlogin = CURRTIME;

		return;
	}

	snoop("GHOST:AF: \2%s\2 to \2%s\2", u->nick, mu->name);

	notice(nicksvs.nick, origin, "Invalid password for \2%s\2.", mu->name);
}
