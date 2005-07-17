/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService RAW command.
 *
 * $Id: os_restart.c 506 2005-06-12 06:47:47Z nenolod $
 */

#include "atheme.h"

static void os_cmd_restart(char *origin);

command_t os_restart = { "RESTART", "Restart services.",
                          AC_SRA, os_cmd_restart };

extern list_t os_cmdtree;

void _modinit(module_t *m)
{
        command_add(&os_restart, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_restart, &os_cmdtree);
}

static void os_cmd_restart(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);

	snoop("RESTART: \2%s\2", origin);
	wallops("Restarting in \2%d\2 seconds by request of \2%s\2.", me.restarttime, origin);

	runflags |= RF_RESTART;
}
