/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: op.c 101 2005-05-28 19:22:02Z nenolod $
 */

#include "atheme.h"

void cs_op(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2OP\2.");
		notice(chansvs.nick, origin, "Syntax: OP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "No such channel: \2%s\2.", chan);
		return;
	}

	hostbuf[0] = '\0';

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if ((u->myuser && !chanacs_find(mc, u->myuser, CA_OP)) && (!chanacs_find_host(mc, hostbuf, CA_OP)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to op */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "No such nickname: \2%s\2.", nick);
			return;
		}
	}

	if (!chanacs_find_host(mc, hostbuf, CA_OP))
	{
		if ((MC_SECURE & mc->flags) && (!u->myuser))
		{
			notice(chansvs.nick, origin, "The \2SECURE\2 flag is set for \2%s\2.", mc->name);
			return;
		}
		else if ((MC_SECURE & mc->flags) && (!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_OP)))
		{
			notice(chansvs.nick, origin, "\2%s\2 could not be opped on \2%s\2.", u->nick, mc->name);
			return;
		}
	}

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	if (CMODE_OP & cu->modes)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already opped on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "+o", u->nick);
	cu->modes |= CMODE_OP;
	notice(chansvs.nick, origin, "\2%s\2 has been opped on \2%s\2.", u->nick, mc->name);
}

void cs_deop(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2DEOP\2.");
		notice(chansvs.nick, origin, "Syntax: DEOP <#channel> [nickname]");
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

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_OP)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to deop */
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

	if (!(CMODE_OP & cu->modes))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not opped on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "-o", u->nick);
	cu->modes &= ~CMODE_OP;
	notice(chansvs.nick, origin, "\2%s\2 has been deopped on \2%s\2.", u->nick, mc->name);
}
