/*
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ RELEASE/ENFORCE functions.
 *
 * This does nickserv enforcement on registered nicks if the ENFORCE option
 * has been enabled. Users who do not identify within 30-60 seconds have
 * their nick changed to Guest<num>.
 * If the ircd or protocol module do not support forced nick changes,
 * they are killed instead.
 * Enforcement of the nick is only supported for ircds that support
 * holdnick_sts(), currently bahamut, charybdis, hybrid, inspircd11,
 * solidircd, ratbox and unreal (i.e. making sure they can't change back
 * immediately). Consequently this module is of little use for other ircds.
 * Note: For hybrid and ratbox, and charybdis before 2.1, the
 * RELEASE command to remove an enforcer prematurely is not supported,
 * although it pretends to be successful.
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/enforce",FALSE, _modinit, _moddeinit,
	"$Id: enforce.c 8233 2007-05-06 22:47:38Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

#define ENFORCE_TIMEOUT 30
#define ENFORCE_CHECK_FREQ 5

typedef struct {
	char nick[NICKLEN];
	char host[HOSTLEN];
	time_t timelimit;
	node_t node;
} enforce_timeout_t;

list_t enforce_list;
BlockHeap *enforce_timeout_heap;

static void guest_nickname(user_t *u);

static void ns_cmd_set_enforce(sourceinfo_t *si, int parc, char *parv[]);
static void ns_cmd_release(sourceinfo_t *si, int parc, char *parv[]);

static void enforce_timeout_check(void *arg);
static void show_enforce(void *vdata);
static void check_registration(void *vdata);
static void check_enforce(void *vdata);

command_t ns_set_enforce = { "ENFORCE", N_("Enables or disables automatic protection of a nickname."), AC_NONE, 1, ns_cmd_set_enforce }; 
command_t ns_release = { "RELEASE", N_("Releases a services enforcer."), AC_NONE, 2, ns_cmd_release };

list_t *ns_cmdtree, *ns_set_cmdtree, *ns_helptree;

/* sends an FNC for the given user */
static void guest_nickname(user_t *u)
{
	char gnick[NICKLEN];
	int tries;

	/* Generate a new guest nickname and check if it already exists
	 * This will try to generate a new nickname 30 different times
	 * if nicks are in use. If it runs into 30 nicks in use, maybe 
	 * you shouldn't use this module. */
	for (tries = 0; tries < 30; tries++)
	{
		snprintf(gnick, sizeof gnick, "Guest%d", arc4random()%100000);
		if (!user_find_named(gnick))
			break;
	}
	fnc_sts(nicksvs.me->me, u, gnick, FNC_FORCE);
}

static void ns_cmd_set_enforce(sourceinfo_t *si, int parc, char *parv[])
{
	metadata_t *md;
	char *setting = parv[0];

	if (!setting)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ENFORCE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET ENFORCE ON|OFF"));
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (strcasecmp(setting, "ON") == 0)
	{
		if ((md = metadata_find(si->smu, METADATA_USER, "private:doenforce")) != NULL)
		{
			command_fail(si, fault_nochange, _("ENFORCE is already enabled."));
		}
		else
		{
			metadata_add(si->smu, METADATA_USER, "private:doenforce", "1");
			command_success_nodata(si, _("ENFORCE is now enabled."));
		}
	}
	else if (strcasecmp(setting, "OFF") == 0)
	{
		if ((md = metadata_find(si->smu, METADATA_USER, "private:doenforce")) != NULL)
		{
			metadata_delete(si->smu, METADATA_USER, "private:doenforce");
			command_success_nodata(si, _("ENFORCE is now disabled."));
		}
		else
		{
			command_fail(si, fault_nochange, _("ENFORCE is already disabled."));
		}
	}
	else
	{
		command_fail(si, fault_badparams, _("Unknown value for ENFORCE. Expected values are ON or OFF."));
	}
}

static void ns_cmd_release(sourceinfo_t *si, int parc, char *parv[])
{
	mynick_t *mn;
	metadata_t *md;
	char *target = parv[0];
	char *password = parv[1];
	user_t *u;
	node_t *n, *tn;
	enforce_timeout_t *timeout;

	/* Absolutely do not do anything like this if nicks
	 * are not considered owned */
	if (nicksvs.no_nick_ownership)
	{
		command_fail(si, fault_noprivs, _("RELEASE is disabled."));
		return;
	}

	if (!target && si->smu != NULL)
		target = si->smu->name;
	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RELEASE");
		command_fail(si, fault_needmoreparams, _("Syntax: RELEASE <nick> [password]"));
		return;
	}

	u = user_find_named(target);
	mn = mynick_find(target);
	
	if (!mn)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered nickname."), target);
		return;
	}
	
	if (u == si->su)
	{
		command_fail(si, fault_noprivs, _("You cannot RELEASE yourself."));
		return;
	}
	if ((si->smu == mn->owner) || verify_password(mn->owner, password))
	{
		/* if this (nick, host) is waiting to be enforced, remove it */
		LIST_FOREACH_SAFE(n, tn, enforce_list.head)
		{
			timeout = n->data;
			if (!irccasecmp(mn->nick, timeout->nick) && (!strcmp(u->host, timeout->host) || !strcmp(u->vhost, timeout->host)))
			{
				node_del(&timeout->node, &enforce_list);
				BlockHeapFree(enforce_timeout_heap, timeout);
			}
		}
		if (u == NULL || is_internal_client(u))
		{
			logcommand(si, CMDLOG_DO, "RELEASE %s", target);
			holdnick_sts(si->service->me, 0, target, mn->owner);
			command_success_nodata(si, _("\2%s\2 has been released."), target);
		}
		else
		{
			notice(nicksvs.nick, target, "%s has released your nickname.", get_source_mask(si));
			guest_nickname(u);
			command_success_nodata(si, _("%s has been released."), target);
			logcommand(si, CMDLOG_DO, "RELEASE %s!%s@%s", u->nick, u->user, u->vhost);
		}
		return;
	}
	if (!password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "RELEASE");
		command_fail(si, fault_needmoreparams, _("Syntax: RELEASE <nickname> [password]"));
		return;
	}
	else
	{
		logcommand(si, CMDLOG_DO, "failed RELEASE %s (bad password)", target);
		command_fail(si, fault_authfail, _("Invalid password for \2%s\2."), target);
	}
}

void enforce_timeout_check(void *arg)
{
	node_t *n, *tn;
	enforce_timeout_t *timeout;
	user_t *u;
	mynick_t *mn;
	boolean_t valid;

	LIST_FOREACH_SAFE(n, tn, enforce_list.head)
	{
		timeout = n->data;
		if (timeout->timelimit > CURRTIME)
			break; /* assume sorted list */
		u = user_find_named(timeout->nick);
		mn = mynick_find(timeout->nick);
		valid = u != NULL && mn != NULL && (!strcmp(u->host, timeout->host) || !strcmp(u->vhost, timeout->host));
		node_del(&timeout->node, &enforce_list);
		BlockHeapFree(enforce_timeout_heap, timeout);
		if (!valid)
			continue;
		if (is_internal_client(u))
			continue;
		if (u->myuser == mn->owner)
			continue;
		if (myuser_access_verify(u, mn->owner))
			continue;
		if (!metadata_find(mn->owner, METADATA_USER, "private:doenforce"))
			continue;

		notice(nicksvs.nick, u->nick, "You failed to identify in time for the nickname %s", mn->nick);
		guest_nickname(u);
		holdnick_sts(nicksvs.me->me, 3600, u->nick, mn->owner);
	}
}

static void show_enforce(void *vdata)
{
	hook_user_req_t *hdata = vdata;

	if (metadata_find(hdata->mu, METADATA_USER, "private:doenforce"))
		command_success_nodata(hdata->si, "%s has enabled nick protection", hdata->mu->name);
}

static void check_registration(void *vdata)
{
	hook_user_register_check_t *hdata = vdata;

	if (hdata->approved)
		return;
	if (!strncasecmp(hdata->account, "Guest", 5) && isdigit(hdata->account[5]))
	{
		command_fail(hdata->si, fault_badparams, "The nick \2%s\2 is reserved and cannot be registered.", hdata->account);
		hdata->approved = 1;
	}
}

static void check_enforce(void *vdata)
{
	hook_nick_enforce_t *hdata = vdata;
	enforce_timeout_t *timeout, *timeout2;
	node_t *n;

	/* nick is a service, ignore it */
	if (is_internal_client(hdata->u))
		return;

	if (!metadata_find(hdata->mn->owner, METADATA_USER, "private:doenforce"))
		return;

	/* check if it's already in enforce_list */
	timeout = NULL;
#ifdef SHOW_CORRECT_TIMEOUT_BUT_BE_SLOW
	/* don't do this now, it's O(N^2) in the number of users using
	 * a nick without access at a time */
	LIST_FOREACH(n, enforce_list.head)
	{
		timeout2 = n->data;
		if (!irccasecmp(hdata->mn->nick, timeout2->nick) && (!strcmp(hdata->u->host, timeout2->host) || !strcmp(hdata->u->vhost, timeout2->host)))
		{
			timeout = timeout2;
			break;
		}
	}
#endif

	if (timeout == NULL)
	{
		timeout = BlockHeapAlloc(enforce_timeout_heap);
		strlcpy(timeout->nick, hdata->mn->nick, sizeof timeout->nick);
		strlcpy(timeout->host, hdata->u->host, sizeof timeout->host);
		/* the following ENFORCE_TIMEOUT must be constant,
		 * otherwise the timeouts will not be sorted and
		 * enforce_timeout_check() will break */
		timeout->timelimit = CURRTIME + ENFORCE_TIMEOUT;
		node_add(timeout, &timeout->node, &enforce_list);
	}

	notice(nicksvs.nick, hdata->u->nick, "You have %d seconds to identify to your nickname before it is changed.", timeout->timelimit - CURRTIME);
}

static int idcheck_foreach_cb(dictionary_elem_t *delem, void *privdata)
{
	metadata_t *md;
	myuser_t *mu = (myuser_t *) delem->node.data;

	if ((md = metadata_find(mu, METADATA_USER, "private:idcheck")))
		metadata_delete(mu, METADATA_USER, "private:idcheck");
	if ((md = metadata_find(mu, METADATA_USER, "private:enforcer")))
		metadata_delete(mu, METADATA_USER, "private:enforcer");

	return 0;
}

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");
	MODULE_USE_SYMBOL(ns_set_cmdtree, "nickserv/set", "ns_set_cmdtree");

	/* Leave this for compatibility with old versions of this code
	 * -- jilles
	 */
	dictionary_foreach(mulist, idcheck_foreach_cb, NULL);

	/* Absolutely do not do anything like this if nicks
	 * are not considered owned */
	if (nicksvs.no_nick_ownership)
	{
		slog(LG_ERROR, "modules/nickserv/enforce: nicks are not configured to be owned");
		m->mflags = MODTYPE_FAIL;
		return;
	}
	
	enforce_timeout_heap = BlockHeapCreate(sizeof(enforce_timeout_t), 128);
	if (enforce_timeout_heap == NULL)
	{
		m->mflags = MODTYPE_FAIL;
		return;
	}

	event_add("enforce_timeout_check", enforce_timeout_check, NULL, ENFORCE_CHECK_FREQ);
	/*event_add("manage_bots", manage_bots, NULL, 30);*/
	command_add(&ns_release, ns_cmdtree);
	command_add(&ns_set_enforce, ns_set_cmdtree);
	help_addentry(ns_helptree, "RELEASE", "help/nickserv/release", NULL);
	help_addentry(ns_helptree, "SET ENFORCE", "help/nickserv/set_enforce", NULL);
	hook_add_event("user_info");
	hook_add_hook("user_info", show_enforce);
	hook_add_event("user_can_register");
	hook_add_hook("user_can_register", check_registration);
	hook_add_event("nick_enforce");
	hook_add_hook("nick_enforce", check_enforce);
}

void _moddeinit()
{
	event_delete(enforce_timeout_check, NULL);
	command_delete(&ns_release, ns_cmdtree);
	command_delete(&ns_set_enforce, ns_set_cmdtree);
	help_delentry(ns_helptree, "RELEASE");
	help_delentry(ns_helptree, "SET ENFORCE");
	hook_del_hook("user_info", show_enforce);
	hook_del_hook("user_can_register", check_registration);
	hook_del_hook("nick_enforce", check_enforce);
	BlockHeapDestroy(enforce_timeout_heap);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
