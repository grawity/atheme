/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 * $Id: ns_logout.c 826 2005-07-16 06:22:04Z w00t $
 */

#include "atheme.h"

static void ns_cmd_logout(char *origin);

command_t ns_logout = { "LOGOUT", "Logs your services session out.",
                        AC_NONE, ns_cmd_logout };
                                                                                   
extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
        command_add(&ns_logout, &ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_logout, &ns_cmdtree);
}

static void ns_cmd_logout(char *origin)
{
	user_t *u = user_find(origin);
	char *user = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if ((!u->myuser) && (!user || !pass))
	{
		notice(nicksvs.nick, origin, "You are not logged in.");
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not registered.", user);
			return;
		}

		if ((!strcmp(mu->pass, pass)) && (mu->user))
		{
			u = mu->user;
			notice(u->nick, "You were logged out by \2%s\2.", origin);
		}
		else
		{
			notice(nicksvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
			return;
		}
	}

	if (is_sra(u->myuser))
		snoop("DESRA: \2%s\2 as \2%s\2", u->nick, u->myuser->name);

	snoop("LOGOUT: \2%s\2 from \2%s\2", u->nick, u->myuser->name);

	if (irccasecmp(origin, u->nick))
		notice(nicksvs.nick, origin, "\2%s\2 has been logged out.", u->nick);
	else
		notice(nicksvs.nick, origin, "You have been logged out.");

	ircd_on_logout(origin, u->myuser->name, NULL);

	u->myuser->lastlogin = CURRTIME;
	u->myuser->user = NULL;
	u->myuser->identified = FALSE;
	u->myuser = NULL;
}
