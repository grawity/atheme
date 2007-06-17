/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGISTER function.
 *
 * $Id: register.c 8279 2007-05-20 06:47:41Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 8279 2007-05-20 06:47:41Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_register(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_register = { "REGISTER", N_("Registers a nickname."), AC_NONE, 3, ns_cmd_register };

list_t *ns_cmdtree, *ns_helptree;

unsigned int tcnt = 0;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_register, ns_cmdtree);
	help_addentry(ns_helptree, "REGISTER", "help/nickserv/register", NULL);
}

void _moddeinit()
{
	command_delete(&ns_register, ns_cmdtree);
	help_delentry(ns_helptree, "REGISTER");
}

static int register_foreach_cb(dictionary_elem_t *delem, void *privdata)
{
	char *email = (char *) privdata;
	myuser_t *tmu = (myuser_t *) delem->node.data;

	if (!strcasecmp(email, tmu->email))
		tcnt++;

	/* optimization: if tcnt >= me.maxusers, quit iterating. -nenolod */
	if (tcnt >= me.maxusers)
		return -1;

	return 0;
}

static void ns_cmd_register(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	mynick_t *mn;
	node_t *n;
	char *account;
	char *pass = parv[0];
	char *email = parv[1];
	char lau[BUFSIZE], lao[BUFSIZE];
	hook_user_register_check_t hdata;

	if (si->smu)
	{
		command_fail(si, fault_already_authed, _("You are already logged in."));
		return;
	}

	if (nicksvs.no_nick_ownership || si->su == NULL)
		account = parv[0], pass = parv[1], email = parv[2];
	else
		account = si->su->nick, pass = parv[0], email = parv[1];

	if (!account || !pass || !email)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "REGISTER");
		if (nicksvs.no_nick_ownership || si->su == NULL)
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <account> <password> <email>"));
		else
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <password> <email>"));
		return;
	}

	if ((strlen(pass) > 32) || (strlen(email) >= EMAILLEN))
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "REGISTER");
		return;
	}

	if (!nicksvs.no_nick_ownership && si->su == NULL && user_find_named(account))
	{
		command_fail(si, fault_noprivs, _("A user matching this account is already on IRC."));
		return;
	}

	if (!nicksvs.no_nick_ownership && IsDigit(*account))
	{
		command_fail(si, fault_badparams, _("For security reasons, you can't register your UID."));
		command_fail(si, fault_badparams, _("Please change to a real nickname, and try again."));
		return;
	}

	if (nicksvs.no_nick_ownership || si->su == NULL)
	{
		if (strchr(account, ' ') || strchr(account, '\n') || strchr(account, '\r') || account[0] == '=' || account[0] == '#' || account[0] == '@' || account[0] == '+' || account[0] == '%' || account[0] == '!' || strchr(account, ','))
		{
			command_fail(si, fault_badparams, _("The account name \2%s\2 is invalid."), account);
			return;
		}
	}

	if ((si->su != NULL && !strcasecmp(pass, si->su->nick)) || !strcasecmp(pass, account))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		if (nicksvs.no_nick_ownership || si->su == NULL)
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <account> <password> <email>"));
		else
			command_fail(si, fault_needmoreparams, _("Syntax: REGISTER <password> <email>"));
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid email address."), email);
		return;
	}

	/* make sure it isn't registered already */
	if (nicksvs.no_nick_ownership ? myuser_find(account) != NULL : mynick_find(account) != NULL)
	{
		command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered."), account);
		return;
	}

	hdata.si = si;
	hdata.account = account;
	hdata.email = email;
	hdata.approved = 0;
	hook_call_event("user_can_register", &hdata);
	if (hdata.approved != 0)
		return;

	/* make sure they're within limits */
	tcnt = 0;
	dictionary_foreach(mulist, register_foreach_cb, email);

	if (tcnt >= me.maxusers)
	{
		command_fail(si, fault_toomany, _("\2%s\2 has too many nicknames registered."), email);
		return;
	}

	mu = myuser_add(account, pass, email, config_options.defuflags | MU_NOBURSTLOGIN);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;
	if (!nicksvs.no_nick_ownership)
	{
		mn = mynick_add(mu, mu->name);
		mn->registered = CURRTIME;
		mn->lastseen = CURRTIME;
	}

	if (me.auth == AUTH_EMAIL)
	{
		char *key = gen_pw(12);
		mu->flags |= MU_WAITAUTH;

		metadata_add(mu, METADATA_USER, "private:verify:register:key", key);
		metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", itoa(time(NULL)));

		if (!sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_REGISTER, mu, key))
		{
			command_fail(si, fault_emailfail, _("Sending email failed, sorry! Registration aborted."));
			object_unref(mu);
			free(key);
			return;
		}

		command_success_nodata(si, _("An email containing nickname activation instructions has been sent to \2%s\2."), mu->email);
		command_success_nodata(si, _("If you do not complete registration within one day, your nickname will expire."));

		free(key);
	}

	if (si->su != NULL)
	{
		si->su->myuser = mu;
		n = node_create();
		node_add(si->su, n, &mu->logins);

		if (!(mu->flags & MU_WAITAUTH))
			/* only grant ircd registered status if it's verified */
			ircd_on_login(si->su->nick, mu->name, NULL);
	}

	if (!nicksvs.no_nick_ownership && si->su != NULL)
		snoop("REGISTER: \2%s\2 to \2%s\2", account, email);
	else
		snoop("REGISTER: \2%s\2 to \2%s\2 by \2%s\2", account, email,
				si->su != NULL ? si->su->nick : get_source_name(si));
	logcommand(si, CMDLOG_REGISTER, "REGISTER to %s", email);
	if (is_soper(mu))
	{
		wallops("%s registered the nick \2%s\2 and gained services operator privileges.", get_oper_name(si), mu->name);
		snoop("SOPER: \2%s\2 as \2%s\2", get_oper_name(si), mu->name);
	}

	command_success_nodata(si, _("\2%s\2 is now registered to \2%s\2."), mu->name, mu->email);
	command_success_nodata(si, _("The password is \2%s\2. Please write this down for future reference."), pass);
	hook_call_event("user_register", mu);

	if (si->su != NULL)
	{
		snprintf(lau, BUFSIZE, "%s@%s", si->su->user, si->su->vhost);
		metadata_add(mu, METADATA_USER, "private:host:vhost", lau);

		snprintf(lao, BUFSIZE, "%s@%s", si->su->user, si->su->host);
		metadata_add(mu, METADATA_USER, "private:host:actual", lao);
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
