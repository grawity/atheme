/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ DROP function.
 *
 * $Id: drop.c 327 2005-06-04 21:09:40Z nenolod $
 */

#include "atheme.h"

void ns_drop(char *origin)
{
	uint32_t i;
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *tmc;
	node_t *n;
	char *pass = strtok(NULL, " ");

	if (!pass)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2DROP\2.");
		notice(nicksvs.nick, origin, "Syntax: DROP <password>");
		return;
	}

	if (!(mu = myuser_find(origin)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", origin);
		return;
	}

	if (strcmp(pass, mu->pass))
	{
		notice(nicksvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
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

				notice(nicksvs.nick, origin, "The channel \2%s\2 has been dropped.", tmc->name);

				part(tmc->name, chansvs.nick);
				mychan_delete(tmc->name);
			}
		}
	}

	snoop("DROP: \2%s\2 by \2%s\2", mu->name, u->nick);
	if (u->myuser == mu)
		u->myuser = NULL;
	myuser_delete(mu->name);
	notice(nicksvs.nick, origin, "The nickname \2%s\2 has been dropped.", origin);
	return;
}
