/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: cs_halfop.c 1432 2005-08-03 19:36:14Z alambert $
 */

#include "atheme.h"

static void cs_cmd_halfop(char *origin);
static void cs_cmd_dehalfop(char *origin);
static void cs_fcmd_halfop(char *origin, char *chan);
static void cs_fcmd_dehalfop(char *origin, char *chan);

command_t cs_halfop = { "HALFOP", "Gives channel halfops to a user.",
                        AC_NONE, cs_cmd_halfop };
command_t cs_dehalfop = { "DEHALFOP", "Removes channel halfops from a user.",
                        AC_NONE, cs_cmd_dehalfop };

fcommand_t fc_halfop = { "!halfop", AC_NONE, cs_fcmd_halfop };
fcommand_t fc_hop = { "!hop", AC_NONE, cs_fcmd_halfop };
fcommand_t fc_dehalfop = { "!dehalfop", AC_NONE, cs_fcmd_dehalfop };
fcommand_t fc_dehop = { "!dehop", AC_NONE, cs_fcmd_dehalfop };

extern list_t cs_cmdtree;
extern list_t cs_fcmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_halfop, &cs_cmdtree);
        command_add(&cs_dehalfop, &cs_cmdtree);
	fcommand_add(&fc_halfop, &cs_fcmdtree);
	fcommand_add(&fc_hop, &cs_fcmdtree);
	fcommand_add(&fc_dehalfop, &cs_fcmdtree);
	fcommand_add(&fc_dehop, &cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_halfop, &cs_cmdtree);
	command_delete(&cs_dehalfop, &cs_cmdtree);
	fcommand_delete(&fc_halfop, &cs_fcmdtree);
	fcommand_delete(&fc_hop, &cs_fcmdtree);
	fcommand_delete(&fc_dehalfop, &cs_fcmdtree);
	fcommand_delete(&fc_dehop, &cs_fcmdtree);
}

static void cs_cmd_halfop(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2HALFOP\2.");
		notice(chansvs.nick, origin, "Syntax: HALFOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to halfop */
	if (!nick)
		tu = u;
	else
	{
		if (!(tu = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (tu->server == me.me)
		return;

	/* SECURE check; we can skip this if sender == target, because we already verified */
	if ((u != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
		notice(chansvs.nick, origin, "\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access.", mc->name, tu->nick);
		return;
	}

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	if (ircd->halfops_mode & cu->modes)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already halfopped on \2%s\2.", tu->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "+h", CLIENT_NAME(tu));
	cu->modes |= ircd->halfops_mode;
	notice(chansvs.nick, origin, "\2%s\2 has been halfopped on \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_dehalfop(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2DEHALFOP\2.");
		notice(chansvs.nick, origin, "Syntax: DEHALFOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to dehalfop */
	if (!nick)
		tu = u;
	else
	{
		if (!(tu = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (tu->server == me.me)
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	if (!(ircd->halfops_mode & cu->modes))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not halfopped on \2%s\2.", tu->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "-h", CLIENT_NAME(tu));
	cu->modes &= ~ircd->halfops_mode;
	notice(chansvs.nick, origin, "\2%s\2 has been dehalfopped on \2%s\2.", tu->nick, mc->name);
}

static void cs_fcmd_halfop(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to halfop, do so, repeat. */
	while ((nick = strtok(NULL, " ")))
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (tu->server == me.me)
			continue;

		/* SECURE check; we can skip this if sender == target, because we already verified */
		if ((u != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_HALFOP))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
			notice(chansvs.nick, origin, "\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access.", mc->name, tu->nick);
			continue;
		}

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		if (ircd->halfops_mode & cu->modes)
		{
			notice(chansvs.nick, origin, "\2%s\2 is already halfopped on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		cmode(chansvs.nick, chan, "+h", CLIENT_NAME(tu));
		cu->modes |= ircd->halfops_mode;
	}
}

static void cs_fcmd_dehalfop(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u, *tu;
	chanuser_t *cu;

	if (!ircd->uses_halfops)
	{
		notice(chansvs.nick, origin, "Your IRC server does not support halfops.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	if (!chanacs_user_has_flag(mc, u, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to dehalfop */
	while ((nick = strtok(NULL, " ")))
	{
		if (!nick)
			tu = u;
		else
		{
			if (!(tu = user_find(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not online.", nick);
				continue;
			}
		}

		if (tu->server == me.me)
			continue;

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		if (!(ircd->halfops_mode & cu->modes))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not halfopped on \2%s\2.", tu->nick, mc->name);
			continue;
		}

		cmode(chansvs.nick, chan, "-h", CLIENT_NAME(tu));
		cu->modes &= ~ircd->halfops_mode;
	}
}
