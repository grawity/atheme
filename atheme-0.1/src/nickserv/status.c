/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService STATUS function.
 *
 * $Id: status.c 357 2002-03-13 17:42:21Z nenolod $
 */

#include "atheme.h"

void ns_status(char *origin)
{
	user_t *u = user_find(origin);

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
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
