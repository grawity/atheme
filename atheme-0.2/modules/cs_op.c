/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: cs_op.c 964 2005-07-18 07:37:30Z nenolod $
 */

#include "atheme.h"

static void cs_cmd_op(char *origin);
static void cs_cmd_deop(char *origin);
static void cs_fcmd_op(char *origin, char *chan);
static void cs_fcmd_deop(char *origin, char *chan);

command_t cs_op = { "OP", "Gives channel ops to a user.",
                        AC_NONE, cs_cmd_op };
command_t cs_deop = { "DEOP", "Removes channel ops from a user.",
                        AC_NONE, cs_cmd_deop };

fcommand_t fc_op = { "!op", AC_NONE, cs_fcmd_op };
fcommand_t fc_deop = { "!deop", AC_NONE, cs_fcmd_deop };
                                                                                   
extern list_t cs_cmdtree;
extern list_t cs_fcmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_op, &cs_cmdtree);
        command_add(&cs_deop, &cs_cmdtree);
	fcommand_add(&fc_op, &cs_fcmdtree);
	fcommand_add(&fc_deop, &cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_op, &cs_cmdtree);
	command_delete(&cs_deop, &cs_cmdtree);
	fcommand_delete(&fc_op, &cs_fcmdtree);
	fcommand_delete(&fc_deop, &cs_fcmdtree);
}

static void cs_cmd_op(char *origin)
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

	if (!chanacs_find(mc, u->myuser, CA_OP) && !chanacs_find_host(mc, hostbuf, CA_OP))
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

	if (!chanacs_find_host(mc, hostbuf, CA_OP))
	{
		if (!u->myuser)
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
			return;
		}
		else if (!is_xop(mc, u->myuser, CA_OP))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
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

	cmode(chansvs.nick, chan, "+o", CLIENT_NAME(u));
	cu->modes |= CMODE_OP;
	notice(chansvs.nick, origin, "\2%s\2 has been opped on \2%s\2.", u->nick, mc->name);
}

static void cs_cmd_deop(char *origin)
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
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
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

	if (!(CMODE_OP & cu->modes))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not opped on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "-o", CLIENT_NAME(u));
	cu->modes &= ~CMODE_OP;
	notice(chansvs.nick, origin, "\2%s\2 has been deopped on \2%s\2.", u->nick, mc->name);
}

static void cs_fcmd_op(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!OP\2.");
		notice(chansvs.nick, origin, "Syntax: !OP [nickname]");
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

	if (!chanacs_find(mc, u->myuser, CA_OP) && !chanacs_find_host(mc, hostbuf, CA_OP))
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

		if (u->server == me.me)
			continue;

		if (!chanacs_find_host(mc, hostbuf, CA_OP))
		{
			if (!u->myuser)
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
				return;
			}
			else if (!is_xop(mc, u->myuser, CA_OP))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.", mc->name);
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

		cmode(chansvs.nick, chan, "+o", CLIENT_NAME(u));
		cu->modes |= CMODE_OP;
	}
}

static void cs_fcmd_deop(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!DEOP\2.");
		notice(chansvs.nick, origin, "Syntax: !DEOP [nickname]");
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

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_OP)))
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

		if (!(CMODE_OP & cu->modes))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not opped on \2%s\2.", u->nick, mc->name);
			return;
		}

		cmode(chansvs.nick, chan, "-o", CLIENT_NAME(u));
		cu->modes &= ~CMODE_OP;
	}
}

