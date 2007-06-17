/*
 * Copyright (c) 2007 Jilles Tjoelker, et al
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Per-channel userinfo thingie
 *
 * $Id: cs_userinfo.c 8027 2007-04-02 10:47:18Z nenolod $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/userinfo", FALSE, _modinit, _moddeinit,
	"$Id: cs_userinfo.c 8027 2007-04-02 10:47:18Z nenolod $",
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *cs_cmdtree, *cs_helptree;

static void userinfo_check_join(void *vptr);
static void cs_cmd_userinfo(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_userinfo = { "USERINFO", N_("Sets a userinfo message."),
			AC_NONE, 3, cs_cmd_userinfo };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

	hook_add_event("channel_join");
	hook_add_hook("channel_join", userinfo_check_join);
	command_add(&cs_userinfo, cs_cmdtree);
	help_addentry(cs_helptree, "USERINFO", "help/cservice/userinfo", NULL);
}

void _moddeinit(void)
{
	hook_del_hook("channel_join", userinfo_check_join);
	command_delete(&cs_userinfo, cs_cmdtree);
	help_delentry(cs_helptree, "USERINFO");
}

/* USERINFO <channel> [user] [message] */
static void cs_cmd_userinfo(sourceinfo_t *si, int parc, char *parv[])
{
	node_t *n;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	metadata_t *md;
	unsigned int restrictflags;

	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "USERINFO");
		command_fail(si, fault_needmoreparams, _("Syntax: USERINFO <channel> [target] [info]"));
		return;
	}

	mc = mychan_find(parv[0]);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[0]);
		return;
	}

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(si, PRIV_CHAN_AUSPEX))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), mc->name);
		return;
	}

	restrictflags = chanacs_source_flags(mc, si);

	if (parc == 1)
	{
		if (!(restrictflags & CA_ACLVIEW))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
		command_success_nodata(si, _("Nickname            Info"));
		command_success_nodata(si, "------------------- ---------------");

		LIST_FOREACH(n, mc->chanacs.head)
		{
			ca = n->data;
			if (ca->myuser == NULL)
				continue;
			md = metadata_find(ca, METADATA_CHANACS, "userinfo");
			if (md == NULL)
				continue;
			command_success_nodata(si, "%-19s %s", ca->myuser->name, md->value);
		}

		command_success_nodata(si, "------------------- ---------------");
		command_success_nodata(si, _("End of \2%s\2 USERINFO listing."), mc->name);
		logcommand(si, CMDLOG_GET, "%s USERINFO", mc->name);
	}
	else
	{
		if (!(restrictflags & CA_FLAGS))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
		mu = myuser_find_ext(parv[1]);
		if (mu == NULL)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), parv[1]);
			return;
		}
		ca = chanacs_find(mc, mu, 0);
		if (ca == NULL || ca->level & CA_AKICK)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 has no access to \2%s\2."), mu->name, mc->name);
			return;
		}
		if (ca->level & ~allow_flags(restrictflags))
		{
			command_fail(si, fault_noprivs, _("You are not authorized to modify the access entry for \2%s\2 on \2%s\2."), mu->name, mc->name);
			return;
		}
		if (parc == 2)
		{
			metadata_delete(ca, METADATA_CHANACS, "userinfo");
			command_success_nodata(si, _("Deleted userinfo for \2%s\2 on \2%s\2."),
						mu->name, mc->name);
			logcommand(si, CMDLOG_SET, "%s USERINFO %s", mc->name, mu->name);
			return;
		}

		metadata_add(ca, METADATA_CHANACS, "userinfo", parv[2]);
		command_success_nodata(si, _("Added userinfo for \2%s\2 on \2%s\2."),
					mu->name, mc->name);
		logcommand(si, CMDLOG_SET, "%s USERINFO %s %s", mc->name, mu->name, parv[2]);
	}
	return;
}

static void userinfo_check_join(void *vptr)
{
	chanuser_t *cu = ((hook_channel_joinpart_t *)vptr)->cu;
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	metadata_t *md;

	if (cu == NULL)
		return;
	if (!(cu->user->server->flags & SF_EOB))
		return;
	mu = cu->user->myuser;
	mc = mychan_find(cu->chan->name);
	if (mu == NULL || mc == NULL)
		return;
	ca = chanacs_find(mc, mu, 0);
	if (ca == NULL || ca->level & CA_AKICK)
		return;
	if (!(md = metadata_find(ca, METADATA_CHANACS, "userinfo")))
		return;
	msg(chansvs.nick, cu->chan->name, "[%s] %s", cu->user->nick, md->value);
}
