/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 74 2005-05-23 02:33:37Z nenolod $
 */

#include "atheme.h"

void cs_status(char *origin)
{
	user_t *u = user_find(origin);
	char *chan = strtok(NULL, " ");

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (chan)
	{
		mychan_t *mc = mychan_find(chan);
		chanacs_t *ca;

		if (*chan != '#')
		{
			notice(chansvs.nick, origin, "Invalid parameters specified for \2STATUS\2.");
			return;
		}

		if (!mc)
		{
			notice(chansvs.nick, origin, "No such channel: \2%s\2.", chan);
			return;
		}

		if (is_founder(mc, u->myuser))
		{
			notice(chansvs.nick, origin, "You are founder on \2%s\2.", mc->name);
			return;
		}

		if (is_xop(mc, u->myuser, CA_VOP))
		{
			notice(chansvs.nick, origin, "You are VOP on \2%s\2.", mc->name);
			return;
		}

		if (is_xop(mc, u->myuser, CA_AOP))
		{
			notice(chansvs.nick, origin, "You are AOP on \2%s\2.", mc->name);
			return;
		}

		if (is_xop(mc, u->myuser, CA_SOP))
		{
			notice(chansvs.nick, origin, "You are SOP on \2%s\2.", mc->name);
			return;
		}

		if ((ca = chanacs_find(mc, u->myuser, 0x0)))
		{
			notice(chansvs.nick, origin, "You have access flags \2%s\2 on \2%s\2.", bitmask_to_flags(ca->level, chanacs_flags), mc->name);
			return;
		}

		notice(chansvs.nick, origin, "You are a normal user on \2%s\2.", mc->name);

		return;
	}

	notice(chansvs.nick, origin, "You are logged in as \2%s\2.", u->myuser->name);

	if (is_sra(u->myuser))
		notice(chansvs.nick, origin, "You are a services root administrator.");

	if (is_admin(u))
		notice(chansvs.nick, origin, "You are a server administrator.");

	if (is_ircop(u))
		notice(chansvs.nick, origin, "You are an IRC operator.");
}
