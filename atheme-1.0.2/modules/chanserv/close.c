/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Closing for channels.
 *
 * $Id: close.c 4639 2006-01-21 22:06:41Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/close", FALSE, _modinit, _moddeinit,
	"$Id: close.c 4639 2006-01-21 22:06:41Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_close(char *origin);

/* CLOSE ON|OFF -- don't pollute the root with REOPEN */
command_t cs_close = { "CLOSE", "Closes a channel.",
			PRIV_CHAN_ADMIN, cs_cmd_close };

static void close_check_join(void *vcu);

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	cs_cmdtree = module_locate_symbol("chanserv/main", "cs_cmdtree");
	cs_helptree = module_locate_symbol("chanserv/main", "cs_helptree");

	command_add(&cs_close, cs_cmdtree);
	hook_add_event("channel_join");
	hook_add_hook("channel_join", close_check_join);
	help_addentry(cs_helptree, "CLOSE", "help/cservice/close", NULL);
}

void _moddeinit()
{
	command_delete(&cs_close, cs_cmdtree);
	hook_del_hook("channel_join", close_check_join);
	help_delentry(cs_helptree, "CLOSE");
}

static void close_check_join(void *vcu)
{
	mychan_t *mc;
	chanuser_t *cu;
	node_t *n;

	cu = vcu;

	if (is_internal_client(cu->user))
		return;

	if (!(mc = mychan_find(cu->chan->name)))
		return;

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
	        char buf[BUFSIZE];

		/* don't join if we're already in there */
		if (!chanuser_find(cu->chan, user_find_named(chansvs.nick)))
			join(cu->chan->name, chansvs.nick);

		/* stay for a bit to stop rejoin floods */
		mc->flags |= MC_INHABIT;

		/* lock it down */
		cmode(chansvs.nick, cu->chan->name, "+isbl", "*!*@*", "1");
		cu->chan->modes |= CMODE_INVITE;
		cu->chan->modes |= CMODE_SEC;
		cu->chan->modes |= CMODE_LIMIT;
		cu->chan->limit = 1;
		if (!chanban_find(cu->chan, "*!*@*", 'b'))
			chanban_add(cu->chan, "*!*@*", 'b');

		/* clear the channel */
		kick(chansvs.nick, cu->chan->name, cu->user->nick, "This channel has been closed");
	}
}

static void cs_cmd_close(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *reason = strtok(NULL, "");
	mychan_t *mc;
	channel_t *c;
	chanuser_t *cu;
	node_t *n, *tn;
	boolean_t found;

	if (!target || !action)
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLOSE");
		notice(chansvs.nick, origin, "Usage: CLOSE <#channel> <ON|OFF> [reason]");
		return;
	}

	if (!(mc = mychan_find(target)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!reason)
		{
			notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLOSE");
			notice(chansvs.nick, origin, "Usage: CLOSE <#channel> ON <reason>");
			return;
		}

		if (config_options.chan && !irccasecmp(config_options.chan, target))
		{
			notice(chansvs.nick, origin, "\2%s\2 cannot be closed.", target);
			return;
		}

		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is already closed.", target);
			return;
		}

		metadata_add(mc, METADATA_CHANNEL, "private:close:closer", origin);
		metadata_add(mc, METADATA_CHANNEL, "private:close:reason", reason);
		metadata_add(mc, METADATA_CHANNEL, "private:close:timestamp", itoa(CURRTIME));

		if ((c = channel_find(target)))
		{
		        char buf[BUFSIZE];

			if (!chanuser_find(c, user_find_named(chansvs.nick)))
				join(target, chansvs.nick);

			/* stay for a bit to stop rejoin floods */
			mc->flags |= MC_INHABIT;

			/* lock it down */
			cmode(chansvs.nick, target, "+isbl", "*!*@*", "1");
			c->modes |= CMODE_INVITE;
			c->modes |= CMODE_SEC;
			c->modes |= CMODE_LIMIT;
			c->limit = 1;
			if (!chanban_find(c, "*!*@*", 'b'))
				chanban_add(c, "*!*@*", 'b');

			/* clear the channel */
			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;

				if (!is_internal_client(cu->user))
					kick(chansvs.nick, target, cu->user->nick, "This channel has been closed");
			}
		}

		wallops("%s closed the channel \2%s\2 (%s).", origin, target, reason);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s CLOSE ON %s", target, reason);
		notice(chansvs.nick, origin, "\2%s\2 is now closed.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not closed.", target);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, "private:close:closer");
		metadata_delete(mc, METADATA_CHANNEL, "private:close:reason");
		metadata_delete(mc, METADATA_CHANNEL, "private:close:timestamp");
		mc->flags &= ~MC_INHABIT;
		c = channel_find(target);
		if (c != NULL)
			if (chanuser_find(c, user_find_named(chansvs.nick)))
				part(c->name, chansvs.nick);
		c = channel_find(target);
		if (c != NULL)
		{
			chanban_t *cb;

			/* hmm, channel still exists, probably permanent? */
			cb = chanban_find(c, "*!*@*", 'b');
			if (cb != NULL)
			{
				chanban_delete(cb);
				cmode(chansvs.nick, target, "-b", "*!*@*");
			}
			cmode(chansvs.nick, target, "-isl");
			c->modes &= ~(CMODE_INVITE | CMODE_SEC | CMODE_LIMIT);
			c->limit = 0;
			check_modes(mc, TRUE);
		}

		wallops("%s reopened the channel \2%s\2.", origin, target);
		logcommand(chansvs.me, user_find_named(origin), CMDLOG_ADMIN, "%s CLOSE OFF", target);
		notice(chansvs.nick, origin, "\2%s\2 has been reopened.", target);
	}
	else
	{
		notice(chansvs.nick, origin, STR_INSUFFICIENT_PARAMS, "CLOSE");
		notice(chansvs.nick, origin, "Usage: CLOSE <#channel> <ON|OFF> [reason]");
	}
}
