/*
 * Copyright (c) 2006 Robin Burchell <surreal.w00t@gmail.com>
 * Rights to this code are as defined in doc/LICENSE.
 *
 * Regexp-based AKILL implementation.
 *
 * $Id: rakill.c 8027 2007-04-02 10:47:18Z nenolod $
 */

/*
 * @makill regex!here.+ [akill reason]
 *  Matches `nick!user@host realname here' for each client against a given regex, and places akills.
 *  CHECK REGEX USING @RMATCH FIRST!
 */
#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/rakill", FALSE, _modinit, _moddeinit,
	"$Id: rakill.c 8027 2007-04-02 10:47:18Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *os_cmdtree;
list_t *os_helptree;

static void os_cmd_rakill(sourceinfo_t *si, int parc, char *parv[]);

command_t os_rakill = { "RAKILL", N_("Sets a group of AKILLs against users matching a specific regex pattern."), PRIV_MASS_AKILL, 1, os_cmd_rakill };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_rakill, os_cmdtree);
	help_addentry(os_helptree, "RAKILL", "help/oservice/rakill", NULL);
}

void _moddeinit(void)
{
	command_delete(&os_rakill, os_cmdtree);
	help_delentry(os_helptree, "RAKILL");
}

static void os_cmd_rakill(sourceinfo_t *si, int parc, char *parv[])
{
	regex_t *regex;
	char usermask[512];
	unsigned int matches = 0;
	dictionary_iteration_state_t state;
	user_t *u;
	char *args = parv[0];
	char *pattern;
	char *reason;
	user_t *source;
	int flags = 0;

	/* make sure they could have done RMATCH */
	if (!has_priv(si, PRIV_USER_AUSPEX))
	{
		command_fail(si, fault_noprivs, _("You do not have %s privilege."), PRIV_USER_AUSPEX);
		return;
	}

	if (args == NULL)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RAKILL");
		command_fail(si, fault_needmoreparams, _("Syntax: RAKILL /<regex>/[i] <reason>"));
		return;
	}

	pattern = regex_extract(args, &args, &flags);
	if (pattern == NULL)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "RAKILL");
		command_fail(si, fault_badparams, _("Syntax: RAKILL /<regex>/[i] <reason>"));
		return;
	}

	reason = args;
	while (*reason == ' ')
		reason++;
	if (*reason == '\0')
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RAKILL");
		command_fail(si, fault_needmoreparams, _("Syntax: RAKILL /<regex>/[i] <reason>"));
		return;
	}

	regex = regex_create(pattern, flags);
	if (regex == NULL)
	{
		command_fail(si, fault_badparams, _("The provided regex \2%s\2 is invalid."), pattern);
		return;
	}

	source = si->su;
	/* try to find them on IRC, otherwise use operserv */
	if (source == NULL)
		source = si->smu != NULL && LIST_LENGTH(&si->smu->logins) > 0 ?
			si->smu->logins.head->data : si->service->me;
	sprintf(usermask, "%s!%s@%s %s", source->nick, source->user, source->host, source->gecos);
	if (regex_match(regex, (char *)usermask))
	{
		regex_destroy(regex);
		command_fail(si, fault_noprivs, _("The provided regex matches you, refusing RAKILL."));
		snoop("RAKILL:REFUSED: \2%s\2 by \2%s\2 (%s) (matches self)", pattern, get_oper_name(si), reason);
		wallops("\2%s\2 attempted to do RAKILL on \2%s\2 matching self",
				get_oper_name(si), pattern);
		logcommand(si, CMDLOG_ADMIN, "RAKILL %s %s (refused, matches self)", pattern, reason);
		return;
	}

	snoop("RAKILL: \2%s\2 by \2%s\2 (%s)", pattern, get_oper_name(si), reason);

	DICTIONARY_FOREACH(u, &state, userlist)
	{
		sprintf(usermask, "%s!%s@%s %s", u->nick, u->user, u->host, u->gecos);

		if (regex_match(regex, (char *)usermask))
		{
			/* match */
			command_success_nodata(si, _("\2Match:\2  %s!%s@%s %s - akilling"), u->nick, u->user, u->host, u->gecos);
			kline_sts("*", "*", u->host, 604800, reason);
			matches++;
		}
	}
	
	regex_destroy(regex);
	command_success_nodata(si, _("\2%d\2 matches for %s akilled."), matches, pattern);
	logcommand(si, CMDLOG_ADMIN, "RAKILL %s %s (%d matches)", pattern, reason, matches);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
