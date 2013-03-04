/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the ChanServ CLEAR USERS function.
 *
 * $Id: clear_users.c 3663 2005-11-08 02:10:26Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/clear_users", FALSE, _modinit, _moddeinit,
	"$Id: clear_users.c 3663 2005-11-08 02:10:26Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_clear_users(char *origin, char *channel);

fcommand_t cs_clear_users = { "USERS", AC_NONE, cs_cmd_clear_users };

list_t *cs_clear_cmds;

void _modinit(module_t *m)
{
	cs_clear_cmds = module_locate_symbol("chanserv/clear", "cs_clear_cmds");
	fcommand_add(&cs_clear_users, cs_clear_cmds);
}

void _moddeinit()
{
	fcommand_delete(&cs_clear_users, cs_clear_cmds);
}

static void cs_cmd_clear_users(char *origin, char *channel)
{
	char *reason = strtok(NULL, "");
	char fullreason[200];
	user_t *u = user_find(origin);
	channel_t *c;
	mychan_t *mc = mychan_find(channel);
	chanacs_t *ca;
	chanuser_t *cu;
	node_t *n, *tn;

	if (!(c = channel_find(channel)))
	{
		notice(chansvs.nick, origin, "\2%s\2 does not exist.", channel);
		return;
	}

	if (reason)
		snprintf(fullreason, sizeof fullreason, "CLEAR USERS used by %s: %s", origin, reason);
	else
		snprintf(fullreason, sizeof fullreason, "CLEAR USERS used by %s", origin);

	if (!mc)
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", c->name);
		return;
	}

	if (!chanacs_user_has_flag(mc, u, CA_RECOVER))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		notice(chansvs.nick, origin, "\2%s\2 is closed.", channel);
		return;
	}

	/* stop a race condition where users can rejoin */
	mode_sts(chansvs.nick, c->name, "+b *!*@*");

	LIST_FOREACH_SAFE(n, tn, c->members.head)
	{
		cu = n->data;

		/* don't kick the user who requested the masskick */

		if (!irccasecmp(cu->user->nick, origin) || is_internal_client(cu->user))
			continue;

		kick(chansvs.nick, c->name, cu->user->nick, fullreason);
		chanuser_delete(c, cu->user);
	}

	logcommand(chansvs.me, u, CMDLOG_DO, "%s CLEAR USERS", mc->name);

	notice(chansvs.nick, origin, "Cleared users from \2%s\2.", channel);

	/* the channel may be empty now, so our pointer may be bogus! */
	if ((c = channel_find(channel)))
		cmode(chansvs.nick, c->name, "-b", "*!*@*");
}
