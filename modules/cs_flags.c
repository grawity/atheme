/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FLAGS functions.
 *
 * $Id: cs_flags.c 915 2005-07-17 06:21:26Z nenolod $
 */

#include "atheme.h"

static void cs_cmd_flags(char *origin);
static void cs_fcmd_flags(char *origin, char *channel);

command_t cs_flags = { "FLAGS", "Manipulates specific permissions on a channel.",
                        AC_NONE, cs_cmd_flags };
fcommand_t cs_fantasy_flags = { ".flags", AC_NONE, cs_fcmd_flags };

extern list_t cs_cmdtree;
extern list_t cs_fcmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_flags, &cs_cmdtree);
	fcommand_add(&cs_fantasy_flags, &cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_flags, &cs_cmdtree);
	fcommand_delete(&cs_fantasy_flags, &cs_fcmdtree);
}

/* FLAGS <channel> [user] [flags] */
static void cs_cmd_flags(char *origin)
{
	user_t *u = user_find(origin);
	chanacs_t *ca;
	node_t *n;

	char *channel = strtok(NULL, " ");
	char *target = strtok(NULL, " ");

	if (!channel)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2FLAGS\2.");
		notice(chansvs.nick, origin, "Syntax: FLAGS <channel> [target] [flags]");
		return;
	}

	if (!target)
	{
		mychan_t *mc = mychan_find(channel);
		uint8_t i = 1;

		if (!mc)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", channel);
			return;
		}

		if (!is_founder(mc, u->myuser) && !is_successor(mc, u->myuser) && !is_xop(mc, u->myuser, CA_ACLVIEW))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		notice(chansvs.nick, origin, "Entry Account/Host           Flags");
		notice(chansvs.nick, origin, "----- ---------------------- -----");

		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = n->data;
			notice(chansvs.nick, origin, "%-5d %-22s %s", i, ca->host ? ca->host : ca->myuser->name, bitmask_to_flags(ca->level, chanacs_flags));
			i++;
		}

		notice(chansvs.nick, origin, "----- ---------------------- -----");
		notice(chansvs.nick, origin, "End of \2%s\2 FLAGS listing.", channel);
	}
	else
	{
		mychan_t *mc = mychan_find(channel);
		myuser_t *tmu;
		char *flagstr = strtok(NULL, " ");

		if (!u->myuser)
		{
			notice(chansvs.nick, origin, "You are not logged in.");
			return;
		}

		if (!(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
		{
			notice(chansvs.nick, origin, "You are not authorized to execute this command.");
			return;
		}

		if (!target || !flagstr)
		{
			notice(chansvs.nick, origin, "Usage: FLAGS %s [target] [flags]", channel);
			return;
		}

		if (!validhostmask(target))
		{
			if (!(tmu = myuser_find(target)))
			{
				notice(chansvs.nick, origin, "This account does not exist.");
				return;
			}

			if (!(ca = chanacs_find(mc, tmu, 0x0)))
				chanacs_add(mc, tmu, flags_to_bitmask(flagstr, chanacs_flags, 0x0));
			else
			{
				ca->level = flags_to_bitmask(flagstr, chanacs_flags, ca->level);

				if (ca->level == 0x0)
					chanacs_delete(mc, tmu, ca->level);
			}
		}
		else
		{
			if (!(ca = chanacs_find_host_literal(mc, target, 0x0)))
				chanacs_add_host(mc, target, flags_to_bitmask(flagstr, chanacs_flags, 0x0));
			else
			{
				ca->level = flags_to_bitmask(flagstr, chanacs_flags, ca->level);

				if (ca->level == 0x0)
					chanacs_delete_host(mc, target, ca->level);
			}
		}

		notice(chansvs.nick, origin, "Flags \2%s\2 were set on \2%s\2 in \2%s\2.", flagstr, target, channel);
		verbose(mc, "Flags \2%s\2 were set on \2%s\2 in \2%s\2.", flagstr, target, channel);
	}
}

/* FLAGS <channel> [user] [flags] */
static void cs_fcmd_flags(char *origin, char *channel)
{
	user_t *u = user_find(origin);
	chanacs_t *ca;
	mychan_t *mc = mychan_find(channel);
	myuser_t *tmu;
	char *target = strtok(NULL, " ");
	char *flagstr = strtok(NULL, " ");

	if (!target || !flagstr)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2FLAGS\2.");
		notice(chansvs.nick, origin, "Syntax: .flags <target> <flags>");
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!(ca = chanacs_find(mc, u->myuser, CA_FLAGS)))
	{
		notice(chansvs.nick, origin, "You are not authorized to execute this command.");
		return;
	}

	if (!validhostmask(target))
	{
		if (!(tmu = myuser_find(target)))
		{
			notice(chansvs.nick, origin, "This account does not exist.");
			return;
		}

		if (!(ca = chanacs_find(mc, tmu, 0x0)))
			chanacs_add(mc, tmu, flags_to_bitmask(flagstr, chanacs_flags, 0x0));
		else
		{
			ca->level = flags_to_bitmask(flagstr, chanacs_flags, ca->level);

			if (ca->level == 0x0)
				chanacs_delete(mc, tmu, ca->level);
		}
	}
	else
	{
		if (!(ca = chanacs_find_host_literal(mc, target, 0x0)))
			chanacs_add_host(mc, target, flags_to_bitmask(flagstr, chanacs_flags, 0x0));
		else
		{
			ca->level = flags_to_bitmask(flagstr, chanacs_flags, ca->level);

			if (ca->level == 0x0)
				chanacs_delete_host(mc, target, ca->level);
		}
	}

	notice(chansvs.nick, origin, "Flags \2%s\2 were set on \2%s\2 in \2%s\2.", flagstr, target, channel);
	verbose(mc, "Flags \2%s\2 were set on \2%s\2 in \2%s\2.", flagstr, target, channel);
}

