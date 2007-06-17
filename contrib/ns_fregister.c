/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ FREGISTER function.
 *
 * $Id: ns_fregister.c 7789 2007-03-04 00:00:48Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/fregister", FALSE, _modinit, _moddeinit,
	"$Id: ns_fregister.c 7789 2007-03-04 00:00:48Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_fregister(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_fregister = { "FREGISTER", "Registers a nickname on behalf of another user.", PRIV_USER_FREGISTER, 20, ns_cmd_fregister };

list_t *ns_cmdtree, *ns_helptree;

unsigned int tcnt = 0;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_fregister, ns_cmdtree);
	help_addentry(ns_helptree, "FREGISTER", "help/nickserv/fregister", NULL);
}

void _moddeinit()
{
	command_delete(&ns_fregister, ns_cmdtree);
	help_delentry(ns_helptree, "FREGISTER");
}

static void ns_cmd_fregister(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mynick_t *mn;
	node_t *n;
	char *account;
	char *pass;
	char *email;
	int i, uflags = 0;

	account = parv[0], pass = parv[1], email = parv[2];

	if (!account || !pass || !email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREGISTER");
		command_fail(si, fault_needmoreparams, "Syntax: FREGISTER <account> <password> <email> [CRYPTPASS]");
		return;
	}

	for (i = 3; i < parc; i++)
	{
		if (!strcasecmp(parv[i], "CRYPTPASS"))
			uflags |= MU_CRYPTPASS;
		else if (!strcasecmp(parv[i], "HIDEMAIL"))
			uflags |= MU_HIDEMAIL;
		else if (!strcasecmp(parv[i], "NOOP"))
			uflags |= MU_NOOP;
		else if (!strcasecmp(parv[i], "NEVEROP"))
			uflags |= MU_NEVEROP;
	}

	if ((!(uflags & MU_CRYPTPASS) && strlen(pass) > 32) ||
			strlen(email) >= EMAILLEN)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FREGISTER");
		return;
	}

	if (!nicksvs.no_nick_ownership && IsDigit(*account))
	{
		command_fail(si, fault_badparams, "For security reasons, you can't register your UID.");
		return;
	}

	if (strchr(account, ' ') || strchr(account, '\n') || strchr(account, '\r') || account[0] == '=' || account[0] == '#' || account[0] == '@' || account[0] == '+' || account[0] == '%' || account[0] == '!' || strchr(account, ','))
	{
		command_fail(si, fault_badparams, "The account name \2%s\2 is invalid.", account);
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, "\2%s\2 is not a valid email address.", email);
		return;
	}

	/* make sure it isn't registered already */
	if (nicksvs.no_nick_ownership ? myuser_find(account) != NULL : mynick_find(account) != NULL)
	{
		command_fail(si, fault_alreadyexists, "\2%s\2 is already registered.", account);
		return;
	}

	mu = myuser_add(account, pass, email, uflags | config_options.defuflags | MU_NOBURSTLOGIN);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_add(mu, mu->name);
		mn->registered = CURRTIME;
		mn->lastseen = CURRTIME;
	}

	snoop("FREGISTER: \2%s\2 to \2%s\2 by \2%s\2", account, email,
			get_oper_name(si));
	logcommand(si, CMDLOG_REGISTER, "FREGISTER %s to %s", account, email);
	if (is_soper(mu))
	{
		wallops("%s used FREGISTER on account \2%s\2 with services operator privileges.", get_oper_name(si), mu->name);
		snoop("SOPER: \2%s\2", mu->name);
	}

	command_success_nodata(si, "\2%s\2 is now registered to \2%s\2.", mu->name, mu->email);
	hook_call_event("user_register", mu);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
