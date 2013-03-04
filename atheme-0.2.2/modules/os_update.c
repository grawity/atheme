/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService UPDATE command.
 *
 * $Id: os_update.c 506 2005-06-12 06:47:47Z nenolod $
 */

#include "atheme.h"

static void os_cmd_update(char *origin);

command_t os_update = { "UPDATE", "Flushes services database to disk.",
                        AC_SRA, os_cmd_update };

extern list_t os_cmdtree;

void _modinit(module_t *m)
{
        command_add(&os_update, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_update, &os_cmdtree);
}

void os_cmd_update(char *origin)
{
	snoop("UPDATE: \2%s\2", origin);
	wallops("Updating database by request of \2%s\2.", origin);
	expire_check(NULL);
	db_save(NULL);
}
