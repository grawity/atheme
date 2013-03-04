/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: os_help.c 708 2005-07-10 02:37:26Z nenolod $
 */

#include "atheme.h"

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ os_help_commands[] = {
  { "HELP",     AC_IRCOP, "help/help"              },
  { "GLOBAL",   AC_IRCOP, "help/gservice/global"   },
  { "KLINE",    AC_IRCOP, "help/oservice/kline"    },
  { "MODE",     AC_IRCOP, "help/oservice/mode"     },

  { "UPDATE",   AC_SRA,   "help/oservice/update"   },
  { "RESTART",  AC_SRA,   "help/oservice/restart"  },
  { "SHUTDOWN", AC_SRA,   "help/oservice/shutdown" },
  { "RAW",      AC_SRA,   "help/oservice/raw"      },
  { "REHASH",   AC_SRA,   "help/oservice/rehash"   },
  { "INJECT",   AC_SRA,   "help/oservice/inject"   },

  { NULL, 0, NULL }
};

/* *INDENT-ON* */

static void os_cmd_help(char *origin);

command_t os_help = { "HELP", "Displays contextual help information.",
			AC_IRCOP, os_cmd_help };

extern list_t os_cmdtree;

void _modinit(module_t *m)
{
	command_add(&os_help, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_help, &os_cmdtree);
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

		command_help(opersvs.nick, origin, &os_cmdtree);

		notice(opersvs.nick, origin, "***** \2End of Help\2 *****", opersvs.nick);
		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(opersvs.nick, origin, command, os_help_commands)))
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
