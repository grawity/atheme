/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the OService HELP command.
 *
 * $Id: help.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t os_help = { "HELP", N_("Displays contextual help information."), AC_NONE, 1, os_cmd_help };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_help, os_cmdtree);
	help_addentry(os_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&os_help, os_cmdtree);
	help_delentry(os_helptree, "HELP");
}

/* HELP <command> [params] */
static void os_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	char *command = parv[0];

	if (!has_any_privs(si))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to use %s."), opersvs.nick);
		return;
	}

	if (!command)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), opersvs.nick);
		command_success_nodata(si, _("\2%s\2 provides essential network management services, such as"), opersvs.nick);
		command_success_nodata(si, _("routing manipulation and access restriction. Please do not abuse"));
		command_success_nodata(si, _("your access to \2%s\2!"), opersvs.nick);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For information on a command, type:"));
		command_success_nodata(si, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", opersvs.disp);
		command_success_nodata(si, " ");

		command_help(si, os_cmdtree);

		command_success_nodata(si, _("***** \2End of Help\2 *****"), opersvs.nick);
		return;
	}

	/* take the command through the hash table */
	help_display(si, command, os_helptree);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
