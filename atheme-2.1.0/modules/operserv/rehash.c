/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService REHASH command.
 *
 * $Id: rehash.c 6927 2006-10-24 15:22:05Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rehash", FALSE, _modinit, _moddeinit,
	"$Id: rehash.c 6927 2006-10-24 15:22:05Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_rehash(sourceinfo_t *si, int parc, char *parv[]);

command_t os_rehash = { "REHASH", "Reload the configuration data.", PRIV_ADMIN, 0, os_cmd_rehash };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_rehash, os_cmdtree);
	help_addentry(os_helptree, "REHASH", "help/oservice/rehash", NULL);
}

void _moddeinit()
{
	command_delete(&os_rehash, os_cmdtree);
	help_delentry(os_helptree, "REHASH");
}

/* REHASH */
void os_cmd_rehash(sourceinfo_t *si, int parc, char *parv[])
{
	snoop("UPDATE: \2%s\2", get_oper_name(si));
	wallops("Updating database by request of \2%s\2.", get_oper_name(si));
	expire_check(NULL);
	db_save(NULL);

	snoop("REHASH: \2%s\2", get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "REHASH");
	wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, get_oper_name(si));

	if (conf_rehash())
		command_success_nodata(si, "REHASH completed.");
	else
		command_fail(si, fault_nosuch_target, "REHASH of \2%s\2 failed. Please corrrect any errors in the file and try again.", config_file);
}
