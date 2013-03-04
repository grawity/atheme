/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 248 2005-05-30 23:03:23Z nenolod $
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

  { NULL }
};

/* *INDENT-ON* */

/* HELP <command> [params] */
void os_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(opersvs.nick, origin, "The following core commands are available.");
		notice(opersvs.nick, origin, "\2HELP\2          Displays help information.");
		notice(opersvs.nick, origin, "\2GLOBAL\2        Send a global notice.");
		notice(opersvs.nick, origin, "\2MODE\2          Sets modes on channels.");
		notice(opersvs.nick, origin, " ");

		if (is_sra(u->myuser))
		{
			notice(opersvs.nick, origin, "The following SRA commands are available.");
			notice(opersvs.nick, origin, "\2UPDATE\2      Flush the database to disk.");
			notice(opersvs.nick, origin, "\2REHASH\2      Reload the configuration file.");

			if (config_options.raw)
			{
				notice(opersvs.nick, origin, "\2RAW\2         Send data to the uplink.");
				notice(opersvs.nick, origin, "\2INJECT\2      Fake incoming data from the uplink.");
			}

			notice(opersvs.nick, origin, "\2RESTART\2     Restart services.");
			notice(opersvs.nick, origin, "\2SHUTDOWN\2    Shuts down services.");
			notice(opersvs.nick, origin, " ");
		}

		if ((is_sra(u->myuser)) || (is_ircop(u)))
		{
			notice(opersvs.nick, origin, "The following IRCop commands are available.");
			notice(opersvs.nick, origin, "\2KLINE\2         Manage global KLINE's.");
			notice(opersvs.nick, origin, " ");
		}

		notice(opersvs.nick, origin, "For more specific help use \2HELP \37command\37\2.");

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

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", opersvs.nick);

				if (buf[0])
					notice(opersvs.nick, origin, "%s", buf);
				else
					notice(opersvs.nick, origin, " ");
			}

			fclose(help_file);
		}
		else
			notice(opersvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
