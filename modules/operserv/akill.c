/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements
 * the OperServ AKILL command.
 *
 * $Id: akill.c 6927 2006-10-24 15:22:05Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/akill", FALSE, _modinit, _moddeinit,
	"$Id: akill.c 6927 2006-10-24 15:22:05Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_akill_newuser(void *vptr);

static void os_cmd_akill(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_akill_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_akill_del(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_akill_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_akill_sync(sourceinfo_t *si, int parc, char *parv[]);


command_t os_akill = { "AKILL", "Manages network bans.", PRIV_AKILL, 3, os_cmd_akill };

command_t os_akill_add = { "ADD", "Adds a network ban", AC_NONE, 2, os_cmd_akill_add };
command_t os_akill_del = { "DEL", "Deletes a network ban", AC_NONE, 1, os_cmd_akill_del };
command_t os_akill_list = { "LIST", "Lists all network bans", AC_NONE, 1, os_cmd_akill_list };
command_t os_akill_sync = { "SYNC", "Synchronises network bans to servers", AC_NONE, 0, os_cmd_akill_sync };

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_akill_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

        command_add(&os_akill, os_cmdtree);

	/* Add sub-commands */
	command_add(&os_akill_add, &os_akill_cmds);
	command_add(&os_akill_del, &os_akill_cmds);
	command_add(&os_akill_list, &os_akill_cmds);
	command_add(&os_akill_sync, &os_akill_cmds);

	help_addentry(os_helptree, "AKILL", "help/oservice/akill", NULL);

	hook_add_event("user_add");
	hook_add_hook("user_add", os_akill_newuser);
}

void _moddeinit()
{
	command_delete(&os_akill, os_cmdtree);

	/* Delete sub-commands */
	command_delete(&os_akill_add, &os_akill_cmds);
	command_delete(&os_akill_del, &os_akill_cmds);
	command_delete(&os_akill_list, &os_akill_cmds);
	command_delete(&os_akill_sync, &os_akill_cmds);
	
	help_delentry(os_helptree, "AKILL");

	hook_del_hook("user_add", os_akill_newuser);
}

static void os_akill_newuser(void *vptr)
{
	user_t *u;
	kline_t *k;

	u = vptr;
	if (is_internal_client(u))
		return;
	k = kline_find_user(u);
	if (k != NULL)
	{
		/* Server didn't have that kline, send it again.
		 * To ensure kline exempt works on akills too, do
		 * not send a KILL. -- jilles */
		kline_sts(u->server->name, k->user, k->host, k->duration ? k->expires - CURRTIME : 0, k->reason);
	}
}

static void os_cmd_akill(sourceinfo_t *si, int parc, char *parv[])
{
	/* Grab args */
	char *cmd = parv[0];
        command_t *c;
	
	/* Bad/missing arg */
	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL");
		command_fail(si, fault_needmoreparams, "Syntax: AKILL ADD|DEL|LIST");
		return;
	}

        c = command_find(&os_akill_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", opersvs.me->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void os_cmd_akill_add(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u;
        char *target = parv[0];
	char *token = strtok(parv[1], " ");
	char *treason, reason[BUFSIZE];
	long duration;
	char *s;
	kline_t *k;

	if (!target || !token)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
		command_fail(si, fault_needmoreparams, "Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] <reason>");
		return;
	}

	if (!strcasecmp(token, "!P"))
	{
		duration = 0;
		treason = strtok(NULL, "");

		if (treason)
			strlcpy(reason, treason, BUFSIZE);
		else
			strlcpy(reason, "No reason given", BUFSIZE);
	}
	else if (!strcasecmp(token, "!T"))
	{
		s = strtok(NULL, " ");
		treason = strtok(NULL, "");
		if (treason)
			strlcpy(reason, treason, BUFSIZE);
		else
			strlcpy(reason, "No reason given", BUFSIZE);
		if (s)
		{
			duration = (atol(s) * 60);
			while (isdigit(*s))
				s++;
			if (*s == 'h' || *s == 'H')
				duration *= 60;
			else if (*s == 'd' || *s == 'D')
				duration *= 1440;
			else if (*s == 'w' || *s == 'W')
				duration *= 10080;
			else if (*s == '\0')
				;
			else
				duration = 0;
			if (duration == 0)
			{
				command_fail(si, fault_badparams, "Invalid duration given.");
				command_fail(si, fault_badparams, "Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] " "<reason>");
				return;
			}
		}
		else {
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
			command_fail(si, fault_needmoreparams, "Syntax: AKILL ADD <nick|hostmask> [!P|!T <minutes>] " "<reason>");
			return;
		}

	}
	else
	{
		duration = config_options.kline_time;
		strlcpy(reason, token, BUFSIZE);
		treason = strtok(NULL, "");

		if (treason)
		{
			strlcat(reason, " ", BUFSIZE);
			strlcat(reason, treason, BUFSIZE);
		}			
	}

	if (strchr(target,'!'))
	{
		command_fail(si, fault_badparams, "Invalid character '%c' in user@host.", '!');
		return;
	}

	if (!(strchr(target, '@')))
	{
		if (!(u = user_find_named(target)))
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not on IRC.", target);
			return;
		}

		if (is_internal_client(u))
			return;

		if ((k = kline_find("*", u->host)))
		{
			command_fail(si, fault_nochange, "AKILL \2*@%s\2 is already matched in the database.", u->host);
			return;
		}

		k = kline_add("*", u->host, reason, duration);
		k->setby = sstrdup(get_oper_name(si));
	}
	else
	{
		char *userbuf = strtok(target, "@");
		char *hostbuf = strtok(NULL, "");
		char *p;
		int i = 0;

		if (!userbuf || !hostbuf)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL ADD");
			command_fail(si, fault_needmoreparams, "Syntax: AKILL ADD <user>@<host> [options] <reason>");
			return;
		}

		if (strchr(hostbuf,'@'))
		{
			command_fail(si, fault_badparams, "Too many '%c' in user@host.", '@');
			command_fail(si, fault_badparams, "Syntax: AKILL ADD <user>@<host> [options] <reason>");
			return;
		}

		/* make sure there's at least 4 non-wildcards */
		for (p = userbuf; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}
		for (p = hostbuf; *p; p++)
		{
			if (*p != '*' && *p != '?' && *p != '.')
				i++;
		}

		if (i < 4)
		{
			command_fail(si, fault_badparams, "Invalid user@host: \2%s@%s\2. At least four non-wildcard characters are required.", userbuf, hostbuf);
			return;
		}

		if ((k = kline_find(userbuf, hostbuf)))
		{
			command_fail(si, fault_nochange, "AKILL \2%s@%s\2 is already matched in the database.", userbuf, hostbuf);
			return;
		}

		k = kline_add(userbuf, hostbuf, reason, duration);
		k->setby = sstrdup(get_oper_name(si));
	}

	if (duration)
		command_success_nodata(si, "Timed AKILL on \2%s@%s\2 was successfully added and will expire in %s.", k->user, k->host, timediff(duration));
	else
		command_success_nodata(si, "AKILL on \2%s@%s\2 was successfully added.", k->user, k->host);

	snoop("AKILL:ADD: \2%s@%s\2 by \2%s\2 for \2%s\2", k->user, k->host, get_oper_name(si), k->reason);

	verbose_wallops("\2%s\2 is \2adding\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2", get_oper_name(si), k->user, k->host, 
		k->reason);
	logcommand(si, CMDLOG_SET, "AKILL ADD %s@%s %s", k->user, k->host, k->reason);
}

static void os_cmd_akill_del(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *userbuf, *hostbuf;
	uint32_t number;
	char *s;
	kline_t *k;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "AKILL DEL");
		command_fail(si, fault_needmoreparams, "Syntax: AKILL DEL <hostmask>");
		return;
	}

	if (strchr(target, ','))
	{
		uint32_t start = 0, end = 0, i;
		char t[16];

		s = strtok(target, ",");

		do
		{
			if (strchr(s, ':'))
			{
				for (i = 0; *s != ':'; s++, i++)
					t[i] = *s;

				t[++i] = '\0';
				start = atoi(t);

				s++;	/* skip past the : */

				for (i = 0; *s != '\0'; s++, i++)
					t[i] = *s;

				t[++i] = '\0';
				end = atoi(t);

				for (i = start; i <= end; i++)
				{
					if (!(k = kline_find_num(i)))
					{
						command_fail(si, fault_nosuch_target, "No such AKILL with number \2%d\2.", i);
						continue;
					}

					command_success_nodata(si, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);
					verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
						get_oper_name(si), k->user, k->host, k->reason);

					snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, get_oper_name(si));
					logcommand(si, CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
					kline_delete(k->user, k->host);
				}

				continue;
			}

			number = atoi(s);

			if (!(k = kline_find_num(number)))
			{
				command_fail(si, fault_nosuch_target, "No such AKILL with number \2%d\2.", number);
				return;
			}

			command_success_nodata(si, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);
			verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
				get_oper_name(si), k->user, k->host, k->reason);

			snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, get_oper_name(si));
			logcommand(si, CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
			kline_delete(k->user, k->host);
		} while ((s = strtok(NULL, ",")));

		return;
	}

	if (!strchr(target, '@'))
	{
		uint32_t start = 0, end = 0, i;
		char t[16];

		if (strchr(target, ':'))
		{
			for (i = 0; *target != ':'; target++, i++)
				t[i] = *target;

			t[++i] = '\0';
			start = atoi(t);

			target++;	/* skip past the : */

			for (i = 0; *target != '\0'; target++, i++)
				t[i] = *target;

			t[++i] = '\0';
			end = atoi(t);

			for (i = start; i <= end; i++)
			{
				if (!(k = kline_find_num(i)))
				{
					command_fail(si, fault_nosuch_target, "No such AKILL with number \2%d\2.", i);
					continue;
				}

				command_success_nodata(si, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);
				verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
					get_oper_name(si), k->user, k->host, k->reason);

				snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, get_oper_name(si));
				kline_delete(k->user, k->host);
			}

			return;
		}

		number = atoi(target);

		if (!(k = kline_find_num(number)))
		{
			command_fail(si, fault_nosuch_target, "No such AKILL with number \2%d\2.", number);
			return;
		}

		command_success_nodata(si, "AKILL on \2%s@%s\2 has been successfully removed.", k->user, k->host);

		verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
			get_oper_name(si), k->user, k->host, k->reason);

		snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, get_oper_name(si));
		logcommand(si, CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
		kline_delete(k->user, k->host);
		return;
	}

	userbuf = strtok(target, "@");
	hostbuf = strtok(NULL, "");

	if (!(k = kline_find(userbuf, hostbuf)))
	{
		command_fail(si, fault_nosuch_target, "No such AKILL: \2%s@%s\2.", userbuf, hostbuf);
		return;
	}

	command_success_nodata(si, "AKILL on \2%s@%s\2 has been successfully removed.", userbuf, hostbuf);

	verbose_wallops("\2%s\2 is \2removing\2 an \2AKILL\2 for \2%s@%s\2 -- reason: \2%s\2",
		get_oper_name(si), k->user, k->host, k->reason);

	snoop("AKILL:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, get_oper_name(si));
	logcommand(si, CMDLOG_SET, "AKILL DEL %s@%s", k->user, k->host);
	kline_delete(userbuf, hostbuf);
}

static void os_cmd_akill_list(sourceinfo_t *si, int parc, char *parv[])
{
	char *param = parv[0];
	boolean_t full = FALSE;
	node_t *n;
	kline_t *k;

	if (param != NULL && !strcasecmp(param, "FULL"))
		full = TRUE;
	
	if (full)
		command_success_nodata(si, "AKILL list (with reasons):");
	else
		command_success_nodata(si, "AKILL list:");

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		if (k->duration && full)
			command_success_nodata(si, "%d: %s@%s - by \2%s\2 - expires in \2%s\2 - (%s)", k->number, k->user, k->host, k->setby, timediff(k->expires > CURRTIME ? k->expires - CURRTIME : 0), k->reason);
		else if (k->duration && !full)
			command_success_nodata(si, "%d: %s@%s - by \2%s\2 - expires in \2%s\2", k->number, k->user, k->host, k->setby, timediff(k->expires > CURRTIME ? k->expires - CURRTIME : 0));
		else if (!k->duration && full)
			command_success_nodata(si, "%d: %s@%s - by \2%s\2 - \2permanent\2 - (%s)", k->number, k->user, k->host, k->setby, k->reason);
		else
			command_success_nodata(si, "%d: %s@%s - by \2%s\2 - \2permanent\2", k->number, k->user, k->host, k->setby);
	}

	command_success_nodata(si, "Total of \2%d\2 %s in AKILL list.", klnlist.count, (klnlist.count == 1) ? "entry" : "entries");
	logcommand(si, CMDLOG_GET, "AKILL LIST%s", full ? " FULL" : "");
}

static void os_cmd_akill_sync(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	kline_t *k;

	logcommand(si, CMDLOG_DO, "AKILL SYNC");
	snoop("AKILL:SYNC: \2%s\2", get_oper_name(si));

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		if (k->duration == 0)
			kline_sts("*", k->user, k->host, 0, k->reason);
		else if (k->expires > CURRTIME)
			kline_sts("*", k->user, k->host, k->expires - CURRTIME, k->reason);
	}

	command_success_nodata(si, "AKILL list synchronized to servers.");
}
