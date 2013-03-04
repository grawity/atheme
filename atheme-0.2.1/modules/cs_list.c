/*
 * Copyright (c) 2005 Robin Burchell, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ LIST function.
 *
 * $Id: cs_list.c 953 2005-07-17 22:58:11Z alambert $
 */

#include "atheme.h"

static void cs_cmd_list(char *origin);

command_t cs_list = { "LIST", "Lists channels registered matching a given pattern.", AC_IRCOP, cs_cmd_list };

extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
	command_add(&cs_list, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_list, &cs_cmdtree);
}

static void cs_cmd_list(char *origin)
{
	mychan_t *mc;
	node_t *n;
	char *chanpattern = strtok(NULL, " ");
	uint32_t i;
	uint32_t matches = 0;

	if (!chanpattern)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2LIST\2.");
		notice(chansvs.nick, origin, "Syntax: LIST <channel pattern>");
		return;
	}

	wallops("\2%s\2 is searching the channels database for channels matching \2%s\2", origin, chanpattern);

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			if (!match(chanpattern, mc->name))
			{
				/* in the future we could add a LIMIT parameter */
				if (matches == 0)
					notice(chansvs.nick, origin, "Channels matching pattern \2%s\2:", chanpattern);
				notice(chansvs.nick, origin, "- %s (%s)", mc->name, mc->founder->name);
				matches++;
			}
		}
	}

	if (matches == 0)
		notice(chansvs.nick, origin, "No channel matched pattern \2%s\2", chanpattern);
	else
		notice(chansvs.nick, origin, "\2%d\2 match%s for pattern \2%s\2", matches, matches != 1 ? "es" : "", chanpattern);
}
