/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Jupiters a server.
 *
 * $Id: jupe.c 6927 2006-10-24 15:22:05Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/jupe", FALSE, _modinit, _moddeinit,
	"$Id: jupe.c 6927 2006-10-24 15:22:05Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_jupe(sourceinfo_t *si, int parc, char *parv[]);

command_t os_jupe = { "JUPE", "Jupiters a server.", PRIV_JUPE, 2, os_cmd_jupe };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_jupe, os_cmdtree);
	help_addentry(os_helptree, "JUPE", "help/oservice/jupe", NULL);
}

void _moddeinit()
{
	command_delete(&os_jupe, os_cmdtree);
	help_delentry(os_helptree, "JUPE");
}

static void os_cmd_jupe(sourceinfo_t *si, int parc, char *parv[])
{
	char *server = parv[0];
	char *reason = parv[1];

	if (!server || !reason)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "JUPE");
		command_fail(si, fault_needmoreparams, "Usage: JUPE <server> <reason>");
		return;
	}

	if (!strchr(server, '.'))
	{
		command_fail(si, fault_badparams, "\2%s\2 is not a valid server name.", server);
		return;
	}

	if (!irccasecmp(server, me.name))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is the services server; it cannot be jupitered!", server);
		return;
	}

	if (!irccasecmp(server, me.actual))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is the current uplink; it cannot be jupitered!", server);
		return;
	}

	logcommand(si, CMDLOG_SET, "JUPE %s %s", server, reason);

	server_delete(server);
	jupe(server, reason);

	command_success_nodata(si, "\2%s\2 has been jupitered.", server);
}
