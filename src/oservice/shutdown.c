/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: shutdown.c 111 2005-05-28 23:19:53Z nenolod $
 */

#include "atheme.h"

void os_shutdown(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("SHUTDOWN: \2%s\2", origin);
	wallops("Shutting down by request of \2%s\2.", origin);

	runflags |= RF_SHUTDOWN;
}
