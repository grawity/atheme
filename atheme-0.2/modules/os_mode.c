/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService MODE command.
 *
 * $Id: os_mode.c 547 2005-06-22 04:44:01Z nenolod $
 */

#include "atheme.h"

static void os_cmd_mode(char *origin);

command_t os_mode = { "MODE", "Changes modes on channels.",
                        AC_IRCOP, os_cmd_mode };

extern list_t os_cmdtree;

void _modinit(module_t *m)
{
        command_add(&os_mode, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_mode, &os_cmdtree);
}

static void os_cmd_mode(char *origin)
{
        char *channel = strtok(NULL, " ");
	char *mode = strtok(NULL, " ");
	char *param = strtok(NULL, "");

        if (!channel || !mode)
        {
                notice(opersvs.nick, origin, "Insufficient parameters for \2MODE\2.");
                notice(opersvs.nick, origin, "Syntax: MODE <parameters>");
                return;
        }

	cmode(opersvs.nick, channel, mode, param ? param : "");
}

