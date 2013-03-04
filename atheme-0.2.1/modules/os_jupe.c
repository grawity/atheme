/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Jupiters a server.
 *
 * $Id: os_jupe.c 676 2005-07-07 02:25:48Z nenolod $
 */

#include "atheme.h"

static void os_cmd_jupe(char *origin);

command_t os_jupe = { "JUPE", "Jupiters a server.", AC_IRCOP, os_cmd_jupe };

extern list_t os_cmdtree;

void _modinit(module_t *m)
{
	command_add(&os_jupe, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_jupe, &os_cmdtree);
}

static void os_cmd_jupe(char *origin)
{
	char *server = strtok(NULL, " ");
	char *reason = strtok(NULL, "");

	if (!server || !reason)
	{
		notice(opersvs.nick, origin, "Insufficient parameters for JUPE.");
		notice(opersvs.nick, origin, "Usage: JUPE <server> <reason>");
		return;
	}

	server_delete(server);
	jupe(server, reason);

	notice(opersvs.nick, origin, "\2%s\2 has been jupitered.", server);
}
