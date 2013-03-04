/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService KICK functions.
 *
 * $Id: cs_kick.c 1440 2005-08-03 20:19:39Z alambert $
 */

#include "atheme.h"

static void cs_cmd_kick(char *origin);
static void cs_cmd_kickban(char *origin);
static void cs_fcmd_kick(char *origin, char *chan);
static void cs_fcmd_kickban(char *origin, char *chan);

command_t cs_kick = { "KICK", "Removes a user from a channel.",
                        AC_NONE, cs_cmd_kick };
command_t cs_kickban = { "KICKBAN", "Removes and bans a user from a channel.",
			AC_NONE, cs_cmd_kickban };

fcommand_t fc_kick = { "!kick", AC_NONE, cs_fcmd_kick };
fcommand_t fc_kickban = { "!kickban", AC_NONE, cs_fcmd_kickban };
fcommand_t fc_kb = { "!kb", AC_NONE, cs_fcmd_kickban };

extern list_t cs_cmdtree;
extern list_t cs_fcmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_kick, &cs_cmdtree);
	command_add(&cs_kickban, &cs_cmdtree);
	fcommand_add(&fc_kick, &cs_fcmdtree);
	fcommand_add(&fc_kickban, &cs_fcmdtree);
	fcommand_add(&fc_kb, &cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_kick, &cs_cmdtree);
	command_delete(&cs_kickban, &cs_cmdtree);
	fcommand_delete(&fc_kick, &cs_fcmdtree);
	fcommand_delete(&fc_kickban, &cs_fcmdtree);
	fcommand_delete(&fc_kb, &cs_fcmdtree);
}

static void cs_cmd_kick(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2KICK\2.");
		notice(chansvs.nick, origin, "Syntax: KICK <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if (!chanacs_find(mc, u->myuser, CA_REMOVE) && (!chanacs_find_host(mc, hostbuf, CA_REMOVE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
			return;
		}
	}

	/* if target is a service, bail. --nenolod */
	if (u->server == me.me)
		return;

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	kick(chansvs.nick, chan, u->nick, reasonbuf);
	notice(chansvs.nick, origin, "\2%s\2 has been kicked from \2%s\2.", u->nick, mc->name);
}

static void cs_cmd_kickban(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2KICKBAN\2.");
		notice(chansvs.nick, origin, "Syntax: KICKBAN <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if (!chanacs_find(mc, u->myuser, CA_REMOVE) && (!chanacs_find_host(mc, hostbuf, CA_REMOVE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
			return;
		}
	}

        /* if target is a service, bail. --nenolod */
        if (u->server == me.me)
                return;

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	ban(chansvs.nick, chan, u);
	kick(chansvs.nick, chan, u->nick, reasonbuf);
	notice(chansvs.nick, origin, "\2%s\2 has been kickbanned from \2%s\2.", u->nick, mc->name);
}

static void cs_fcmd_kick(char *origin, char *chan)
{
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2KICK\2.");
		notice(chansvs.nick, origin, "Syntax: KICK <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if (!chanacs_find(mc, u->myuser, CA_REMOVE) && (!chanacs_find_host(mc, hostbuf, CA_REMOVE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
			return;
		}
	}

        /* if target is a service, bail. --nenolod */
        if (u->server == me.me)
                return;

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	kick(chansvs.nick, chan, u->nick, reasonbuf);
	notice(chansvs.nick, origin, "\2%s\2 has been kicked from \2%s\2.", u->nick, mc->name);
}

static void cs_fcmd_kickban(char *origin, char *chan)
{
	char *nick = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];
	char reasonbuf[BUFSIZE];

	if (!chan || !nick)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2KICKBAN\2.");
		notice(chansvs.nick, origin, "Syntax: KICKBAN <#channel> <nickname> [reason]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if (!chanacs_find(mc, u->myuser, CA_REMOVE) && !chanacs_find_host(mc, hostbuf, CA_REMOVE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to voice */
	if (nick)
	{
		if (!(u = user_find(nick)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", nick);
			return;
		}
	}

        /* if target is a service, bail. --nenolod */
        if (u->server == me.me)
                return;

	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	snprintf(reasonbuf, BUFSIZE, "%s (%s)", reason ? reason : "No reason given", origin);
	ban(chansvs.nick, chan, u);
	kick(chansvs.nick, chan, u->nick, reasonbuf);
	notice(chansvs.nick, origin, "\2%s\2 has been kickbanned from \2%s\2.", u->nick, mc->name);
}

