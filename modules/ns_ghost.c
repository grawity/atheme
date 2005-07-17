/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService LOGIN functions.
 *
 * $Id: ns_ghost.c 896 2005-07-16 23:31:13Z nenolod $
 */

#include "atheme.h"

static void ns_cmd_ghost(char *origin);

command_t ns_ghost = { "GHOST", "Reclaims use of a nickname.", AC_NONE, ns_cmd_ghost };

extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
	command_add(&ns_ghost, &ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_ghost, &ns_cmdtree);
}

void ns_cmd_ghost(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	char *target = strtok(NULL, " ");
	char *password = strtok(NULL, " ");

	if (!password)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2GHOST\2.");
		notice(nicksvs.nick, origin, "Syntax: GHOST <target> <password>");
		return;
	}

	mu = myuser_find(target);
	if (!mu)
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a registered nickname.", target);
		return;
	}

	if (!strcmp(password, mu->pass))
	{
		snoop("GHOST: \2%s\2 by \2%s\2", mu->name, u->nick);

		mu->user = NULL;

		skill(nicksvs.nick, target, "GHOST command used by %s!%s@%s", u->nick, u->user, u->vhost);

		notice(nicksvs.nick, origin, "\2%s\2 has been ghosted.", target);

		mu->lastlogin = CURRTIME;

		return;
	}

	snoop("GHOST:AF: \2%s\2 to \2%s\2", u->nick, mu->name);

	notice(nicksvs.nick, origin, "Invalid password for \2%s\2.", mu->name);
}
