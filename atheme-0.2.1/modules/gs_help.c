/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: gs_help.c 708 2005-07-10 02:37:26Z nenolod $
 */

#include "atheme.h"

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ gs_help_commands[] = {
  { "HELP",     AC_IRCOP, "help/help"              },
  { "GLOBAL",   AC_IRCOP, "help/gservice/global"   },
  { NULL, 0, NULL }
};

static void gs_cmd_help(char *origin);

command_t gs_help = { "HELP", "Displays contextual help information.",
                        AC_IRCOP, gs_cmd_help };

extern list_t gs_cmdtree;

void _modinit(module_t *m)
{
        command_add(&gs_help, &gs_cmdtree);
}

void _moddeinit()
{
	command_delete(&gs_help, &gs_cmdtree);
}

/* *INDENT-ON* */

/* HELP <command> [params] */
static void gs_cmd_help(char *origin)
{
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		command_help(globsvs.nick, origin, &gs_cmdtree);
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

				replace(buf, sizeof(buf), "&nick&", globsvs.disp);

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
