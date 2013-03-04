/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: cservice.c 227 2005-05-30 01:48:09Z nenolod $
 */

#include "atheme.h"

struct command_ cs_commands[] = {
	{"LOGIN", AC_NONE, cs_login},
	{"LOGOUT", AC_NONE, cs_logout},
	{"HELP", AC_NONE, cs_help},
	{"SET", AC_NONE, cs_set},
	{"SOP", AC_NONE, cs_sop},
	{"AOP", AC_NONE, cs_aop},
	{"VOP", AC_NONE, cs_vop},
	{"OP", AC_NONE, cs_op},
	{"DEOP", AC_NONE, cs_deop},
	{"VOICE", AC_NONE, cs_voice},
	{"DEVOICE", AC_NONE, cs_devoice},
	{"INVITE", AC_NONE, cs_invite},
	{"INFO", AC_NONE, cs_info},
	{"RECOVER", AC_NONE, cs_recover},
	{"REGISTER", AC_NONE, cs_register},
	{"DROP", AC_NONE, cs_drop},
	{"FLAGS", AC_NONE, cs_flags},
	{"STATUS", AC_NONE, cs_status},
	{"AKICK", AC_NONE, cs_akick},
	{"KICK", AC_NONE, cs_kick},
	{"KICKBAN", AC_NONE, cs_kickban},
	{"TOPIC", AC_NONE, cs_topic},
	{"TOPICAPPEND", AC_NONE, cs_topicappend},
	{"BAN", AC_NONE, cs_ban},
	{"UNBAN", AC_NONE, cs_unban},
	{"SENDPASS", AC_IRCOP, cs_sendpass},
	{NULL}
};


/* main services client routine */
void cservice(char *origin, uint8_t parc, char *parv[])
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
		notice(chansvs.nick, origin, "\001PING %s\001", s);
		return;
	}
	else if (!strcmp(cmd, "\001VERSION\001"))
	{
		notice(chansvs.nick, origin,
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
		notice(chansvs.nick, origin, "\001CLIENTINFO 114 97 107 97 117 114\001");
		return;
	}

	/* ctcps we don't care about are ignored */
	else if (*cmd == '\001')
		return;

	/* take the command through the hash table */
	if ((c = cmd_find(chansvs.nick, origin, cmd, cs_commands)))
		if (c->func)
			c->func(origin);
}
