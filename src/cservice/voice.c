/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService VOICE functions.
 *
 * $Id: voice.c 111 2005-05-28 23:19:53Z nenolod $
 */

#include "atheme.h"

void cs_voice(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2VOICE\2.");
		notice(chansvs.nick, origin, "Syntax: VOICE <#channel> [nickname]");
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

	if ((u->myuser && !chanacs_find(mc, u->myuser, CA_VOICE)) && (!chanacs_find_host(mc, hostbuf, CA_VOICE)))
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

	if (CMODE_VOICE & cu->modes)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already voiced on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "+v", u->nick);
	cu->modes |= CMODE_VOICE;
	notice(chansvs.nick, origin, "\2%s\2 has been voiced on \2%s\2.", u->nick, mc->name);
}

void cs_devoice(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2DEVOICE\2.");
		notice(chansvs.nick, origin, "Syntax: DEVOICE <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "No such channel: \2%s\2.", chan);
		return;
	}

	u = user_find(origin);
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_VOICE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to devoice */
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

	if (!(CMODE_VOICE & cu->modes))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not voiced on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "-v", u->nick);
	cu->modes &= ~CMODE_VOICE;
	notice(chansvs.nick, origin, "\2%s\2 has been devoiced on \2%s\2.", u->nick, mc->name);
}
