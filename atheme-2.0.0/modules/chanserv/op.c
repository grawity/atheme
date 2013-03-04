/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 * $Id: op.c 7047 2006-11-02 23:54:45Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/op", FALSE, _modinit, _moddeinit,
	"$Id: op.c 7047 2006-11-02 23:54:45Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_op = { "OP", "Gives channel ops to a user.",
                        AC_NONE, 2, cs_cmd_op };
command_t cs_deop = { "DEOP", "Removes channel ops from a user.",
                        AC_NONE, 2, cs_cmd_deop };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_op, cs_cmdtree);
        command_add(&cs_deop, cs_cmdtree);

	help_addentry(cs_helptree, "OP", "help/cservice/op_voice", NULL);
	help_addentry(cs_helptree, "DEOP", "help/cservice/op_voice", NULL);
}

void _moddeinit()
{
	command_delete(&cs_op, cs_cmdtree);
	command_delete(&cs_deop, cs_cmdtree);

	help_delentry(cs_helptree, "OP");
	help_delentry(cs_helptree, "DEOP");
}

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "OP");
		command_fail(si, fault_needmoreparams, "Syntax: OP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_OP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to op */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	/* SECURE check; we can skip this if sender == target, because we already verified */
	if ((si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, CA_OP) && !chanacs_user_has_flag(mc, tu, CA_AUTOOP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.", mc->name);
		command_fail(si, fault_noprivs, "\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access.", mc->name, tu->nick);
		return;
	}

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_ADD, 'o', CLIENT_NAME(tu));
	cu->modes |= CMODE_OP;

	if (si->c == NULL && tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been opped on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_SET, "%s OP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been opped on \2%s\2.", tu->nick, mc->name);
}

static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;

	if (!chan)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "DEOP");
		command_fail(si, fault_needmoreparams, "Syntax: DEOP <#channel> [nickname]");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_OP))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	/* figure out who we're going to deop */
	if (!nick)
		tu = si->su;
	else
	{
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, "\2%s\2 is not online.", nick);
			return;
		}
	}

	if (is_internal_client(tu))
		return;

	cu = chanuser_find(mc->chan, tu);
	if (!cu)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not on \2%s\2.", tu->nick, mc->name);
		return;
	}

	modestack_mode_param(chansvs.nick, chan, MTYPE_DEL, 'o', CLIENT_NAME(tu));
	cu->modes &= ~CMODE_OP;

	if (si->c == NULL && tu != si->su)
		notice(chansvs.nick, tu->nick, "You have been deopped on %s by %s", mc->name, get_source_name(si));

	logcommand(si, CMDLOG_SET, "%s DEOP %s!%s@%s", mc->name, tu->nick, tu->user, tu->vhost);
	if (!chanuser_find(mc->chan, si->su))
		command_success_nodata(si, "\2%s\2 has been deopped on \2%s\2.", tu->nick, mc->name);
}

