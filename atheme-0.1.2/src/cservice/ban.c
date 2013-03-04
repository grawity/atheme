/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService BAN/UNBAN function.
 *
 * $Id: ban.c 207 2005-05-29 20:41:31Z nenolod $
 */

#include "atheme.h"

void cs_ban (char *origin)
{
	char *channel = strtok(NULL, " ");
	char *target = strtok(NULL, " ");
	channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	chanacs_t *ca;
	user_t *u = user_find(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, "Insufficient parameters provided for \2BAN\2.");
		notice(chansvs.nick, origin, "Syntax: BAN <#channel> <nickname|hostmask>");
		return;
	}

	if (!c)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", channel);
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!(ca = chanacs_find(mc, u->myuser, CA_REMOVE)) && !(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (validhostmask(target))
	{
		cmode(chansvs.nick, c->name, "+b", target);
		chanban_add(c, target);
		notice(chansvs.nick, origin, "Banned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else if ((tu = user_find(target)))
	{
		char hostbuf[BUFSIZE];

		hostbuf[0] = '\0';

		strlcat(hostbuf, "*!*@", BUFSIZE);
		strlcat(hostbuf, tu->host, BUFSIZE);

		cmode(chansvs.nick, c->name, "+b", hostbuf);
		chanban_add(c, hostbuf);
		notice(chansvs.nick, origin, "Banned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
	else
	{
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: BAN <#channel> <nickname|hostmask>");
		return;
	}
}

void cs_unban (char *origin)
{
        char *channel = strtok(NULL, " ");
        char *target = strtok(NULL, " ");
        channel_t *c = channel_find(channel);
	mychan_t *mc = mychan_find(channel);
	chanacs_t *ca;
	user_t *u = user_find(origin);
	user_t *tu;

	if (!channel || !target)
	{
		notice(chansvs.nick, origin, "Insufficient parameters provided for \2UNBAN\2.");
		notice(chansvs.nick, origin, "Syntax: UNBAN <#channel> <nickname|hostmask>");
		return;
	}

	if (!c)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 does not exist.", channel);
		return;
	}

	if (!mc)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", channel);
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!(ca = chanacs_find(mc, u->myuser, CA_REMOVE)) && !(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (validhostmask(target))
	{
		chanban_t *cb = chanban_find(c, target);

		if (cb)
		{
			cmode(chansvs.nick, c->name, "-b", target);
			chanban_delete(cb);
			notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		}

		return;
	}
	else if ((tu = user_find(target)))
	{
		node_t *n;
		char hostbuf[BUFSIZE];

		strlcat(hostbuf, tu->nick, BUFSIZE);
		strlcat(hostbuf, "!", BUFSIZE);
		strlcat(hostbuf, tu->user, BUFSIZE);
		strlcat(hostbuf, "@", BUFSIZE);
		strlcat(hostbuf, tu->host, BUFSIZE);

		LIST_FOREACH(n, c->bans.head)
		{
			chanban_t *cb = n->data;

			slog(LG_DEBUG, "cs_unban(): iterating %s on %s", cb->mask, c->name);

			if (!match(cb->mask, hostbuf))
			{
				cmode(chansvs.nick, c->name, "-b", cb->mask);
				chanban_delete(cb);
			}
		}
		notice(chansvs.nick, origin, "Unbanned \2%s\2 on \2%s\2.", target, channel);
		return;
	}
        else
        {
		notice(chansvs.nick, origin, "Invalid nickname/hostmask provided: \2%s\2", target);
		notice(chansvs.nick, origin, "Syntax: UNBAN <#channel> <nickname|hostmask>");
		return;
        }
}
