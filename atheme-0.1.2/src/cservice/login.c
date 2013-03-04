/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGIN functions.
 *
 * $Id: login.c 338 2005-06-05 03:32:45Z nenolod $
 */

#include "atheme.h"

void cs_login(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	chanuser_t *cu;
	chanacs_t *ca;
	node_t *n;
	char *username = strtok(NULL, " ");
	char *password = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;

	if (nicksvs.enable == TRUE)
	{
		notice(chansvs.nick, origin, "Please identify to %s instead. To do so, use /msg %s identify <password>.",
			nicksvs.nick);
		return;
	}

	if (!username || !password)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2LOGIN\2.");
		notice(chansvs.nick, origin, "Syntax: LOGIN <username> <password>");
		return;
	}

	if (u->myuser && u->myuser->identified)
	{
		notice(chansvs.nick, origin, "You are already logged in as \2%s\2.", u->myuser->name);
		notice(chansvs.nick, origin, "Please use the \2LOGOUT\2 command first.");
		return;
	}

	mu = myuser_find(username);
	if (!mu)
	{
		notice(chansvs.nick, origin, "No such username: \2%s\2.", username);
		return;
	}

	if (mu->user)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already logged in as \2%s\2.", mu->user->nick, mu->name);
		return;
	}

	if (!strcmp(password, mu->pass))
	{
		snoop("LOGIN:AS: \2%s\2 to \2%s\2", u->nick, mu->name);

		if (is_sra(mu))
		{
			snoop("SRA: \2%s\2 as \2%s\2", u->nick, mu->name);
			wallops("\2%s\2 (as \2%s\2) is now an SRA.", u->nick, mu->name);
		}

		u->myuser = mu;
		mu->user = u;
		mu->identified = TRUE;

		notice(chansvs.nick, origin, "Authentication successful. You are now logged in as \2%s\2.", u->myuser->name);

		/* check for failed attempts and let them know */
		if (u->myuser->failnum != 0)
		{
			tm = *localtime(&mu->lastlogin);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

			notice(chansvs.nick, origin, "\2%d\2 failed %s since %s.", u->myuser->failnum, (u->myuser->failnum == 1) ? "login" : "logins", strfbuf);

			tm = *localtime(&mu->lastfailon);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

			notice(chansvs.nick, origin, "Last failed attempt from: \2%s\2 on %s.", u->myuser->lastfail, strfbuf);

			u->myuser->failnum = 0;
			free(u->myuser->lastfail);
		}

		mu->lastlogin = CURRTIME;

		/* now we get to check for xOP */
		LIST_FOREACH(n, mu->chanacs.head)
		{
			ca = (chanacs_t *)n->data;

			cu = chanuser_find(ca->mychan->chan, u);
			if (cu)
			{
				if (should_kick(ca->mychan, ca->myuser))
				{
					ban(chansvs.nick, ca->mychan->name, u);
					kick(chansvs.nick, ca->mychan->name, u->nick, "User is banned from this channel");
				}
				else if (should_op(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, "+o", u->nick);
					cu->modes |= CMODE_VOICE;
				}
				else if (should_voice(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, "+v", u->nick);
					cu->modes |= CMODE_OP;
				}
			}

			/* XXX: ircd_on_login supports hostmasking, we just dont have it yet. */
			ircd_on_login(origin, mu->name, NULL);
		}

		return;
	}

	snoop("LOGIN:AF: \2%s\2 to \2%s\2", u->nick, mu->name);

	notice(chansvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);

	/* record the failed attempts */
	if (mu->lastfail)
		free(mu->lastfail);

	strlcpy(buf, u->nick, BUFSIZE);
	strlcat(buf, "!", BUFSIZE);
	strlcat(buf, u->user, BUFSIZE);
	strlcat(buf, "@", BUFSIZE);
	strlcat(buf, u->host, BUFSIZE);
	mu->lastfail = sstrdup(buf);
	mu->failnum++;
	mu->lastfailon = CURRTIME;

	if (mu->failnum == 10)
	{
		tm = *localtime(&mu->lastfailon);

		wallops("Warning: Numerous failed login attempts to \2%s\2. Last attempt " "received from \2%s\2 on %s.", mu->name, mu->lastfail, strfbuf);
	}
}
