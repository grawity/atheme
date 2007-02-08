/*
 * Copyright (c) 2006 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 7067 2006-11-04 20:14:57Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 7067 2006-11-04 20:14:57Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

list_t *ns_cmdtree, *ns_helptree;

command_t ns_set = { "SET", "Sets various control flags.", AC_NONE, 2, ns_cmd_set };

list_t ns_set_cmdtree;

/* HELP SET */
static void ns_help_set(sourceinfo_t *si)
{
	command_success_nodata(si, "Help for \2SET\2:");
	command_success_nodata(si, " ");
	command_success_nodata(si, "SET allows you to set various control flags");
	if (nicksvs.no_nick_ownership)
		command_success_nodata(si, "for accounts that change the way certain operations");
	else
		command_success_nodata(si, "for nicknames that change the way certain operations");
	command_success_nodata(si, "are performed on them.");
	command_success_nodata(si, " ");
	command_help(si, &ns_set_cmdtree);
	command_success_nodata(si, " ");
	command_success_nodata(si, "For more information, use \2/msg %s HELP SET \37command\37\2.", nicksvs.nick);
}

/* SET <setting> <parameters> */
static void ns_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *setting = parv[0];
	command_t *c;

	if (si->smu == NULL)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (setting == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, "Syntax: SET <setting> <parameters>");
		return;
	}

	/* take the command through the hash table */
        if ((c = command_find(&ns_set_cmdtree, setting)))
	{
		command_exec(si->service, si, c, parc - 1, parv + 1);
	}
	else
	{
		command_fail(si, fault_badparams, "Invalid set command. Use \2/%s%s HELP SET\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", nicksvs.nick);
	}
}

/* SET EMAIL <new address> */
static void _ns_setemail(sourceinfo_t *si, int parc, char *parv[])
{
	char *email = parv[0];

	if (si->smu == NULL)
		return;

	if (email == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "EMAIL");
		command_fail(si, fault_needmoreparams, "Syntax: SET EMAIL <new e-mail>");
		return;
	}

	if (strlen(email) >= EMAILLEN)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "EMAIL");
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_noprivs, "Please verify your original registration before changing your e-mail address.");
		return;
	}

	if (!validemail(email))
	{
		command_fail(si, fault_badparams, "\2%s\2 is not a valid email address.", email);
		return;
	}

	if (!strcasecmp(si->smu->email, email))
	{
		command_fail(si, fault_badparams, "The email address for \2%s\2 is already set to \2%s\2.", si->smu->name, si->smu->email);
		return;
	}

	snoop("SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", si->smu->name, si->smu->email, email);

	if (me.auth == AUTH_EMAIL)
	{
		unsigned long key = makekey();

		metadata_add(si->smu, METADATA_USER, "private:verify:emailchg:key", itoa(key));
		metadata_add(si->smu, METADATA_USER, "private:verify:emailchg:newemail", email);
		metadata_add(si->smu, METADATA_USER, "private:verify:emailchg:timestamp", itoa(time(NULL)));

		if (!sendemail(si->su != NULL ? si->su : si->service->me, EMAIL_SETEMAIL, si->smu, itoa(key)))
		{
			command_fail(si, fault_emailfail, "Sending email failed, sorry! Your email address is unchanged.");
			metadata_delete(si->smu, METADATA_USER, "private:verify:emailchg:key");
			metadata_delete(si->smu, METADATA_USER, "private:verify:emailchg:newemail");
			metadata_delete(si->smu, METADATA_USER, "private:verify:emailchg:timestamp");
			return;
		}

		logcommand(si, CMDLOG_SET, "SET EMAIL %s (awaiting verification)", email);
		command_success_nodata(si, "An email containing email changing instructions has been sent to \2%s\2.", email);
		command_success_nodata(si, "Your email address will not be changed until you follow these instructions.");

		return;
	}

	strlcpy(si->smu->email, email, EMAILLEN);

	logcommand(si, CMDLOG_SET, "SET EMAIL %s", email);
	command_success_nodata(si, "The email address for \2%s\2 has been changed to \2%s\2.", si->smu->name, si->smu->email);
}

command_t ns_set_email = { "EMAIL", "Changes your e-mail address.", AC_NONE, 1, _ns_setemail };

/* SET HIDEMAIL [ON|OFF] */
static void _ns_sethidemail(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "HIDEMAIL");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & si->smu->flags)
		{
			command_fail(si, fault_nochange, "The \2HIDEMAIL\2 flag is already set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET HIDEMAIL ON");

		si->smu->flags |= MU_HIDEMAIL;

		command_success_nodata(si, "The \2HIDEMAIL\2 flag has been set for \2%s\2.", si->smu->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & si->smu->flags))
		{
			command_fail(si, fault_nochange, "The \2HIDEMAIL\2 flag is not set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET HIDEMAIL OFF");

		si->smu->flags &= ~MU_HIDEMAIL;

		command_success_nodata(si, "The \2HIDEMAIL\2 flag has been removed for \2%s\2.", si->smu->name);

		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "HIDEMAIL");
		return;
	}
}

command_t ns_set_hidemail = { "HIDEMAIL", "Hides your e-mail address.", AC_NONE, 1, _ns_sethidemail };

static void _ns_setemailmemos(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_noprivs, "You have to verify your email address before you can enable emailing memos.");
		return;
	}

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "EMAILMEMOS");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (me.mta == NULL)
		{
			command_fail(si, fault_emailfail, "Sending email is administratively disabled.");
			return;
		}
		if (MU_EMAILMEMOS & si->smu->flags)
		{
			command_fail(si, fault_nochange, "The \2EMAILMEMOS\2 flag is already set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET EMAILMEMOS ON");
		si->smu->flags |= MU_EMAILMEMOS;
		command_success_nodata(si, "The \2EMAILMEMOS\2 flag has been set for \2%s\2.", si->smu->name);
		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_EMAILMEMOS & si->smu->flags))
		{
			command_fail(si, fault_nochange, "The \2EMAILMEMOS\2 flag is not set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET EMAILMEMOS OFF");
		si->smu->flags &= ~MU_EMAILMEMOS;
		command_success_nodata(si, "The \2EMAILMEMOS\2 flag has been removed for \2%s\2.", si->smu->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "EMAILMEMOS");
		return;
	}
}

command_t ns_set_emailmemos = { "EMAILMEMOS", "Forwards incoming memos to your e-mail address.", AC_NONE, 1, _ns_setemailmemos };

static void _ns_setnomemo(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOMEMO");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOMEMO & si->smu->flags)
		{
			command_fail(si, fault_nochange, "The \2NOMEMO\2 flag is already set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOMEMO ON");
		si->smu->flags |= MU_NOMEMO;
		command_success_nodata(si, "The \2NOMEMO\2 flag has been set for \2%s\2.", si->smu->name);
		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOMEMO & si->smu->flags))
		{
			command_fail(si, fault_nochange, "The \2NOMEMO\2 flag is not set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOMEMO OFF");
		si->smu->flags &= ~MU_NOMEMO;
		command_success_nodata(si, "The \2NOMEMO\2 flag has been removed for \2%s\2.", si->smu->name);
		return;
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOMEMO");
		return;
	}
}

command_t ns_set_nomemo = { "NOMEMO", "Disables the ability to recieve memos.", AC_NONE, 1, _ns_setnomemo };

static void _ns_setneverop(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NEVEROP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVEROP & si->smu->flags)
		{
			command_fail(si, fault_nochange, "The \2NEVEROP\2 flag is already set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NEVEROP ON");

		si->smu->flags |= MU_NEVEROP;

		command_success_nodata(si, "The \2NEVEROP\2 flag has been set for \2%s\2.", si->smu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVEROP & si->smu->flags))
		{
			command_fail(si, fault_nochange, "The \2NEVEROP\2 flag is not set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NEVEROP OFF");

		si->smu->flags &= ~MU_NEVEROP;

		command_success_nodata(si, "The \2NEVEROP\2 flag has been removed for \2%s\2.", si->smu->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NEVEROP");
		return;
	}
}

command_t ns_set_neverop = { "NEVEROP", "Prevents you from being added to access lists.", AC_NONE, 1, _ns_setneverop };

static void _ns_setnoop(sourceinfo_t *si, int parc, char *parv[])
{
	char *params = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (params == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NOOP");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & si->smu->flags)
		{
			command_fail(si, fault_nochange, "The \2NOOP\2 flag is already set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOOP ON");

		si->smu->flags |= MU_NOOP;

		command_success_nodata(si, "The \2NOOP\2 flag has been set for \2%s\2.", si->smu->name);

		return;
	}
	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & si->smu->flags))
		{
			command_fail(si, fault_nochange, "The \2NOOP\2 flag is not set for \2%s\2.", si->smu->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "SET NOOP OFF");

		si->smu->flags &= ~MU_NOOP;

		command_success_nodata(si, "The \2NOOP\2 flag has been removed for \2%s\2.", si->smu->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "NOOP");
		return;
	}
}

command_t ns_set_noop = { "NOOP", "Prevents services from setting modes upon you automatically.", AC_NONE, 1, _ns_setnoop };

static void _ns_setproperty(sourceinfo_t *si, int parc, char *parv[])
{
	char *property = strtok(parv[0], " ");
	char *value = strtok(NULL, "");

	if (si->smu == NULL)
		return;

	if (!property)
	{
		command_fail(si, fault_needmoreparams, "Syntax: SET PROPERTY <property> [value]");
		return;
	}

	if (strchr(property, ':') && !has_priv(si, PRIV_METADATA))
	{
		command_fail(si, fault_badparams, "Invalid property name.");
		return;
	}

	if (strchr(property, ':'))
		snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", si->smu->name, property, value);

	if (si->smu->metadata.count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, "Cannot add \2%s\2 to \2%s\2 metadata table, it is full.",
					property, si->smu->name);
		return;
	}

	if (!value)
	{
		metadata_t *md = metadata_find(si->smu, METADATA_USER, property);

		if (!md)
		{
			command_fail(si, fault_nosuch_target, "Metadata entry \2%s\2 was not set.", property);
			return;
		}

		metadata_delete(si->smu, METADATA_USER, property);
		logcommand(si, CMDLOG_SET, "SET PROPERTY %s (deleted)", property);
		command_success_nodata(si, "Metadata entry \2%s\2 has been deleted.", property);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300)
	{
		command_fail(si, fault_badparams, "Parameters are too long. Aborting.");
		return;
	}

	metadata_add(si->smu, METADATA_USER, property, value);
	logcommand(si, CMDLOG_SET, "SET PROPERTY %s to %s", property, value);
	command_success_nodata(si, "Metadata entry \2%s\2 added.", property);
}

command_t ns_set_property = { "PROPERTY", "Manipulates metadata entries associated with a nickname.", AC_NONE, 2, _ns_setproperty };

static void _ns_setpassword(sourceinfo_t *si, int parc, char *parv[])
{
	char *password = strtok(parv[0], " ");

	if (si->smu == NULL)
		return;

	if (password == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "PASSWORD");
		return;
	}

	if (strlen(password) > 32)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "PASSWORD");
		return;
	}

	if (!strcasecmp(password, si->smu->name))
	{
		command_fail(si, fault_badparams, "You cannot use your nickname as a password.");
		command_fail(si, fault_badparams, "Syntax: SET PASSWORD <new password>");
		return;
	}

	/*snoop("SET:PASSWORD: \2%s\2 as \2%s\2 for \2%s\2", si->su->user, si->smu->name, si->smu->name);*/
	logcommand(si, CMDLOG_SET, "SET PASSWORD");

	set_password(si->smu, password);

	command_success_nodata(si, "The password for \2%s\2 has been changed to \2%s\2. Please write this down for future reference.", si->smu->name, password);

	return;
}

command_t ns_set_password = { "PASSWORD", "Changes the password associated with your nickname.", AC_NONE, 1, _ns_setpassword };

command_t *ns_set_commands[] = {
	&ns_set_email,
	&ns_set_emailmemos,
	&ns_set_hidemail,
	&ns_set_nomemo,
	&ns_set_noop,
	&ns_set_neverop,
	&ns_set_password,
	&ns_set_property,
	NULL
};

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");
	command_add(&ns_set, ns_cmdtree);

	help_addentry(ns_helptree, "SET", NULL, ns_help_set);
	help_addentry(ns_helptree, "SET EMAIL", "help/nickserv/set_email", NULL);
	help_addentry(ns_helptree, "SET EMAILMEMOS", "help/nickserv/set_emailmemos", NULL);
	help_addentry(ns_helptree, "SET HIDEMAIL", "help/nickserv/set_hidemail", NULL);
	help_addentry(ns_helptree, "SET NOMEMO", "help/nickserv/set_nomemo", NULL);
	help_addentry(ns_helptree, "SET NEVEROP", "help/nickserv/set_neverop", NULL);
	help_addentry(ns_helptree, "SET NOOP", "help/nickserv/set_noop", NULL);
	help_addentry(ns_helptree, "SET PASSWORD", "help/nickserv/set_password", NULL);
	help_addentry(ns_helptree, "SET PROPERTY", "help/nickserv/set_property", NULL);

	/* populate ns_set_cmdtree */
	command_add_many(ns_set_commands, &ns_set_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_set, ns_cmdtree);
	help_delentry(ns_helptree, "SET");
	help_delentry(ns_helptree, "SET EMAIL");
	help_delentry(ns_helptree, "SET EMAILMEMOS");
	help_delentry(ns_helptree, "SET HIDEMAIL");
	help_delentry(ns_helptree, "SET NOMEMO");
	help_delentry(ns_helptree, "SET NEVEROP");
	help_delentry(ns_helptree, "SET NOOP");
	help_delentry(ns_helptree, "SET PASSWORD");
	help_delentry(ns_helptree, "SET PROPERTY");

	/* clear ns_set_cmdtree */
	command_delete_many(ns_set_commands, &ns_set_cmdtree);
}
