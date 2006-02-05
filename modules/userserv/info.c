/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ INFO functions.
 *
 * $Id: info.c 4743 2006-01-31 02:22:42Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/info", FALSE, _modinit, _moddeinit,
	"$Id: info.c 4743 2006-01-31 02:22:42Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_info(char *origin);

command_t us_info = { "INFO", "Displays information on registrations.", AC_NONE, us_cmd_info };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_info, us_cmdtree);
	help_addentry(us_helptree, "INFO", "help/userserv/info", NULL);
}

void _moddeinit()
{
	command_delete(&us_info, us_cmdtree);
	help_delentry(us_helptree, "INFO");
}

static void us_cmd_info(char *origin)
{
	user_t *u = user_find_named(origin);
	myuser_t *mu;
	char *name = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32], lastlogin[32];
	struct tm tm, tm2;
	metadata_t *md;
	node_t *n;

	if (!name)
	{
		notice(usersvs.nick, origin, STR_INSUFFICIENT_PARAMS, "INFO");
		notice(usersvs.nick, origin, "Syntax: INFO <account>");
		return;
	}

	if (!(mu = myuser_find_ext(name)))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	tm = *localtime(&mu->registered);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
	tm2 = *localtime(&mu->lastlogin);
	strftime(lastlogin, sizeof(lastlogin) -1, "%b %d %H:%M:%S %Y", &tm2);


	notice(usersvs.nick, origin, "Information on \2%s\2:", mu->name);

	notice(usersvs.nick, origin, "Registered: %s (%s ago)", strfbuf, time_ago(mu->registered));
	if (has_priv(u, PRIV_USER_AUSPEX) && (md = metadata_find(mu, METADATA_USER, "private:host:actual")))
		notice(usersvs.nick, origin, "Last address: %s", md->value);
	else if (md = metadata_find(mu, METADATA_USER, "private:host:vhost"))
		notice(usersvs.nick, origin, "Last address: %s", md->value);

	if (LIST_LENGTH(&mu->logins) == 0)
		notice(usersvs.nick, origin, "Last seen: %s (%s ago)", lastlogin, time_ago(mu->lastlogin));
	else if (mu == u->myuser || has_priv(u, PRIV_USER_AUSPEX))
	{
		buf[0] = '\0';
		LIST_FOREACH(n, mu->logins.head)
		{
			if (strlen(buf) > 80)
			{
				notice(usersvs.nick, origin, "Logins from: %s", buf);
				buf[0] = '\0';
			}
			if (buf[0])
				strcat(buf, " ");
			strcat(buf, ((user_t *)(n->data))->nick);
		}
		if (buf[0])
			notice(usersvs.nick, origin, "Logins from: %s", buf);
	}
	else
		notice(usersvs.nick, origin, "Logins from: <hidden>");


	if (!(mu->flags & MU_HIDEMAIL)
		|| (u->myuser == mu || has_priv(u, PRIV_USER_AUSPEX)))
		notice(usersvs.nick, origin, "Email: %s%s", mu->email,
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
		notice(usersvs.nick, origin, "Flags: %s", buf);

	if (mu->soper && (mu == u->myuser || has_priv(u, PRIV_VIEWPRIVS)))
	{
		notice(usersvs.nick, origin, "Oper class: %s", mu->soper->operclass ? mu->soper->operclass->name : "*");
	}

        if (has_priv(u, PRIV_USER_AUSPEX) && (md = metadata_find(mu, METADATA_USER, "private:freeze:freezer")))
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

                notice(usersvs.nick, origin, "%s was \2FROZEN\2 by %s on %s (%s)", mu->name, setter, strfbuf, reason);
        }

	if (has_priv(u, PRIV_USER_AUSPEX) && (md = metadata_find(mu, METADATA_USER, "private:mark:setter")))
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

		notice(usersvs.nick, origin, "%s was \2MARKED\2 by %s on %s (%s)", mu->name, setter, strfbuf, reason);
	}

	if ((MU_WAITAUTH & mu->flags) && has_priv(u, PRIV_USER_AUSPEX))
		notice(usersvs.nick, origin, "%s has not completed registration verification", mu->name);

	notice(usersvs.nick, origin, "*** \2End of Info\2 ***");

	logcommand(usersvs.me, u, CMDLOG_GET, "INFO %s", mu->name);
}
