/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService MODE command.
 *
 * $Id: mode.c 236 2005-05-30 04:22:34Z nenolod $
 */

#include "atheme.h"

void os_mode(char *origin)
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

