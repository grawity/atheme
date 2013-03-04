/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService INFO functions.
 *
 * $Id: info.c 177 2005-05-29 08:05:17Z nenolod $
 */

#include "atheme.h"

void cs_info(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc;
	char *name = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;

	if (!name)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2INFO\2.");
		notice(chansvs.nick, origin, "Syntax: INFO <username|#channel>");
		return;
	}

	if (*name == '#')
	{
		if (!(mc = mychan_find(name)))
		{
			notice(chansvs.nick, origin, "No such channel: \2%s\2.", name);
			return;
		}

		snoop("INFO: \2%s\2 by \2%s\2", name, origin);

		tm = *localtime(&mc->registered);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(chansvs.nick, origin, "Information on \2%s\2", mc->name);

		if (mc->founder->user)
			notice(chansvs.nick, origin, "Founder    : %s (logged in from \2%s\2)", mc->founder->name, mc->founder->user->nick);
		else
			notice(chansvs.nick, origin, "Founder    : %s (not logged in)", mc->founder->name);

		if (mc->successor)
		{
			if (mc->successor->user)
				notice(chansvs.nick, origin, "Successor  : %s (logged in from \2%s\2)", mc->successor->name, mc->successor->user->nick);
			else
				notice(chansvs.nick, origin, "Successor  : %s (not logged in)", mc->successor->name);
		}

		notice(chansvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mc->registered));

		if (mc->mlock_on || mc->mlock_off)
		{
			char params[BUFSIZE];

			*buf = 0;
			*params = 0;

			if (mc->mlock_on)
			{
				strcat(buf, "+");
				strcat(buf, flags_to_string(mc->mlock_on));

				/* add these in manually */
				if (mc->mlock_limit)
				{
					strcat(buf, "l");
					strcat(params, " ");
					strcat(params, itoa(mc->mlock_limit));
				}

				if (mc->mlock_key)
					strcat(buf, "k");
			}

			if (mc->mlock_off)
			{
				strcat(buf, "-");
				strcat(buf, flags_to_string(mc->mlock_off));
			}

			if (*buf)
				notice(chansvs.nick, origin, "Mode lock  : %s %s", buf, (params) ? params : "");
		}

		if (mc->url)
			notice(chansvs.nick, origin, "URL        : %s", mc->url);

		*buf = '\0';

		if (MC_HOLD & mc->flags)
			strcat(buf, "HOLD");

		if (MC_NEVEROP & mc->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "NEVEROP");
		}
		if (MC_SECURE & mc->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "SECURE");
		}
		if (MC_VERBOSE & mc->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "VERBOSE");
		}
		if (MC_STAFFONLY & mc->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "STAFFONLY");
		}

		if (*buf)
			notice(chansvs.nick, origin, "Flags      : %s", buf);
	}
	else
	{
		if (!(mu = myuser_find(name)))
		{
			notice(chansvs.nick, origin, "No such username: \2%s\2.", name);
			return;
		}

		snoop("INFO: \2%s\2 by \2%s\2", name, origin);

		tm = *localtime(&mu->registered);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(chansvs.nick, origin, "Information on \2%s\2", mu->name);

		notice(chansvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mu->registered));

		if ((!(mu->flags & MU_HIDEMAIL)) || (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
			notice(chansvs.nick, origin, "Email      : %s", mu->email);

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
			notice(chansvs.nick, origin, "Flags      : %s", buf);
	}
}
