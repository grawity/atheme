/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: raw.c 111 2005-05-28 23:19:53Z nenolod $
 */

#include "atheme.h"

void os_raw(char *origin)
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
