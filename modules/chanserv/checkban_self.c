/*
 * Copyright (c) 2005-2007 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a CService CHECKBAN which shows bans matching the source user.
 *
 */

#include "atheme.h"
#include "channels.h"

DECLARE_MODULE_V1
(
	"chanserv/checkban_self", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_checkban(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_checkban = { "CHECKBAN", N_("Finds bans and quiets matching you on a channel."),
			AC_NONE, 2, cs_cmd_checkban, { .path = "cservice/checkban_self" } };

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_checkban);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_checkban);
}

static void cs_cmd_checkban(sourceinfo_t *si, int parc, char *parv[])
{
        const char *channel = parv[0];
        const char *target = parv[1];
        channel_t *c = channel_find(channel);
	user_t *tu;
	chanban_t *cb;

	if (si->su == NULL)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 can only be executed via IRC."), "CHECKBAN");
		return;
	}

	if (!channel)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "CHECKBAN");
		command_fail(si, fault_needmoreparams, _("Syntax: CHECKBAN <#channel>"));
		return;
	}

	if (target && irccasecmp(target, si->su->nick))
	{
		command_fail(si, fault_noprivs, _("You may only check your own bans via %s."), si->service->nick);
		command_fail(si, fault_noprivs, _("Try \2/mode %s +b\2 to see all bans."), channel);
		return;
	}
	target = si->su->nick;

	if (!c)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), channel);
		return;
	}

	tu = si->su;
	{
		mowgli_node_t *n, *tn;
		char hostbuf2[BUFSIZE];
		int bcount = 0, qcount = 0;

		snprintf(hostbuf2, BUFSIZE, "%s!%s@%s", tu->nick, tu->user, tu->vhost);
#if 0
		if (!si->smu && c->modes && CMODE_REGONLY)
		{
			command_success_nodata(si, _("Found ban: Channel requires registration."));
			bcount++;
		}
#endif
		for (n = next_matching_ban(c, tu, 'b', c->bans.head); n != NULL; n = next_matching_ban(c, tu, 'b', tn))
		{
			tn = n->next;
			cb = n->data;
			command_success_nodata(si, _("Found ban: \2%s\2"), cb->mask, hostbuf2);
			bcount++;
		}
#if 0
		if (c->modes && CMODE_MOD)
		{
			command_success_nodata(si, _("Found quiet: Channel is moderated."));
			bcount++;
		}
#endif
		for (n = next_matching_ban(c, tu, 'q', c->bans.head); n != NULL; n = next_matching_ban(c, tu, 'q', tn))
		{
			tn = n->next;
			cb = n->data;
			command_success_nodata(si, _("Found quiet: \2%s\2"), cb->mask, hostbuf2);
			qcount++;
		}
		if (bcount + qcount > 0)
			command_success_nodata(si, _("Found %d %s and %d %s matching \2%s\2 on \2%s\2."),
				bcount, (bcount != 1 ? "bans" : "ban"),
				qcount, (qcount != 1 ? "quiets" : "quiet"),
				target, channel);
		else
			command_success_nodata(si, _("No bans or quiets found matching \2%s\2 on \2%s\2."),
				target, channel);
		return;
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
