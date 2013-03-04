/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ DROP function.
 *
 * $Id: ns_drop.c 1072 2005-07-20 06:33:32Z w00t $
 */

#include "atheme.h"

static void ns_cmd_drop(char *origin);

command_t ns_drop = { "DROP", "Drops a nickname registration.", AC_NONE, ns_cmd_drop };

extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
	command_add(&ns_drop, &ns_cmdtree);
}

void _moddeinit()
{
	command_add(&ns_drop, &ns_cmdtree);
}

static void ns_cmd_drop(char *origin)
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
	hook_call_event("user_drop", mu);
	ircd_on_logout(origin, u->myuser->name, NULL);
	if (u->myuser == mu)
		u->myuser = NULL;
	notice(nicksvs.nick, origin, "The nickname \2%s\2 has been dropped.", mu->name);
	myuser_delete(mu->name);
}
