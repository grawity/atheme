/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: oservice.c 222 2005-05-30 01:05:54Z nenolod $
 */

#include "atheme.h"

struct command_ os_commands[] = {
	{"HELP", AC_IRCOP, os_help},
	{"GLOBAL", AC_IRCOP, gs_global},	/* Might as well have it here... --nenolod */
	{"KLINE", AC_IRCOP, os_kline},
	{"MODE", AC_IRCOP, os_mode},
	{"UPDATE", AC_SRA, os_update},
	{"REHASH", AC_SRA, os_rehash},
	{"RAW", AC_SRA, os_raw},
	{"INJECT", AC_SRA, os_inject},
	{"RESTART", AC_SRA, os_restart},
	{"SHUTDOWN", AC_SRA, os_shutdown},
	{NULL}
};

/* main services client routine */
void oservice(char *origin, uint8_t parc, char *parv[])
{
	char *cmd, *s;
	char orig[BUFSIZE];
	struct command_ *c;

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[1], " ");

	if (!cmd)
		return;

	/* ctcp? case-sensitive as per rfc */
	if (!strcmp(cmd, "\001PING"))
	{
		if (!(s = strtok(NULL, " ")))
			s = " 0 ";

		strip(s);
		notice(opersvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(opersvs.nick, origin,
		       "\001VERSION atheme-%s. %s %s %s%s%s%s%s%s%s%s%s TS5ow\001",
		       version, revision, me.name,
		       (match_mapping) ? "A" : "",
		       (me.loglevel & LG_DEBUG) ? "d" : "",
		       (me.auth) ? "e" : "",
		       (config_options.flood_msgs) ? "F" : "",
		       (config_options.leave_chans) ? "l" : "",
		       (config_options.join_chans) ? "j" : "",
		       (config_options.leave_chans) ? "l" : "", (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");

		return;
	}
	else if (!strcmp(cmd, "\001CLIENTINFO\001"))
	{
		/* easter egg :X */
		notice(opersvs.nick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	/* take the command through the hash table */
	if ((c = cmd_find(opersvs.nick, origin, cmd, os_commands)))
		if (c->func)
			c->func(origin);
}
