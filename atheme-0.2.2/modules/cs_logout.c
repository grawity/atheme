/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGOUT functions.
 *
 * $Id: cs_logout.c 826 2005-07-16 06:22:04Z w00t $
 */

#include "atheme.h"

static void cs_cmd_logout(char *origin);

command_t cs_logout = { "LOGOUT", "Logs you out of services.",
                        AC_NONE, cs_cmd_logout };
                                                                                   
extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_logout, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_logout, &cs_cmdtree);
}

static void cs_cmd_logout(char *origin)
{
	user_t *u = user_find(origin);
	char *user = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");

	if ((!u->myuser) && (!user || !pass))
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (user && pass)
	{
		myuser_t *mu = myuser_find(user);

		if (!mu)
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", user);
			return;
		}

		if ((!strcmp(mu->pass, pass)) && (mu->user))
		{
			u = mu->user;
			notice(u->nick, "You were logged out by \2%s\2.", origin);
		}
		else
		{
			notice(chansvs.nick, origin, "Authentication failed. Invalid password for \2%s\2.", mu->name);
			return;
		}
	}

	if (is_sra(u->myuser))
		snoop("DESRA: \2%s\2 as \2%s\2", u->nick, u->myuser->name);

	snoop("LOGOUT: \2%s\2 from \2%s\2", u->nick, u->myuser->name);

	if (irccasecmp(origin, u->nick))
		notice(chansvs.nick, origin, "\2%s\2 has been logged out.", u->nick);
	else
		notice(chansvs.nick, origin, "You have been logged out.");

	ircd_on_logout(origin, u->myuser->name, NULL);

	u->myuser->user = NULL;
	u->myuser->identified = FALSE;
	u->myuser = NULL;
}
