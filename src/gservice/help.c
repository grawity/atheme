/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 111 2005-05-28 23:19:53Z nenolod $
 */

#include "atheme.h"

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ gs_help_commands[] = {
  { "HELP",     AC_IRCOP, "help/help"              },
  { "GLOBAL",   AC_IRCOP, "help/gservice/global"   },
  { NULL }
};

/* *INDENT-ON* */

/* HELP <command> [params] */
void gs_help(char *origin)
{
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(globsvs.nick, origin, "The following core commands are available.");
		notice(globsvs.nick, origin, "\2HELP\2          Displays help information.");
		notice(globsvs.nick, origin, "\2GLOBAL\2        Send a global notice.");
		notice(globsvs.nick, origin, "For more specific help use \2HELP \37command\37\2.");

		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(globsvs.nick, origin, command, gs_help_commands)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(globsvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", globsvs.nick);

				if (buf[0])
					notice(globsvs.nick, origin, "%s", buf);
				else
					notice(globsvs.nick, origin, " ");
			}

			fclose(help_file);
		}
		else
			notice(globsvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
