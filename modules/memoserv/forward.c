/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the Memoserv FORWARD function
 *
 * $Id: forward.c 8329 2007-05-27 13:31:59Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"memoserv/forward", FALSE, _modinit, _moddeinit,
	"$Id: forward.c 8329 2007-05-27 13:31:59Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ms_cmd_forward(sourceinfo_t *si, int parc, char *parv[]);

command_t ms_forward = { "FORWARD", N_(N_("Forwards a memo.")),
                        AC_NONE, 2, ms_cmd_forward };

list_t *ms_cmdtree;
list_t *ms_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ms_cmdtree, "memoserv/main", "ms_cmdtree");
	MODULE_USE_SYMBOL(ms_helptree, "memoserv/main", "ms_helptree");

        command_add(&ms_forward, ms_cmdtree);
	help_addentry(ms_helptree, "forward", "help/memoserv/forward", NULL);
}

void _moddeinit()
{
	command_delete(&ms_forward, ms_cmdtree);
	help_delentry(ms_helptree, "FORWARD");
}

static void ms_cmd_forward(sourceinfo_t *si, int parc, char *parv[])
{
	/* Misc structs etc */
	user_t *tu;
	myuser_t *tmu;
	mymemo_t *memo, *newmemo;
	node_t *n, *temp;
	unsigned int i = 1, memonum = 0;
	
	/* Grab args */
	char *target = parv[0];
	char *arg = parv[1];
	
	/* Arg validator */
	if (!target || !arg)
	{
		command_fail(si, fault_needmoreparams, 
			STR_INSUFFICIENT_PARAMS, "FORWARD");
		
		command_fail(si, fault_needmoreparams, 
			"Syntax: FORWARD <account> <memo number>");
		
		return;
	}
	else
		memonum = atoi(arg);
	
	/* user logged in? */
	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}
	
	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, _("You need to verify your email address before you may send memos."));
		return;
	}

	/* Check to see if any memos */
	if (!si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("You have no memos to forward."));
		return;
	}

	/* Check to see if target user exists */
	if (!(tmu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}
	
	/* Make sure target isn't sender */
	if (si->smu == tmu)
	{
		command_fail(si, fault_noprivs, _("You cannot send yourself a memo."));
		return;
	}
	
	/* Make sure arg is an int */
	if (!memonum)
	{
		command_fail(si, fault_badparams, _("Invalid message index."));
		return;
	}
	
	/* check if targetuser has nomemo set */
	if (tmu->flags & MU_NOMEMO)
	{
		command_fail(si, fault_noprivs,
			"\2%s\2 does not wish to receive memos.", target);

		return;
	}

	/* Check to see if memo n exists */
	if (memonum > si->smu->memos.count)
	{
		command_fail(si, fault_nosuch_key, _("Invalid memo number."));
		return;
	}
	
	/* Check to make sure target inbox not full */
	if (tmu->memos.count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, _("Target inbox is full."));
		logcommand(si, CMDLOG_SET, "failed FORWARD to %s (target inbox full)", tmu->name);
		return;
	}

	/* rate limit it -- jilles */
	if (CURRTIME - si->smu->memo_ratelimit_time > MEMO_MAX_TIME)
		si->smu->memo_ratelimit_num = 0;
	if (si->smu->memo_ratelimit_num > MEMO_MAX_NUM && !has_priv(si, PRIV_FLOOD))
	{
		command_fail(si, fault_toomany, _("Too many memos; please wait a while and try again"));
		return;
	}
	si->smu->memo_ratelimit_num++;
	si->smu->memo_ratelimit_time = CURRTIME;

	/* Make sure we're not on ignore */
	LIST_FOREACH(n, tmu->memo_ignores.head)
	{
		if (!strcasecmp((char *)n->data, si->smu->name))
		{
			/* Lie... change this if you want it to fail silent */
			logcommand(si, CMDLOG_SET, "failed FORWARD to %s (on ignore list)", tmu->name);
			command_success_nodata(si, _("The memo has been successfully forwarded to \2%s\2."), target);
			return;
		}
	}
	logcommand(si, CMDLOG_SET, "FORWARD to %s", tmu->name);
	
	/* Go to forwarding memos */
	LIST_FOREACH(n, si->smu->memos.head)
	{
		if (i == memonum)
		{
			/* should have some function for send here...  ask nenolod*/
			memo = (mymemo_t *)n->data;
			newmemo = smalloc(sizeof(mymemo_t));
			
			/* Create memo */
			newmemo->sent = CURRTIME;
			newmemo->status = 0;
			strlcpy(newmemo->sender,si->smu->name,NICKLEN);
			strlcpy(newmemo->text,memo->text,MEMOLEN);
			
			/* Create node, add to their linked list of memos */
			temp = node_create();
			node_add(newmemo, temp, &tmu->memos);
			tmu->memoct_new++;
		
			/* Should we email this? */
			if (tmu->flags & MU_EMAILMEMOS)
			{
				if (sendemail(si->su, EMAIL_MEMO, tmu, memo->text))
				{
					command_success_nodata(si, _("Your memo has been emailed to \2%s\2."), target);
					return;
				}
			}
		}
		i++;
	}

	/* Note: do not disclose other nicks they're logged in with
	 * -- jilles */
	tu = user_find_named(target);
	if (tu != NULL && tu->myuser == tmu)
	{
		command_success_nodata(si, _("%s is currently online, and you may talk directly, by sending a private message."), target);
	}
	if (si->su == NULL || !irccasecmp(si->su->nick, si->smu->name))
		myuser_notice(memosvs.nick, tmu, "You have a new forwarded memo from %s.", si->smu->name);
	else
		myuser_notice(memosvs.nick, tmu, "You have a new forwarded memo from %s (nick: %s).", si->smu->name, si->su->nick);

	command_success_nodata(si, _("The memo has been successfully forwarded to \2%s\2."), target);
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
