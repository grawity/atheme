/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 2561 2005-10-04 06:59:29Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/help", FALSE, _modinit, _moddeinit,
	"$Id: help.c 2561 2005-10-04 06:59:29Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_help(char *origin);

command_t os_help = { "HELP", "Displays contextual help information.",
			AC_IRCOP, os_cmd_help };

list_t *os_cmdtree;
list_t *os_helptree;

void _modinit(module_t *m)
{
	os_cmdtree = module_locate_symbol("operserv/main", "os_cmdtree");
	os_helptree = module_locate_symbol("operserv/main", "os_helptree");

	command_add(&os_help, os_cmdtree);
	help_addentry(os_helptree, "HELP", "help/help", NULL);
}

void _moddeinit()
{
	command_delete(&os_help, os_cmdtree);
	help_delentry(os_helptree, "HELP");
}

/* HELP <command> [params] */
static void os_cmd_help(char *origin)
{
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(opersvs.nick, origin, "***** \2%s Help\2 *****", opersvs.nick);
		notice(opersvs.nick, origin, "\2%s\2 provides essential network management services, such as", opersvs.nick);
		notice(opersvs.nick, origin, "routing manipulation and access restriction. Please do not abuse");
		notice(opersvs.nick, origin, "your access to \2%s\2!", opersvs.nick);
		notice(opersvs.nick, origin, " ");
		notice(opersvs.nick, origin, "For information on a command, type:");
		notice(opersvs.nick, origin, "\2/%s%s help <command>\2", (ircd->uses_rcommand == FALSE) ? "msg " : "", opersvs.disp);
		notice(opersvs.nick, origin, " ");

		command_help(opersvs.nick, origin, os_cmdtree);

		notice(opersvs.nick, origin, "***** \2End of Help\2 *****", opersvs.nick);
		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(opersvs.nick, origin, command, os_helptree)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(opersvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			notice(opersvs.nick, origin, "***** \2%s Help\2 *****", opersvs.nick);

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", opersvs.disp);

				if (buf[0])
					notice(opersvs.nick, origin, "%s", buf);
				else
					notice(opersvs.nick, origin, " ");
			}

			notice(opersvs.nick, origin, "***** \2End of Help\2 *****", opersvs.nick);

			fclose(help_file);
		}
		else
			notice(opersvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
