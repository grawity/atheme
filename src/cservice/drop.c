/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService DROP function.
 *
 * $Id: drop.c 63 2002-02-26 13:41:53Z nenolod $
 */

#include "atheme.h"

void cs_drop(char *origin)
{
	uint32_t i;
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc, *tmc;
	node_t *n;
	char *name = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if (!name || !pass)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2DROP\2.");
		notice(chansvs.nick, origin, "Syntax: DROP <username|#channel> <password>");
		return;
	}

	if (*name == '#')
	{
		/* we know it's a channel to DROP now */

		if (!(mc = mychan_find(name)))
		{
			notice(chansvs.nick, origin, "No such channel: \2%s\2.", name);
			return;
		}

		if (!is_founder(mc, u->myuser))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		if (strcmp(pass, mc->pass))
		{
			notice(chansvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mc->name);
			return;
		}

		snoop("DROP: \2%s\2 by \2%s\2 as \2%s\2", mc->name, u->nick, u->myuser->name);

		part(mc->name, chansvs.nick);
		mychan_delete(mc->name);

		notice(chansvs.nick, origin, "The channel \2%s\2 has been dropped.", name);
		return;
	}

	else
	{
		/* we know it's a username to DROP now */

		if (!(mu = myuser_find(name)))
		{
			notice(chansvs.nick, origin, "No such username: \2%s\2.", name);
			return;
		}

		if (strcmp(pass, mu->pass))
		{
			notice(chansvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
			return;
		}

		/* find all channels that are theirs and drop them */
		for (i = 1; i < HASHSIZE; i++)
		{
			LIST_FOREACH(n, mclist[i].head)
			{
				tmc = (mychan_t *)n->data;
				if (tmc->founder == mu)
				{
					snoop("DROP: \2%s\2 by \2%s\2 as \2%s\2", tmc->name, u->nick, u->myuser->name);

					notice(chansvs.nick, origin, "The channel \2%s\2 has been dropped.", tmc->name);

					part(tmc->name, chansvs.nick);
					mychan_delete(tmc->name);
				}
			}
		}

		snoop("DROP: \2%s\2 by \2%s\2", mu->name, u->nick);
		if (u->myuser == mu)
			u->myuser = NULL;
		myuser_delete(mu->name);
		notice(chansvs.nick, origin, "The username \2%s\2 has been dropped.", name);
		return;
	}
}
