/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService VOICE functions.
 *
 * $Id: cs_voice.c 964 2005-07-18 07:37:30Z nenolod $
 */

#include "atheme.h"

static void cs_cmd_voice(char *origin);
static void cs_cmd_devoice(char *origin);
static void cs_fcmd_voice(char *origin, char *chan);
static void cs_fcmd_devoice(char *origin, char *chan);

command_t cs_voice = { "VOICE", "Gives channel voice to a user.",
                         AC_NONE, cs_cmd_voice };
command_t cs_devoice = { "DEVOICE", "Removes channel voice from a user.",
                         AC_NONE, cs_cmd_devoice };

fcommand_t fc_voice = { "!voice", AC_NONE, cs_fcmd_voice };
fcommand_t fc_devoice = { "!devoice", AC_NONE, cs_fcmd_devoice };

extern list_t cs_cmdtree;
extern list_t cs_fcmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_voice, &cs_cmdtree);
        command_add(&cs_devoice, &cs_cmdtree);
	fcommand_add(&fc_voice, &cs_fcmdtree);
	fcommand_add(&fc_devoice, &cs_fcmdtree);
}

void _moddeinit()
{
	command_delete(&cs_voice, &cs_cmdtree);
	command_delete(&cs_devoice, &cs_cmdtree);
	fcommand_delete(&fc_voice, &cs_fcmdtree);
	fcommand_delete(&fc_devoice, &cs_fcmdtree);
}

static void cs_cmd_voice(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2VOICE\2.");
		notice(chansvs.nick, origin, "Syntax: VOICE <#channel> [nickname]");
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

	if (!chanacs_find(mc, u->myuser, CA_VOICE) && !chanacs_find_host(mc, hostbuf, CA_VOICE))
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
	
	if (u->server == me.me)
		return;
	cu = chanuser_find(mc->chan, u);
	if (!cu)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not on \2%s\2.", u->nick, mc->name);
		return;
	}

	if (CMODE_VOICE & cu->modes)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already voiced on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "+v", CLIENT_NAME(u));
	cu->modes |= CMODE_VOICE;
	notice(chansvs.nick, origin, "\2%s\2 has been voiced on \2%s\2.", u->nick, mc->name);
}

static void cs_cmd_devoice(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2DEVOICE\2.");
		notice(chansvs.nick, origin, "Syntax: DEVOICE <#channel> [nickname]");
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

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_VOICE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to devoice */
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

	if (!(CMODE_VOICE & cu->modes))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not voiced on \2%s\2.", u->nick, mc->name);
		return;
	}

	cmode(chansvs.nick, chan, "-v", CLIENT_NAME(u));
	cu->modes &= ~CMODE_VOICE;
	notice(chansvs.nick, origin, "\2%s\2 has been devoiced on \2%s\2.", u->nick, mc->name);
}

static void cs_fcmd_voice(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;
	char hostbuf[BUFSIZE];

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!VOICE\2.");
		notice(chansvs.nick, origin, "Syntax: !VOICE [nickname]");
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

	if (!chanacs_find(mc, u->myuser, CA_VOICE) && !chanacs_find_host(mc, hostbuf, CA_VOICE))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	while ((nick = strtok(NULL, " ")))
	{
		/* figure out who we're going to voice */
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

		if (CMODE_VOICE & cu->modes)
		{
			notice(chansvs.nick, origin, "\2%s\2 is already voiced on \2%s\2.", u->nick, mc->name);
			return;
		}

		cmode(chansvs.nick, chan, "+v", CLIENT_NAME(u));
		cu->modes |= CMODE_VOICE;
	}
}

static void cs_fcmd_devoice(char *origin, char *chan)
{
	char *nick;
	mychan_t *mc;
	user_t *u;
	chanuser_t *cu;

	if (!chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2!DEVOICE\2.");
		notice(chansvs.nick, origin, "Syntax: !DEVOICE [nickname]");
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

	if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_VOICE)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	/* figure out who we're going to devoice */
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

		if (!(CMODE_VOICE & cu->modes))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not voiced on \2%s\2.", u->nick, mc->name);
			return;
		}

		cmode(chansvs.nick, chan, "-v", CLIENT_NAME(u));
		cu->modes &= ~CMODE_VOICE;
	}
}

