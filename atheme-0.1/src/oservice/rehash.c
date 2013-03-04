/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService REHASH command.
 *
 * $Id: rehash.c 111 2005-05-28 23:19:53Z nenolod $
 */

#include "atheme.h"

/* REHASH */
void os_rehash(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("REHASH: \2%s\2", origin);
	wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, origin);

	if (!conf_rehash())
	{
		notice(opersvs.nick, origin, "REHASH of \2%s\2 failed. Please corrrect any errors in the " "file and try again.", config_file);
	}
}
