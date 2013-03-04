/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 343 2002-03-13 14:59:00Z nenolod $
 */

#include "atheme.h"

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ ns_help_commands[] = {
  { "REGISTER", AC_NONE,  "help/nickserv/register" },
  { "IDENTIFY", AC_NONE,  "help/nickserv/identify" },
  { "HELP",     AC_NONE,  "help/help"              },
  { "DROP",     AC_NONE,  "help/nickserv/drop"     },
  { "GHOST",    AC_NONE,  "help/nickserv/ghost"    },
  { "SENDPASS", AC_IRCOP, "help/nickserv/sendpass" },
  { "SET EMAIL",     AC_NONE, "help/cservice/set_email"     },
  { "SET HIDEMAIL",  AC_NONE, "help/cservice/set_hidemail"  },
  { "SET HOLD",      AC_SRA,  "help/cservice/set_hold"      },
  { "SET NEVEROP",   AC_NONE, "help/cservice/set_neverop"   },
  { "SET NOOP",      AC_NONE, "help/cservice/set_noop"      },
  { "SET PASSWORD",  AC_NONE, "help/cservice/set_password"  },
  { NULL }
};

/* *INDENT-ON* */

/* HELP <command> [params] */
void ns_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(nicksvs.nick, origin, "The following core commands are available.");
		notice(nicksvs.nick, origin, "\2REGISTER\2      Registers a nickname.");
		notice(nicksvs.nick, origin, "\2DROP\2          Drops a registered nickname.");
		notice(nicksvs.nick, origin, "\2IDENTIFY\2      Identifies to Services for a nick.");
		notice(nicksvs.nick, origin, "\2SET\2           Sets various control flags.");
		notice(nicksvs.nick, origin, "\2STATUS\2        Displays your status in services.");
		notice(nicksvs.nick, origin, "\2INFO\2          Displays information on registrations.");
		notice(nicksvs.nick, origin, " ");
		notice(nicksvs.nick, origin, "The following additional commands are available.");
		notice(nicksvs.nick, origin, "\2GHOST\2         Recovers a nickname.");
		notice(nicksvs.nick, origin, " ");

		if ((is_sra(u->myuser)) || (is_ircop(u)))
		{
			notice(nicksvs.nick, origin, "The following IRCop commands are available.");
			notice(nicksvs.nick, origin, "\2SENDPASS\2      Email registration passwords.");
			notice(nicksvs.nick, origin, " ");
		}

		notice(nicksvs.nick, origin, "For more specific help use \2HELP \37command\37\2.");

		return;
	}

	if (!strcasecmp("SET", command))
	{
		notice(nicksvs.nick, origin, "Help for \2SET\2:");
		notice(nicksvs.nick, origin, " ");
		notice(nicksvs.nick, origin, "SET allows you to set various control flags");
		notice(nicksvs.nick, origin, "for nicknames that change the way certain operations");
		notice(nicksvs.nick, origin, "are performed on them.");
		notice(nicksvs.nick, origin, " ");
		notice(nicksvs.nick, origin, "The following commands are available.");
		notice(nicksvs.nick, origin, "\2EMAIL\2         Changes the email address associated " "with a username.");
		notice(nicksvs.nick, origin, "\2HIDEMAIL\2      Hides a username's email address");
		notice(nicksvs.nick, origin, "\2NEVEROP\2       Prevents services from automatically " "setting modes associated with access lists.");
		notice(nicksvs.nick, origin, "\2NOOP\2          Prevents you from being added to " "access lists.");
		notice(nicksvs.nick, origin, "\2PASSWORD\2      Change the password of a username or " "channel.");
		notice(nicksvs.nick, origin, " ");

		if (is_sra(u->myuser))
		{
			notice(nicksvs.nick, origin, "The following SRA commands are available.");
			notice(nicksvs.nick, origin, "\2HOLD\2          Prevents services from expiring a nickname.");
			notice(nicksvs.nick, origin, " ");
		}

		notice(nicksvs.nick, origin, "For more specific help use \2HELP SET \37command\37\2.");

		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(nicksvs.nick, origin, command, ns_help_commands)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(nicksvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", nicksvs.nick);

				if (buf[0])
					notice(nicksvs.nick, origin, "%s", buf);
				else
					notice(nicksvs.nick, origin, " ");
			}

			fclose(help_file);
		}
		else
			notice(nicksvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
