/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGIN functions.
 *
 * $Id: ns_identify.c 1070 2005-07-20 06:11:27Z w00t $
 */

#include "atheme.h"

static void ns_cmd_identify(char *origin);

command_t ns_identify = { "IDENTIFY", "Identifies to services for a nickname.",
				AC_NONE, ns_cmd_identify };

extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
	command_add(&ns_identify, &ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_identify, &ns_cmdtree);
}

static void ns_cmd_identify(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	chanuser_t *cu;
	chanacs_t *ca;
	node_t *n;
	char *target = strtok(NULL, " ");
	char *password = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;

	if (target && !password)
	{
		password = target;
		target = origin;
	}

	if (!target && !password)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2IDENTIFY\2.");
		notice(nicksvs.nick, origin, "Syntax: IDENTIFY [nick] <password>");
		return;
	}

	if (u->myuser && u->myuser->identified)
	{
		notice(nicksvs.nick, origin, "You are already identified.");
		return;
	}

	mu = myuser_find(target);
	if (!mu)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	if (mu->user)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is already identified to \2%s\2.", mu->user->nick, mu->name);
		return;
	}

	if (!strcmp(password, mu->pass))
	{
		snoop("LOGIN:AS: \2%s\2 to \2%s\2", u->nick, mu->name);

		if (is_sra(mu))
		{
			snoop("SRA: \2%s\2 as \2%s\2", u->nick, mu->name);
			wallops("\2%s\2 is now an SRA.", u->nick);
		}

		u->myuser = mu;
		mu->user = u;
		mu->identified = TRUE;

		notice(nicksvs.nick, origin, "You are now identified for \2%s\2.", u->myuser->name);

		/* check for failed attempts and let them know */
		if (u->myuser->failnum != 0)
		{
			tm = *localtime(&mu->lastlogin);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

			notice(nicksvs.nick, origin, "\2%d\2 failed %s since %s.", u->myuser->failnum, (u->myuser->failnum == 1) ? "login" : "logins", strfbuf);

			tm = *localtime(&mu->lastfailon);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

			notice(nicksvs.nick, origin, "Last failed attempt from: \2%s\2 on %s.", u->myuser->lastfail, strfbuf);

			u->myuser->failnum = 0;
			free(u->myuser->lastfail);
			u->myuser->lastfail = NULL;
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

				if (ircd->uses_owner && should_owner(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, ircd->owner_mchar, CLIENT_NAME(u));
					cu->modes |= ircd->owner_mode;
				}

				if (ircd->uses_protect && should_protect(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, ircd->protect_mchar, CLIENT_NAME(u));
					cu->modes |= ircd->protect_mode;
				}

				if (should_op(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, "+o", CLIENT_NAME(u));
					cu->modes |= CMODE_OP;
				}

				if (ircd->uses_halfops && should_halfop(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, "+h", CLIENT_NAME(u));
					cu->modes |= ircd->halfops_mode;
				}

				if (should_voice(ca->mychan, ca->myuser))
				{
					cmode(chansvs.nick, ca->mychan->name, "+v", CLIENT_NAME(u));
					cu->modes |= CMODE_VOICE;
				}
			}
		}

		/* XXX: ircd_on_login supports hostmasking, we just dont have it yet. */
		ircd_on_login(origin, mu->name, NULL);

		hook_call_event("user_identify", mu);

		return;
	}

	snoop("LOGIN:AF: \2%s\2 to \2%s\2", u->nick, mu->name);

	notice(nicksvs.nick, origin, "Invalid password for \2%s\2.", mu->name);

	/* record the failed attempts */
	if (mu->lastfail)
		free(mu->lastfail);

	strlcpy(buf, u->nick, BUFSIZE);
	strlcat(buf, "!", BUFSIZE);
	strlcat(buf, u->user, BUFSIZE);
	strlcat(buf, "@", BUFSIZE);
	strlcat(buf, u->vhost, BUFSIZE);
	mu->lastfail = sstrdup(buf);
	mu->failnum++;
	mu->lastfailon = CURRTIME;

	if (mu->failnum == 10)
	{
		tm = *localtime(&mu->lastfailon);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
		wallops("Warning: Numerous failed login attempts to \2%s\2. Last attempt received from \2%s\2 on %s.", mu->name, mu->lastfail, strfbuf);
	}
}
