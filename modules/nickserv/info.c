/*
 * Copyright (c) 2005-2006 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ INFO functions.
 *
 * $Id: info.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/info", FALSE, _modinit, _moddeinit,
	"$Id: info.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_info(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_info = { "INFO", N_("Displays information on registrations."), AC_NONE, 1, ns_cmd_info };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_info, ns_cmdtree);
	help_addentry(ns_helptree, "INFO", "help/nickserv/info", NULL);
}

void _moddeinit()
{
	command_delete(&ns_info, ns_cmdtree);
	help_delentry(ns_helptree, "INFO");
}

static void ns_cmd_info(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mynick_t *mn = NULL;
	user_t *u;
	char *name = parv[0];
	char buf[BUFSIZE], strfbuf[32], lastlogin[32];
	struct tm tm, tm2;
	metadata_t *md;
	node_t *n;
	hook_user_req_t req;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: INFO <nickname>"));
		return;
	}

	if (!(mu = myuser_find_ext(name)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), name);
		return;
	}

	tm = *localtime(&mu->registered);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
	tm2 = *localtime(&mu->lastlogin);
	strftime(lastlogin, sizeof(lastlogin) -1, "%b %d %H:%M:%S %Y", &tm2);

	command_success_nodata(si, _("Information on \2%s\2:"), mu->name);

	command_success_nodata(si, _("Registered: %s (%s ago)"), strfbuf, time_ago(mu->registered));
	if (has_priv(si, PRIV_USER_AUSPEX) && (md = metadata_find(mu, METADATA_USER, "private:host:actual")))
		command_success_nodata(si, _("Last address: %s"), md->value);
	else if ((md = metadata_find(mu, METADATA_USER, "private:host:vhost")))
		command_success_nodata(si, _("Last address: %s"), md->value);

	if (LIST_LENGTH(&mu->logins) == 0)
		command_success_nodata(si, _("Last seen: %s (%s ago)"), lastlogin, time_ago(mu->lastlogin));
	else if (mu == si->smu || has_priv(si, PRIV_USER_AUSPEX))
	{
		buf[0] = '\0';
		LIST_FOREACH(n, mu->logins.head)
		{
			if (strlen(buf) > 80)
			{
				command_success_nodata(si, _("Logins from: %s"), buf);
				buf[0] = '\0';
			}
			if (buf[0])
				strcat(buf, " ");
			strcat(buf, ((user_t *)(n->data))->nick);
		}
		if (buf[0])
			command_success_nodata(si, _("Logins from: %s"), buf);
	}
	else
		command_success_nodata(si, _("Logins from: <hidden>"));

	if (!nicksvs.no_nick_ownership)
	{
		/* describe queried nick */
		mn = mynick_find(*name == '=' ? name + 1 : name);
		if (mn != NULL && mn->owner == mu)
		{
			tm = *localtime(&mn->registered);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
			command_success_nodata(si, _("Nick %s registered: %s (%s ago)"), mn->nick, strfbuf, time_ago(mn->registered));
			u = user_find_named(mn->nick);
			if (u != NULL && u->myuser == mu)
				command_success_nodata(si, _("Nick %s is currently online"), mn->nick);
			else
			{
				tm = *localtime(&mn->lastseen);
				strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
				command_success_nodata(si, _("Nick %s last seen: %s (%s ago)"), mn->nick, strfbuf, time_ago(mn->lastseen));
			}
		}
		/* list registered nicks if privileged */
		if (mu == si->smu || has_priv(si, PRIV_USER_AUSPEX))
		{
			buf[0] = '\0';
			LIST_FOREACH(n, mu->nicks.head)
			{
				if (strlen(buf) > 80)
				{
					command_success_nodata(si, _("Registered nicks: %s"), buf);
					buf[0] = '\0';
				}
				if (buf[0])
					strcat(buf, " ");
				strcat(buf, ((mynick_t *)(n->data))->nick);
			}
			if (buf[0])
				command_success_nodata(si, _("Registered nicks: %s"), buf);
		}
	}

	if (!(mu->flags & MU_HIDEMAIL)
		|| (si->smu == mu || has_priv(si, PRIV_USER_AUSPEX)))
		command_success_nodata(si, _("Email: %s%s"), mu->email,
					(mu->flags & MU_HIDEMAIL) ? " (hidden)": "");

	*buf = '\0';

	if (MU_HIDEMAIL & mu->flags)
		strcat(buf, "HideMail");

	if (MU_HOLD & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "Hold");
	}
	if (MU_NEVEROP & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NeverOp");
	}
	if (MU_NOOP & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NoOp");
	}
	if (MU_NOMEMO & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "NoMemo");
	}
	if (MU_EMAILMEMOS & mu->flags)
	{
		if (*buf)
			strcat(buf, ", ");

		strcat(buf, "EMailMemos");
	}

	if (*buf)
		command_success_nodata(si, _("Flags: %s"), buf);

	if (mu->soper && (mu == si->smu || has_priv(si, PRIV_VIEWPRIVS)))
	{
		command_success_nodata(si, _("Oper class: %s"), mu->soper->operclass ? mu->soper->operclass->name : mu->soper->classname);
	}

	if (has_priv(si, PRIV_USER_AUSPEX) && (md = metadata_find(mu, METADATA_USER, "private:freeze:freezer")))
	{
		char *setter = md->value;
		char *reason;
		time_t ts;

		md = metadata_find(mu, METADATA_USER, "private:freeze:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mu, METADATA_USER, "private:freeze:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		command_success_nodata(si, _("%s was \2FROZEN\2 by %s on %s (%s)"), mu->name, setter, strfbuf, reason);
	}

	if (has_priv(si, PRIV_USER_AUSPEX) && (md = metadata_find(mu, METADATA_USER, "private:mark:setter")))
	{
		char *setter = md->value;
		char *reason;
		time_t ts;

		md = metadata_find(mu, METADATA_USER, "private:mark:reason");
		reason = md != NULL ? md->value : "unknown";

		md = metadata_find(mu, METADATA_USER, "private:mark:timestamp");
		ts = md != NULL ? atoi(md->value) : 0;

		tm = *localtime(&ts);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		command_success_nodata(si, _("%s was \2MARKED\2 by %s on %s (%s)"), mu->name, setter, strfbuf, reason);
	}

	if (MU_WAITAUTH & mu->flags)
		command_success_nodata(si, _("%s has not completed registration verification"), mu->name);

	req.si = si;
	req.mu = mu;
	req.mn = mn;
	hook_call_event("user_info", &req);

	command_success_nodata(si, _("*** \2End of Info\2 ***"));

	logcommand(si, CMDLOG_GET, "INFO %s", mu->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
