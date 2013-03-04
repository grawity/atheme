/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService UPDATE command.
 *
 * $Id: update.c 6927 2006-10-24 15:22:05Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/update", FALSE, _modinit, _moddeinit,
	"$Id: update.c 6927 2006-10-24 15:22:05Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_update(sourceinfo_t *si, int parc, char *parv[]);

command_t os_update = { "UPDATE", "Flushes services database to disk.", PRIV_ADMIN, 0, os_cmd_update };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_update, os_cmdtree);
	help_addentry(os_helptree, "UPDATE", "help/oservice/update", NULL);
}

void _moddeinit()
{
	command_delete(&os_update, os_cmdtree);
	help_delentry(os_helptree, "UPDATE");
}

void os_cmd_update(sourceinfo_t *si, int parc, char *parv[])
{
	snoop("UPDATE: \2%s\2", get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "UPDATE");
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	db_save(NULL);
	/* db_save() will wallops/snoop/log the error */
	command_success_nodata(si, "UPDATE completed.");
}
