/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENCE.
 *
 * This file contains functionality implementing OperServ RWATCH.
 *
 * $Id: rwatch.c 7959 2007-03-20 12:25:45Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rwatch", FALSE, _modinit, _moddeinit,
	"$Id: rwatch.c 7959 2007-03-20 12:25:45Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void rwatch_newuser(void *);

static void os_cmd_rwatch(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_del(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_rwatch_set(sourceinfo_t *si, int parc, char *parv[]);

static void write_rwatchdb(void);
static void load_rwatchdb(void);

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_rwatch_cmds;

list_t rwatch_list;

#define RWACT_SNOOP 1
#define RWACT_KLINE 2

typedef struct rwatch_ rwatch_t;
struct rwatch_
{
	char *regex;
	int reflags; /* AREGEX_* */
	char *reason;
	int actions; /* RWACT_* */
	regex_t *re;
};

command_t os_rwatch = { "RWATCH", N_("Performs actions on connecting clients matching regexes."), PRIV_USER_AUSPEX, 2, os_cmd_rwatch };

command_t os_rwatch_add = { "ADD", N_("Adds an entry to the regex watch list."), AC_NONE, 1, os_cmd_rwatch_add };
command_t os_rwatch_del = { "DEL", N_("Removes an entry from the regex watch list."), AC_NONE, 1, os_cmd_rwatch_del };
command_t os_rwatch_list = { "LIST", N_("Displays the regex watch list."), AC_NONE, 1, os_cmd_rwatch_list };
command_t os_rwatch_set = { "SET", N_("Changes actions on an entry in the regex watch list"), AC_NONE, 1, os_cmd_rwatch_set };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_rwatch, os_cmdtree);

	command_add(&os_rwatch_add, &os_rwatch_cmds);
	command_add(&os_rwatch_del, &os_rwatch_cmds);
	command_add(&os_rwatch_list, &os_rwatch_cmds);
	command_add(&os_rwatch_set, &os_rwatch_cmds);

	help_addentry(os_helptree, "RWATCH", "help/oservice/rwatch", NULL);

	hook_add_event("user_add");
	hook_add_hook("user_add", rwatch_newuser);

	load_rwatchdb();
}

void _moddeinit(void)
{
	node_t *n, *tn;

	LIST_FOREACH_SAFE(n, tn, rwatch_list.head)
	{
		rwatch_t *rw = n->data;

		free(rw->regex);
		free(rw->reason);
		if (rw->re != NULL)
			regex_destroy(rw->re);
		free(rw);

		node_del(n, &rwatch_list);
		node_free(n);
	}

	command_delete(&os_rwatch, os_cmdtree);

	command_delete(&os_rwatch_add, &os_rwatch_cmds);
	command_delete(&os_rwatch_del, &os_rwatch_cmds);
	command_delete(&os_rwatch_list, &os_rwatch_cmds);
	command_delete(&os_rwatch_set, &os_rwatch_cmds);

	help_delentry(os_helptree, "RWATCH");

	hook_del_hook("user_add", rwatch_newuser);
}

static void write_rwatchdb(void)
{
	FILE *f;
	node_t *n;
	rwatch_t *rw;

	if (!(f = fopen(DATADIR "/rwatch.db.new", "w")))
	{
		slog(LG_ERROR, "write_rwatchdb(): cannot write rwatch database: %s", strerror(errno));
		return;
	}

	LIST_FOREACH(n, rwatch_list.head)
	{
		rw = n->data;
		fprintf(f, "RW %d %s\n", rw->reflags, rw->regex);
		fprintf(f, "RR %d %s\n", rw->actions, rw->reason);
	}

	fclose(f);

	if ((rename(DATADIR "/rwatch.db.new", DATADIR "/rwatch.db")) < 0)
	{
		slog(LG_ERROR, "write_rwatchdb(): couldn't rename rwatch database.");
		return;
	}
}

static void load_rwatchdb(void)
{
	FILE *f;
	char *item, rBuf[BUFSIZE * 2];
	rwatch_t *rw = NULL;

	if (!(f = fopen(DATADIR "/rwatch.db", "r")))
	{
		slog(LG_DEBUG, "load_rwatchdb(): cannot open rwatch database: %s", strerror(errno));
		return;
	}

	while (fgets(rBuf, BUFSIZE * 2, f))
	{
		item = strtok(rBuf, " ");
		strip(item);

		if (!strcmp(item, "RW"))
		{
			char *reflagsstr = strtok(NULL, " ");
			char *regex = strtok(NULL, "\n");

			if (!reflagsstr || !regex || rw)
				; /* erroneous, don't add */
			else
			{
				rw = (rwatch_t *)smalloc(sizeof(rwatch_t));

				rw->regex = sstrdup(regex);
				rw->reflags = atoi(reflagsstr);
				rw->re = regex_create(rw->regex, rw->reflags);
			}
		}
		else if (!strcmp(item, "RR"))
		{
			char *actionstr = strtok(NULL, " ");
			char *reason = strtok(NULL, "\n");

			if (!actionstr || !reason || !rw)
				; /* erroneous, don't add */
			else
			{
				rw->actions = atoi(actionstr);
				rw->reason = sstrdup(reason);
				node_add(rw, node_create(), &rwatch_list);
				rw = NULL;
			}
		}
	}

	fclose(f);
}

static void os_cmd_rwatch(sourceinfo_t *si, int parc, char *parv[])
{
	/* Grab args */
	char *cmd = parv[0];
	command_t *c;

	/* Bad/missing arg */
	if (!cmd)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH ADD|DEL|LIST|SET"));
		return;
	}

	c = command_find(&os_rwatch_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == FALSE) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void os_cmd_rwatch_add(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	char *pattern;
	char *reason;
	regex_t *regex;
	rwatch_t *rw;
	int flags;
	char *args = parv[0];

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH ADD /<regex>/[i] <reason>"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH ADD");
		command_fail(si, fault_badparams, _("Syntax: RWATCH ADD /<regex>/[i] <reason>"));
		return;
	}

	reason = args;
	while (*reason == ' ')
		reason++;
	if (*reason == '\0')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH ADD /<regex>/[i] <reason>"));
		return;
	}

	LIST_FOREACH(n, rwatch_list.head)
	{
		rwatch_t *t = n->data;

		if (!strcmp(pattern, t->regex))
		{
			command_fail(si, fault_nochange, _("\2%s\2 already found in regex watch list; not adding."), pattern);
			return;
		}
	}

	regex = regex_create(pattern, flags);
	if (regex == NULL)
	{
		command_fail(si, fault_badparams, _("The provided regex \2%s\2 is invalid."), pattern);
		return;
	}

	rw = malloc(sizeof(rwatch_t));
	rw->regex = sstrdup(pattern);
	rw->reflags = flags;
	rw->reason = sstrdup(reason);
	rw->actions = RWACT_SNOOP;
	rw->re = regex;

	node_add(rw, node_create(), &rwatch_list);
	command_success_nodata(si, _("Added \2%s\2 to regex watch list."), pattern);
	snoop("RWATCH:ADD: \2%s\2 by \2%s\2", pattern, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "RWATCH ADD %s %s", pattern, reason);
	write_rwatchdb();
}

static void os_cmd_rwatch_del(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n, *tn;
	char *pattern;
	int flags;
	char *args = parv[0];

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH DEL /<regex>/[i]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH DEL");
		command_fail(si, fault_badparams, _("Syntax: RWATCH DEL /<regex>/[i]"));
		return;
	}

	LIST_FOREACH_SAFE(n, tn, rwatch_list.head)
	{
		rwatch_t *rw = n->data;

		if (!strcmp(rw->regex, pattern))
		{
			if (rw->actions & RWACT_KLINE)
			{
				if (!has_priv(si, PRIV_MASS_AKILL))
				{
					command_fail(si, fault_noprivs, _("You do not have %s privilege."), PRIV_MASS_AKILL);
					return;
				}
				wallops("\2%s\2 disabled kline on regex watch pattern \2%s\2", get_oper_name(si), pattern);
			}
			free(rw->regex);
			free(rw->reason);
			if (rw->re != NULL)
				regex_destroy(rw->re);
			free(rw);
			node_del(n, &rwatch_list);
			node_free(n);
			command_success_nodata(si, _("Removed \2%s\2 from regex watch list."), pattern);
			snoop("RWATCH:DEL: \2%s\2 by \2%s\2", pattern, get_oper_name(si));
			logcommand(si, CMDLOG_ADMIN, "RWATCH DEL %s", pattern);
			write_rwatchdb();
			return;
		}
	}

	command_fail(si, fault_nochange, _("\2%s\2 not found in regex watch list."), pattern);
}

static void os_cmd_rwatch_list(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;

	LIST_FOREACH(n, rwatch_list.head)
	{
		rwatch_t *rw = n->data;

		command_success_nodata(si, "%s (%s%s%s) - %s",
				rw->regex,
				rw->reflags & AREGEX_ICASE ? "i" : "",
				rw->actions & RWACT_SNOOP ? "S" : "",
				rw->actions & RWACT_KLINE ? "\2K\2" : "",
				rw->reason);
	}
	command_success_nodata(si, _("End of RWATCH LIST"));
	logcommand(si, CMDLOG_GET, "RWATCH LIST");
}

static void os_cmd_rwatch_set(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n, *tn;
	char *pattern;
	char *opts;
	int addflags = 0, removeflags = 0;
	int flags;
	char *args = parv[0];

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH SET");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH SET");
		command_fail(si, fault_badparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
		return;
	}
	while (*args == ' ')
		args++;

	if (*args == '\0')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RWATCH SET");
		command_fail(si, fault_needmoreparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
		return;
	}

	opts = args;
	while (*args != '\0')
	{
		if (!strncasecmp(args, "KLINE", 5))
			addflags |= RWACT_KLINE, removeflags &= ~RWACT_KLINE, args += 5;
		else if (!strncasecmp(args, "NOKLINE", 7))
			removeflags |= RWACT_KLINE, addflags &= ~RWACT_KLINE, args += 7;
		else if (!strncasecmp(args, "SNOOP", 5))
			addflags |= RWACT_SNOOP, removeflags &= ~RWACT_SNOOP, args += 5;
		else if (!strncasecmp(args, "NOSNOOP", 7))
			removeflags |= RWACT_SNOOP, addflags &= ~RWACT_SNOOP, args += 7;

		if (*args != '\0' && *args != ' ')
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RWATCH SET");
			command_fail(si, fault_badparams, _("Syntax: RWATCH SET /<regex>/[i] [KLINE] [NOKLINE] [SNOOP] [NOSNOOP]"));
			return;
		}
		while (*args == ' ')
			args++;
	}

	if ((addflags | removeflags) & RWACT_KLINE && !has_priv(si, PRIV_MASS_AKILL))
	{
		command_fail(si, fault_noprivs, _("You do not have %s privilege."), PRIV_MASS_AKILL);
		return;
	}

	LIST_FOREACH_SAFE(n, tn, rwatch_list.head)
	{
		rwatch_t *rw = n->data;

		if (!strcmp(rw->regex, pattern))
		{
			if (((~rw->actions & addflags) | (rw->actions & removeflags)) == 0)
			{
				command_fail(si, fault_nochange, _("Options for \2%s\2 unchanged."), pattern);
				return;
			}
			rw->actions |= addflags;
			rw->actions &= ~removeflags;
			command_success_nodata(si, _("Set options \2%s\2 on \2%s\2."), opts, pattern);
			snoop("RWATCH:SET: \2%s\2 \2%s\2 by \2%s\2", pattern, opts, get_oper_name(si));
			if (addflags & RWACT_KLINE)
				wallops("\2%s\2 enabled kline on regex watch pattern \2%s\2", get_oper_name(si), pattern);
			if (removeflags & RWACT_KLINE)
				wallops("\2%s\2 disabled kline on regex watch pattern \2%s\2", get_oper_name(si), pattern);
			logcommand(si, CMDLOG_ADMIN, "RWATCH SET %s %s", pattern, opts);
			write_rwatchdb();
			return;
		}
	}

	command_fail(si, fault_nosuch_target, _("\2%s\2 not found in regex watch list."), pattern);
}

static void rwatch_newuser(void *vptr)
{
	user_t *u = vptr;
	char usermask[NICKLEN+USERLEN+HOSTLEN+GECOSLEN];
	node_t *n;
	rwatch_t *rw;

	if (is_internal_client(u))
		return;

	snprintf(usermask, sizeof usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

	LIST_FOREACH(n, rwatch_list.head)
	{
		rw = n->data;
		if (!rw->re)
			continue;
		if (regex_match(rw->re, usermask))
		{
			if (rw->actions & RWACT_SNOOP)
			{
				snoop("RWATCH:%s %s matches \2%s\2 (%s)",
						rw->actions & RWACT_KLINE ? "KLINE:" : "",
						usermask, rw->regex, rw->reason);
			}
			if (rw->actions & RWACT_KLINE)
			{
				slog(LG_INFO, "rwatch_newuser(): klining *@%s (user %s!%s@%s matches %s %s)",
						u->host, u->nick, u->user, u->host,
						rw->regex, rw->reason);
				kline_sts("*", "*", u->host, 86400, rw->reason);
			}
		}
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
