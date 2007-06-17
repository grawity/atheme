/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: main.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/main", FALSE, _modinit, _moddeinit,
	"$Id: main.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void on_user_identify(void *vptr);
static void on_user_away(void *vptr);

list_t ms_cmdtree;
list_t ms_helptree;

/* main services client routine */
static void memoserv(sourceinfo_t *si, int parc, char *parv[])
{
	char *cmd;
        char *text;
	char orig[BUFSIZE];

	/* this should never happen */
	if (parv[0][0] == '&')
	{
		slog(LG_ERROR, "services(): got parv with local channel: %s", parv[0]);
		return;
	}

	/* make a copy of the original for debugging */
	strlcpy(orig, parv[parc - 1], BUFSIZE);

	/* lets go through this to get the command */
	cmd = strtok(parv[parc - 1], " ");
	text = strtok(NULL, "");

	if (!cmd)
		return;
	if (*cmd == '\001')
	{
		handle_ctcp_common(si, cmd, text);
		return;
	}

	/* take the command through the hash table */
	command_exec_split(si->service, si, cmd, text, &ms_cmdtree);
}

static void memoserv_config_ready(void *unused)
{
	if (memosvs.me)
		del_service(memosvs.me);

	memosvs.me = add_service(memosvs.nick, memosvs.user,
				 memosvs.host, memosvs.real,
				 memoserv, &ms_cmdtree);
	memosvs.disp = memosvs.me->disp;

        hook_del_hook("config_ready", memoserv_config_ready);
}

void _modinit(module_t *m)
{
        hook_add_event("config_ready");
        hook_add_hook("config_ready", memoserv_config_ready);
	
	hook_add_event("user_identify");
	hook_add_hook("user_identify", on_user_identify);

	hook_add_event("user_away");
	hook_add_hook("user_away", on_user_away);

        if (!cold_start)
        {
                memosvs.me = add_service(memosvs.nick, memosvs.user,
			memosvs.host, memosvs.real, memoserv, &ms_cmdtree);
                memosvs.disp = memosvs.me->disp;
        }
}

void _moddeinit(void)
{
        if (memosvs.me)
	{
                del_service(memosvs.me);
		memosvs.me = NULL;
	}
}

static void on_user_identify(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu = u->myuser;
	
	if (mu->memoct_new > 0)
	{
		notice(memosvs.nick, u->nick, ngettext(N_("You have %d new memo."),
						       N_("You have %d new memos."),
						       mu->memoct_new), mu->memoct_new);
	}
}

static void on_user_away(void *vptr)
{
	user_t *u = vptr;
	myuser_t *mu;
	mynick_t *mn;

	if (u->flags & UF_AWAY)
		return;
	mu = u->myuser;
	if (mu == NULL)
	{
		mn = mynick_find(u->nick);
		if (mn != NULL && myuser_access_verify(u, mn->owner))
			mu = mn->owner;
	}
	if (mu == NULL)
		return;
	if (mu->memoct_new > 0)
	{
		notice(memosvs.nick, u->nick, ngettext(N_("You have %d new memo."),
						       N_("You have %d new memos."),
						       mu->memoct_new), mu->memoct_new);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
