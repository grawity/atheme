/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: restart.c 111 2005-05-28 23:19:53Z nenolod $
 */

#include "atheme.h"

void os_restart(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("RESTART: \2%s\2", origin);
	wallops("Restarting in \2%d\2 seconds by request of \2%s\2.", me.restarttime, origin);

	runflags |= RF_RESTART;
}
