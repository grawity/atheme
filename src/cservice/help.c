/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService HELP command.
 *
 * $Id: help.c 247 2005-05-30 22:58:26Z nenolod $
 */

#include "atheme.h"

/* *INDENT-OFF* */

/* help commands we understand */
static struct help_command_ cs_help_commands[] = {
  { "LOGIN",    AC_NONE,  "help/cservice/login"    },
  { "LOGOUT",   AC_NONE,  "help/cservice/logout"   },
  { "HELP",     AC_NONE,  "help/help"              },
  { "SOP",      AC_NONE,  "help/cservice/xop"      },
  { "AOP",      AC_NONE,  "help/cservice/xop"      },
  { "VOP",      AC_NONE,  "help/cservice/xop"      },
  { "OP",       AC_NONE,  "help/cservice/op_voice" },
  { "DEOP",     AC_NONE,  "help/cservice/op_voice" },
  { "VOICE",    AC_NONE,  "help/cservice/op_voice" },
  { "DEVOICE",  AC_NONE,  "help/cservice/op_voice" },
  { "INVITE",   AC_NONE,  "help/cservice/invite"   },
  { "INFO",     AC_NONE,  "help/cservice/info"     },
  { "RECOVER",  AC_NONE,  "help/cservice/recover"  },
  { "REGISTER", AC_NONE,  "help/cservice/register" },
  { "DROP",     AC_NONE,  "help/cservice/drop"     },
  { "AKICK",    AC_NONE,  "help/cservice/akick"    },
  { "STATUS",   AC_NONE,  "help/cservice/status"   },
  { "FLAGS",    AC_NONE,  "help/cservice/flags"    },
  { "KICK",     AC_NONE,  "help/cservice/kick"     },
  { "KICKBAN",  AC_NONE,  "help/cservice/kickban"  },
  { "TOPIC",    AC_NONE,  "help/cservice/topic"    },
  { "TOPICAPPEND", AC_NONE, "help/cservice/topicappend" },
  { "BAN",      AC_NONE,  "help/cservice/ban"      },
  { "UNBAN",    AC_NONE,  "help/cservice/unban"    },
  { "SENDPASS", AC_IRCOP, "help/cservice/sendpass" },
  { "SET EMAIL",     AC_NONE, "help/cservice/set_email"     },
  { "SET FOUNDER",   AC_NONE, "help/cservice/set_founder"   },
  { "SET HIDEMAIL",  AC_NONE, "help/cservice/set_hidemail"  },
  { "SET HOLD",      AC_SRA,  "help/cservice/set_hold"      },
  { "SET MLOCK",     AC_NONE, "help/cservice/set_mlock"     },
  { "SET NEVEROP",   AC_NONE, "help/cservice/set_neverop"   },
  { "SET NOOP",      AC_NONE, "help/cservice/set_noop"      },
  { "SET PASSWORD",  AC_NONE, "help/cservice/set_password"  },
  { "SET SECURE",    AC_NONE, "help/cservice/set_secure"    },
  { "SET SUCCESSOR", AC_NONE, "help/cservice/set_successor" },
  { "SET VERBOSE",   AC_NONE, "help/cservice/set_verbose"   },
  { "SET URL",       AC_NONE, "help/cservice/set_url"       },
  { "SET ENTRYMSG",  AC_NONE, "help/cservice/set_entrymsg"  },
  { "SET STAFFONLY", AC_IRCOP, "help/cservice/set_staffonly" },
  { NULL }
};

/* *INDENT-ON* */

/* HELP <command> [params] */
void cs_help(char *origin)
{
	user_t *u = user_find(origin);
	char *command = strtok(NULL, "");
	char buf[BUFSIZE];
	struct help_command_ *c;
	FILE *help_file;

	if (!command)
	{
		notice(chansvs.nick, origin, "The following core commands are available.");
		notice(chansvs.nick, origin, "\2HELP\2          Displays help information.");
		notice(chansvs.nick, origin, "\2REGISTER\2      Registers a username or channel.");
		notice(chansvs.nick, origin, "\2DROP\2          Drops a registered user or channel.");
		notice(chansvs.nick, origin, "\2LOGIN\2         Logs you into services.");
		notice(chansvs.nick, origin, "\2LOGOUT\2        Logs you out of services.");
		notice(chansvs.nick, origin, "\2SET\2           Sets various control flags.");
		notice(chansvs.nick, origin, "\2STATUS\2        Displays your status in services.");
		notice(chansvs.nick, origin, "\2INFO\2          Displays information on registrations.");
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "The following additional commands are available.");
		notice(chansvs.nick, origin, "\2SOP\2           Manipulates a channel's SOP list.");
		notice(chansvs.nick, origin, "\2AOP\2           Manipulates a channel's AOP list.");
		notice(chansvs.nick, origin, "\2VOP\2           Manipulates a channel's VOP list.");
		notice(chansvs.nick, origin, "\2FLAGS\2         Manipulates specific permissions on a channel.");
		notice(chansvs.nick, origin, "\2AKICK\2         Manipulates a channel's AKICK list.");
		notice(chansvs.nick, origin, "\2OP|DEOP\2       Manipulates ops on a channel.");
		notice(chansvs.nick, origin, "\2VOICE|DEVOICE\2 Manipulates voices on a channel.");
		notice(chansvs.nick, origin, "\2INVITE\2        Invites a nickname to a channel.");
		notice(chansvs.nick, origin, "\2RECOVER\2       Regain control of your channel.");
		notice(chansvs.nick, origin, "\2KICK\2          Removes a user from a channel.");
		notice(chansvs.nick, origin, "\2KICKBAN\2       Removes and bans a user from a channel.");
		notice(chansvs.nick, origin, "\2TOPIC\2         Sets a topic on a channel.");
		notice(chansvs.nick, origin, "\2TOPICAPPEND\2   Appends a topic on a channel.");
		notice(chansvs.nick, origin, "\2BAN\2           Sets a ban on a channel.");
		notice(chansvs.nick, origin, "\2UNBAN\2         Removes a ban on a channel.");
		notice(chansvs.nick, origin, " ");

		if ((is_sra(u->myuser)) || (is_ircop(u)))
		{
			notice(chansvs.nick, origin, "The following IRCop commands are available.");
			notice(chansvs.nick, origin, "\2SENDPASS\2      Email registration passwords.");
			notice(chansvs.nick, origin, " ");
		}

		notice(chansvs.nick, origin, "For more specific help use \2HELP \37command\37\2.");

		return;
	}

	if (!strcasecmp("SET", command))
	{
		notice(chansvs.nick, origin, "Help for \2SET\2:");
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "SET allows you to set various control flags");
		notice(chansvs.nick, origin, "for usernames and channels that change the");
		notice(chansvs.nick, origin, "way certain operations are performed on them.");
		notice(chansvs.nick, origin, " ");
		notice(chansvs.nick, origin, "The following commands are available.");
		notice(chansvs.nick, origin, "\2EMAIL\2         Changes the email address associated " "with a username.");
		notice(chansvs.nick, origin, "\2FOUNDER\2       Sets you founder of a channel.");
		notice(chansvs.nick, origin, "\2HIDEMAIL\2      Hides a username's email address");
		notice(chansvs.nick, origin, "\2MLOCK\2         Sets channel mode lock.");
		notice(chansvs.nick, origin, "\2NEVEROP\2       Prevents services from automatically " "setting modes associated with access lists.");
		notice(chansvs.nick, origin, "\2NOOP\2          Prevents you from being added to " "access lists.");
		notice(chansvs.nick, origin, "\2PASSWORD\2      Change the password of a username or " "channel.");
		notice(chansvs.nick, origin, "\2SECURE\2        Prevents unauthorized people from " "gaining operator status.");
		notice(chansvs.nick, origin, "\2SUCCESSOR\2     Sets a channel successor.");
		notice(chansvs.nick, origin, "\2VERBOSE\2       Notifies channel about access list " "modifications.");
		notice(chansvs.nick, origin, "\2URL\2           Sets the channel URL.");
		notice(chansvs.nick, origin, "\2ENTRYMSG\2      Sets the channel's entry message.");
		notice(chansvs.nick, origin, " ");

		if (is_sra(u->myuser) || is_ircop(u))
		{
			notice(chansvs.nick, origin, "The following IRCop commands are available.");
			notice(chansvs.nick, origin, "\2STAFFONLY\2     Sets the channel as staff-only. (Non staff-members are kickbanned.)");
			notice(chansvs.nick, origin, " ");
		}

		if (is_sra(u->myuser))
		{
			notice(chansvs.nick, origin, "The following SRA commands are available.");
			notice(chansvs.nick, origin, "\2HOLD\2          Prevents services from expiring a username or channel.");
			notice(chansvs.nick, origin, " ");
		}

		notice(chansvs.nick, origin, "For more specific help use \2HELP SET \37command\37\2.");

		return;
	}

	/* take the command through the hash table */
	if ((c = help_cmd_find(chansvs.nick, origin, command, cs_help_commands)))
	{
		if (c->file)
		{
			help_file = fopen(c->file, "r");

			if (!help_file)
			{
				notice(chansvs.nick, origin, "No help available for \2%s\2.", command);
				return;
			}

			while (fgets(buf, BUFSIZE, help_file))
			{
				strip(buf);

				replace(buf, sizeof(buf), "&nick&", chansvs.nick);

				if (buf[0])
					notice(chansvs.nick, origin, "%s", buf);
				else
					notice(chansvs.nick, origin, " ");
			}

			fclose(help_file);
		}
		else
			notice(chansvs.nick, origin, "No help available for \2%s\2.", command);
	}
}
