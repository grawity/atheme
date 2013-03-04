/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ INFO functions.
 *
 * $Id: info.c 357 2002-03-13 17:42:21Z nenolod $
 */

#include "atheme.h"

void ns_info(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *name = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;

	if (!name)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2INFO\2.");
		notice(nicksvs.nick, origin, "Syntax: INFO <nickname>");
		return;
	}

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	snoop("INFO: \2%s\2 by \2%s\2", name, origin);

	tm = *localtime(&mu->registered);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	notice(nicksvs.nick, origin, "Information on \2%s\2", mu->name);

	notice(nicksvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mu->registered));

	if ((!(mu->flags & MU_HIDEMAIL)) || (is_sra(u->myuser) 
		|| is_ircop(u) || u->myuser == mu))
		notice(nicksvs.nick, origin, "Email      : %s", mu->email);

	*buf = '\0';

	if (MU_HIDEMAIL & mu->flags)
		strcat(buf, "HIDEMAIL");

	if (MU_HOLD & mu->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "HOLD");
	}
	if (MU_NEVEROP & mu->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "NEVEROP");
	}
	if (MU_NOOP & mu->flags)
	{
		if (*buf)
			strcat(buf, " ");

		strcat(buf, "NOOP");
	}

	if (*buf)
		notice(nicksvs.nick, origin, "Flags      : %s", buf);
}
