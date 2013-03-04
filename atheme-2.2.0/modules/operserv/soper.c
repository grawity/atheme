/*
 * Copyright (c) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dynamic services operator privileges
 *
 * $Id: soper.c 7895 2007-03-06 02:40:03Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/soper", FALSE, _modinit, _moddeinit,
	"$Id: soper.c 7895 2007-03-06 02:40:03Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void os_cmd_soper(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_list(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_listclass(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_add(sourceinfo_t *si, int parc, char *parv[]);
static void os_cmd_soper_del(sourceinfo_t *si, int parc, char *parv[]);

command_t os_soper = { "SOPER", N_("Shows and changes services operator privileges."), AC_NONE, 3, os_cmd_soper };

command_t os_soper_list = { "LIST", N_("Lists services operators."), PRIV_VIEWPRIVS, 0, os_cmd_soper_list };
command_t os_soper_listclass = { "LISTCLASS", N_("Lists operclasses."), PRIV_VIEWPRIVS, 0, os_cmd_soper_listclass };
command_t os_soper_add = { "ADD", N_("Grants services operator privileges to an account."), PRIV_GRANT, 2, os_cmd_soper_add };
command_t os_soper_del = { "DEL", N_("Removes services operator privileges from an account."), PRIV_GRANT, 1, os_cmd_soper_del };

list_t *os_cmdtree;
list_t *os_helptree;
list_t os_soper_cmds;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_soper, os_cmdtree);
	command_add(&os_soper_list, &os_soper_cmds);
	command_add(&os_soper_listclass, &os_soper_cmds);
	command_add(&os_soper_add, &os_soper_cmds);
	command_add(&os_soper_del, &os_soper_cmds);

	help_addentry(os_helptree, "SOPER", "help/oservice/soper", NULL);
}

void _moddeinit()
{
	command_delete(&os_soper, os_cmdtree);
	command_delete(&os_soper_list, &os_soper_cmds);
	command_delete(&os_soper_listclass, &os_soper_cmds);
	command_delete(&os_soper_add, &os_soper_cmds);
	command_delete(&os_soper_del, &os_soper_cmds);

	help_delentry(os_helptree, "SOPER");
}

static void os_cmd_soper(sourceinfo_t *si, int parc, char *parv[])
{
	command_t *c;

	if (!has_any_privs(si))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER LIST|LISTCLASS|ADD|DEL [account] [operclass]"));
		return;
	}

	c = command_find(&os_soper_cmds, parv[0]);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == FALSE) ? "msg " : "", si->service->disp);
		return;
	}

	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void os_cmd_soper_list(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	soper_t *soper;
	const char *typestr;

	logcommand(si, CMDLOG_GET, "SOPER LIST");
	command_success_nodata(si, "%-20s %-5s %-20s", _("Account"), _("Type"), _("Operclass"));
	command_success_nodata(si, "%-20s %-5s %-20s", "--------------------", "-----", "--------------------");
	LIST_FOREACH(n, soperlist.head)
	{
		soper = n->data;
		if (!(soper->flags & SOPER_CONF))
			typestr = "DB";
		else if (soper->myuser != NULL)
			typestr = "Conf";
		else
			typestr = "Conf*";
		command_success_nodata(si, "%-20s %-5s %-20s",
				soper->myuser != NULL ? soper->myuser->name : soper->name,
				typestr,
				soper->classname);
	}
	command_success_nodata(si, "%-20s %-5s %-20s", "--------------------", "-----", "--------------------");
	command_success_nodata(si, _("End of services operator list"));
}

static void os_cmd_soper_listclass(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	operclass_t *operclass;

	logcommand(si, CMDLOG_GET, "SOPER LISTCLASS");
	command_success_nodata(si, _("Oper class list:"));
	LIST_FOREACH(n, operclasslist.head)
	{
		operclass = n->data;
		command_success_nodata(si, "%c%c %s",
				has_all_operclass(si, operclass) ? '+' : '-',
				operclass->flags & OPERCLASS_NEEDOPER ? '*' : ' ',
				operclass->name);
	}
	command_success_nodata(si, _("End of oper class list"));
}

static void os_cmd_soper_add(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	operclass_t *operclass;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER ADD");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER ADD <account> <operclass>"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
		return;
	}

	if (is_conf_soper(mu))
	{
		command_fail(si, fault_noprivs, _("You may not modify \2%s\2's operclass as it is defined in the configuration file."), mu->name);
		return;
	}

	operclass = operclass_find(parv[1]);
	if (operclass == NULL)
	{
		command_fail(si, fault_nosuch_target, _("No such oper class \2%s\2."), parv[1]);
		return;
	}
	else if (mu->soper != NULL && mu->soper->operclass == operclass)
	{
		command_fail(si, fault_nochange, _("Oper class for \2%s\2 is already set to \2%s\2."), mu->name, operclass->name);
		return;
	}

	if (!has_all_operclass(si, operclass))
	{
		command_fail(si, fault_noprivs, _("Oper class \2%s\2 has more privileges than you."), operclass->name);
		return;
	}
	else if (mu->soper != NULL && mu->soper->operclass != NULL && !has_all_operclass(si, mu->soper->operclass))
	{
		command_fail(si, fault_noprivs, _("Oper class for \2%s\2 is set to \2%s\2 which you are not authorized to change."),
				mu->name, mu->soper->operclass->name);
		return;
	}

	wallops("\2%s\2 is changing oper class for \2%s\2 to \2%s\2",
		get_oper_name(si), mu->name, operclass->name);
	snoop("SOPER:ADD: \2%s\2 to \2%s\2 by \2%s\2", mu->name, operclass->name, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "SOPER ADD %s %s", mu->name, operclass->name);
	if (is_soper(mu))
		soper_delete(mu->soper);
	soper_add(mu->name, operclass->name, 0);
	command_success_nodata(si, _("Set class for \2%s\2 to \2%s\2."), mu->name, operclass->name);
}

static void os_cmd_soper_del(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SOPER DEL");
		command_fail(si, fault_needmoreparams, _("Syntax: SOPER DEL <account>"));
		return;
	}

	mu = myuser_find_ext(parv[0]);
	if (mu == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
		return;
	}

	if (is_conf_soper(mu))
	{
		command_fail(si, fault_noprivs, _("You may not modify \2%s\2's operclass as it is defined in the configuration file."), mu->name);
		return;
	}

	if (!is_soper(mu))
	{
		command_fail(si, fault_nochange, _("\2%s\2 does not have an operclass set."), mu->name);
		return;
	}
	else if (mu->soper->operclass != NULL && !has_all_operclass(si, mu->soper->operclass))
	{
		command_fail(si, fault_noprivs, _("Oper class for \2%s\2 is set to \2%s\2 which you are not authorized to change."),
				mu->name, mu->soper->operclass->name);
		return;
	}

	wallops("\2%s\2 is removing oper class for \2%s\2",
		get_oper_name(si), mu->name);
	snoop("SOPER:DELETE: \2%s\2 by \2%s\2", mu->name, get_oper_name(si));
	logcommand(si, CMDLOG_ADMIN, "SOPER DEL %s", mu->name);
	soper_delete(mu->soper);
	command_success_nodata(si, _("Removed class for \2%s\2."), mu->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
