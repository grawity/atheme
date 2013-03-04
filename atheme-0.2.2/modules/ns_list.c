/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ LIST function.
 * Based on Alex Lambert's LISTEMAIL.
 *
 * $Id: ns_list.c 821 2005-07-16 05:10:40Z w00t $
 */

#include "atheme.h"

static void ns_cmd_list(char *origin);

command_t ns_list = { "LIST", "Lists nicknames registered matching a given pattern.", AC_IRCOP, ns_cmd_list };

extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
	command_add(&ns_list, &ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_list, &ns_cmdtree);
}

static void ns_cmd_list(char *origin)
{
	myuser_t *mu;
	node_t *n;
	char *nickpattern = strtok(NULL, " ");
	uint32_t i;
	uint32_t matches = 0;

	if (!nickpattern)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2LIST\2.");
		notice(nicksvs.nick, origin, "Syntax: LIST <nickname pattern>");
		return;
	}

	wallops("\2%s\2 is searching the nickname database for nicknames matching \2%s\2", origin, nickpattern);

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			mu = (myuser_t *)n->data;

			if (!match(nickpattern, mu->name))
			{
				/* in the future we could add a LIMIT parameter */
				if (matches == 0)
					notice(nicksvs.nick, origin, "Nicknames matching pattern \2%s\2:", nickpattern);
				notice(nicksvs.nick, origin, "- %s (%s)", mu->name, mu->email);
				matches++;
			}
		}
	}

	if (matches == 0)
		notice(nicksvs.nick, origin, "No nicknames matched pattern \2%s\2", nickpattern);
	else
		notice(nicksvs.nick, origin, "\2%d\2 match%s for pattern \2%s\2", matches, matches != 1 ? "es" : "", nickpattern);
}
