/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService INVITE functions.
 *
 * $Id: invite.c 63 2002-02-26 13:41:53Z nenolod $
 */

#include "atheme.h"

void cs_invite(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2INVITE\2.");
		notice(chansvs.nick, origin, "Syntax: INVITE <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_INVITE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to invite */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "No such nickname: \2%s\2.", nick);
			return;
		}
	}

	cu = chanuser_find(mc->chan, u);
	if (cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already on \2%s\2.", u->nick, mc->name);
		return;
	}

	sts(":%s INVITE %s :%s", chansvs.nick, u->nick, chan);
	notice(chansvs.nick, origin, "\2%s\2 has been invited to \2%s\2.", u->nick, mc->name);
}
