/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Implements nickserv RETURN.
 *
 * $Id: return.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/return", FALSE, _modinit, _moddeinit,
	"$Id: return.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_return(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_return = { "RETURN", N_("Returns a nickname to its owner."), PRIV_USER_ADMIN, 2, ns_cmd_return };

list_t *ns_cmdtree, *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_return, ns_cmdtree);
	help_addentry(ns_helptree, "RETURN", "help/nickserv/return", NULL);
}

void _moddeinit()
{
	command_delete(&ns_return, ns_cmdtree);
	help_delentry(ns_helptree, "RETURN");
}

static void ns_cmd_return(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *newmail = parv[1];
	char *newpass;
	char oldmail[EMAILLEN];
	myuser_t *mu;
	user_t *u;
	node_t *n, *tn;

	if (!target || !newmail)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RETURN");
		command_fail(si, fault_needmoreparams, _("Usage: RETURN <nickname> <e-mail address>"));
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	if (is_soper(mu))
	{
		logcommand(si, CMDLOG_ADMIN, "failed RETURN %s to %s (is SOPER)", target, newmail);
		command_fail(si, fault_badparams, _("\2%s\2 belongs to a services operator; it cannot be returned."), target);
		return;
	}

	if ((strlen(newmail) > 32) || !validemail(newmail))
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not a valid e-mail address."), newmail);
		return;
	}

	newpass = gen_pw(12);
	strlcpy(oldmail, mu->email, EMAILLEN);
	strlcpy(mu->email, newmail, EMAILLEN);

	if (!sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_SENDPASS, mu, newpass))
	{
		strlcpy(mu->email, oldmail, EMAILLEN);
		command_fail(si, fault_emailfail, _("Sending email failed, nickname \2%s\2 remains with \2%s\2."),
				mu->name, mu->email);
		return;
	}

	set_password(mu, newpass);

	free(newpass);

	/* prevents users from "stealing it back" in the event of a takeover */
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:key");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:newemail");
	metadata_delete(mu, METADATA_USER, "private:verify:emailchg:timestamp");
	metadata_delete(mu, METADATA_USER, "private:setpass:key");
	/* log them out */
	LIST_FOREACH_SAFE(n, tn, mu->logins.head)
	{
		u = (user_t *)n->data;
		if (!ircd_on_logout(u->nick, mu->name, NULL))
		{
			u->myuser = NULL;
			node_del(n, &mu->logins);
			node_free(n);
		}
	}
	mu->flags |= MU_NOBURSTLOGIN;

	wallops("%s returned the nickname \2%s\2 to \2%s\2", get_oper_name(si), target, newmail);
	snoop("RETURN: \2%s\2 to \2%s\2 by \2%s\2", target, newmail, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "RETURN %s to %s", target, newmail);
	command_success_nodata(si, _("The e-mail address for \2%s\2 has been set to \2%s\2"),
						target, newmail);
	command_success_nodata(si, _("A random password has been set; it has been sent to \2%s\2."),
						newmail);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
