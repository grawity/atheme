/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: os_raw.c 506 2005-06-12 06:47:47Z nenolod $
 */

#include "atheme.h"

static void os_cmd_raw(char *origin);

command_t os_raw = { "RAW", "Sends data to the uplink.",
                        AC_SRA, os_cmd_raw };

extern list_t os_cmdtree;

void _modinit(module_t *m)
{
        command_add(&os_raw, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_raw, &os_cmdtree);
}

static void os_cmd_raw(char *origin)
{
	char *s = strtok(NULL, "");

	if (!config_options.raw)
		return;

	if (!s)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2RAW\2.");
		notice(chansvs.nick, origin, "Syntax: RAW <parameters>");
		return;
	}

	snoop("RAW: \"%s\" by \2%s\2", s, origin);
	sts("%s", s);
}
