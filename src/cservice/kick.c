/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: kick.c 111 2005-05-28 23:19:53Z nenolod $
 */

#include "atheme.h"

void cs_kick(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2KICK\2.");
		notice(chansvs.nick, origin, "Syntax: KICK <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "No such channel: \2%s\2.", chan);
		return;
	}

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if ((u->myuser && !chanacs_find(mc, u->myuser, CA_REMOVE)) && (!chanacs_find_host(mc, hostbuf, CA_REMOVE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "No such nickname: \2%s\2.", nick);
			return;
		}
	}

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	kick(chansvs.nick, chan, u->nick, reasonbuf);
	chanuser_delete(mc->chan, u);
	notice(chansvs.nick, origin, "\2%s\2 has been kicked from \2%s\2.", u->nick, mc->name);
}

void cs_kickban(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2KICKBAN\2.");
		notice(chansvs.nick, origin, "Syntax: KICKBAN <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "No such channel: \2%s\2.", chan);
		return;
	}

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if ((u->myuser && !chanacs_find(mc, u->myuser, CA_REMOVE)) && (!chanacs_find_host(mc, hostbuf, CA_REMOVE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "No such nickname: \2%s\2.", nick);
			return;
		}
	}

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	kick(chansvs.nick, chan, u->nick, reasonbuf);
	ban(chansvs.nick, chan, u);
	chanuser_delete(mc->chan, u);
	notice(chansvs.nick, origin, "\2%s\2 has been kickbanned from \2%s\2.", u->nick, mc->name);
}
