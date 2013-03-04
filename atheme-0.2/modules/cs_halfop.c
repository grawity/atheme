/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: cs_halfop.c 1056 2005-07-19 21:48:51Z alambert $
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
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];

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

	hostbuf[0] = '\0';

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if (!chanacs_find(mc, u->myuser, CA_HALFOP) && !chanacs_find_host(mc, hostbuf, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to op */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
			return;
		}
	}

	if (u->server == me.me)
		return;

	if (!chanacs_find_host(mc, hostbuf, CA_HALFOP))
	{
		if ((MC_SECURE & mc->flags) && (!u->myuser))
		{
			notice(chansvs.nick, origin, "The \2SECURE\2 flag is set for \2%s\2.", mc->name);
			return;
		}
		else if ((MC_SECURE & mc->flags) && (!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_HALFOP)))
		{
			notice(chansvs.nick, origin, "\2%s\2 could not be halfopped on \2%s\2.", u->nick, mc->name);
			return;
		}
	}

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	if (ircd->halfops_mode & cu->modes)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already halfopped on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "+h", CLIENT_NAME(u));
	cu->modes |= ircd->halfops_mode;
	notice(chansvs.nick, origin, "\2%s\2 has been halfopped on \2%s\2.", u->nick, mc->name);
}

static void cs_cmd_dehalfop(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

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
	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_HALFOP)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to deop */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
			return;
		}
	}

	if (u->server == me.me)
		return;

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	if (!(ircd->halfops_mode & cu->modes))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not halfopped on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "-h", CLIENT_NAME(u));
	cu->modes &= ircd->halfops_mode;
	notice(chansvs.nick, origin, "\2%s\2 has been dehalfopped on \2%s\2.", u->nick, mc->name);
}

static void cs_fcmd_halfop(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!HALFOP\2.");
		notice(chansvs.nick, origin, "Syntax: !HALFOP [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	hostbuf[0] = '\0';

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if (!chanacs_find(mc, u->myuser, CA_HALFOP) && !chanacs_find_host(mc, hostbuf, CA_HALFOP))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to op */
	while ((nick = strtok(NULL, " ")))
	{
		if (nick)
		{
			if (!(u = user_find(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
				return;
			}
		}

		if (!chanacs_find_host(mc, hostbuf, CA_HALFOP))
		{
			if ((MC_SECURE & mc->flags) && (!u->myuser))
			{
				notice(chansvs.nick, origin, "The \2SECURE\2 flag is set for \2%s\2.", mc->name);
				return;
			}
			else if ((MC_SECURE & mc->flags) && (!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_HALFOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 could not be halfopped on \2%s\2.", u->nick, mc->name);
				return;
			}
		}

		if (u->server == me.me)
			continue;

		cu = chanuser_find(mc->chan, u);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
			return;
		}

		if (ircd->halfops_mode & cu->modes)
		{
			notice(chansvs.nick, origin, "\2%s\2 is already halfopped on \2%s\2.", u->nick, mc->name);
			return;
		}

		cmode(chansvs.nick, chan, "+h", CLIENT_NAME(u));
		cu->modes |= ircd->halfops_mode;
	}
}

static void cs_fcmd_dehalfop(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!DEHALFOP\2.");
		notice(chansvs.nick, origin, "Syntax: !DEHALFOP [nickname]");
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

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_HALFOP)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to deop */
	while ((nick = strtok(NULL, " ")))
	{
		if (nick)
		{
			if (!(u = user_find(nick)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
				return;
			}
		}

		if (u->server == me.me)
			continue;

		cu = chanuser_find(mc->chan, u);
		if (!cu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
			return;
		}

		if (!(ircd->halfops_mode & cu->modes))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not halfopped on \2%s\2.", u->nick, mc->name);
			return;
		}

		cmode(chansvs.nick, chan, "-h", CLIENT_NAME(u));
		cu->modes &= ircd->halfops_mode;
		notice(chansvs.nick, origin, "\2%s\2 has been dehalfopped on \2%s\2.", u->nick, mc->name);
	}
}
