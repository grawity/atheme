/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGISTER function.
 *
 * $Id: register.c 3779 2005-11-10 21:49:28Z pfish $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"userserv/register", FALSE, _modinit, _moddeinit,
	"$Id: register.c 3779 2005-11-10 21:49:28Z pfish $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void us_cmd_register(char *origin);

command_t us_register = { "REGISTER", "Registers a services account.", 
			  AC_NONE, us_cmd_register };

list_t *us_cmdtree, *us_helptree;

void _modinit(module_t *m)
{
	us_cmdtree = module_locate_symbol("userserv/main", "us_cmdtree");
	us_helptree = module_locate_symbol("userserv/main", "us_helptree");
	command_add(&us_register, us_cmdtree);
	help_addentry(us_helptree, "REGISTER", "help/userserv/register", NULL);
}

void _moddeinit()
{
	command_delete(&us_register, us_cmdtree);
	help_delentry(us_helptree, "REGISTER");
}

static void us_cmd_register(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu, *tmu;
	node_t *n;
	char *account = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");
	char *email = strtok(NULL, " ");
	char lau[BUFSIZE], lao[BUFSIZE];
	uint32_t i, tcnt;

	if (u->myuser)
	{
		notice(usersvs.nick, origin, "You are already logged in.");
		return;
	}

	if (!account || !pass || !email)
	{
		notice(usersvs.nick, origin, "Insufficient parameters specified for \2REGISTER\2.");
		notice(usersvs.nick, origin, "Syntax: REGISTER <account> <password> <email>");
		return;
	}

	if ((strlen(pass) > 32) || (strlen(email) >= EMAILLEN))
	{
		notice(usersvs.nick, origin, "Invalid parameters specified for \2REGISTER\2.");
		return;
	}

	if (!strcasecmp(pass, account))
	{
		notice(usersvs.nick, origin, "You cannot use your account name as a password.");
		notice(usersvs.nick, origin, "Syntax: REGISTER <account> <password> <email>");
		return;
	}

	if (!validemail(email))
	{
		notice(usersvs.nick, origin, "\2%s\2 is not a valid email address.", email);
		return;
	}

	/* make sure it isn't registered already */
	mu = myuser_find(account);
	if (mu != NULL)
	{
		/* should we reveal the e-mail address? (from us_info.c) */
		if (!(mu->flags & MU_HIDEMAIL)
			|| (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
			notice(usersvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mu->name, mu->email);
		else
			notice(usersvs.nick, origin, "\2%s\2 is already registered.", mu->name);

		return;
	}

	/* make sure they're within limits */
	for (i = 0, tcnt = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			tmu = (myuser_t *)n->data;

			if (!strcasecmp(email, tmu->email))
				tcnt++;
		}
	}

	if (tcnt >= me.maxusers)
	{
		notice(usersvs.nick, origin, "\2%s\2 has too many accounts registered.", email);
		return;
	}

	mu = myuser_add(origin, pass, email, config_options.defuflags);
	mu->registered = CURRTIME;
	mu->lastlogin = CURRTIME;

	if (me.auth == AUTH_EMAIL)
	{
		char *key = gen_pw(12);
		mu->flags |= MU_WAITAUTH;

		metadata_add(mu, METADATA_USER, "private:verify:register:key", key);
		metadata_add(mu, METADATA_USER, "private:verify:register:timestamp", itoa(time(NULL)));

		if (!sendemail(u, EMAIL_REGISTER, mu, key))
		{
			notice(usersvs.nick, origin, "Sending email failed, sorry! Registration aborted.");
			myuser_delete(mu->name);
			free(key);
			return;
		}

		notice(usersvs.nick, origin, "An email containing account activiation instructions has been sent to \2%s\2.", mu->email);
		notice(usersvs.nick, origin, "If you do not complete registration within one day your account will expire.");

		free(key);
	}

	u->myuser = mu;
	n = node_create();
	node_add(u, n, &mu->logins);

	if (!(mu->flags & MU_WAITAUTH))
		/* only grant ircd registered status if it's verified */
		ircd_on_login(origin, mu->name, NULL);

	snoop("REGISTER: \2%s\2 to \2%s\2", account, email);
	logcommand(usersvs.me, u, CMDLOG_REGISTER, "REGISTER to %s", email);

	notice(usersvs.nick, origin, "\2%s\2 is now registered to \2%s\2.", mu->name, mu->email);
	notice(usersvs.nick, origin, "The password is \2%s\2. Please write this down for future reference.", pass);
	hook_call_event("user_register", mu);

	snprintf(lau, BUFSIZE, "%s@%s", u->user, u->vhost);
	metadata_add(mu, METADATA_USER, "private:host:vhost", lau);

	snprintf(lao, BUFSIZE, "%s@%s", u->user, u->host);
	metadata_add(mu, METADATA_USER, "private:host:actual", lao);

}
