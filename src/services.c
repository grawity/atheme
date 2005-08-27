/*
 * Copyright (c) 2005 Atheme Development Group
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains client interaction routines.
 *
 * $Id: services.c 1682 2005-08-12 10:45:42Z pfish $
 */

#include "atheme.h"

extern list_t services[HASHSIZE];

/* ban wrapper for cmode */
void ban(char *sender, char *channel, user_t *user)
{
	char mask[128];
	channel_t *c = channel_find(channel);
	chanban_t *cb;

	if (!c)
		return;

	mask[0] = '\0';

	strlcat(mask, "*!*@", 128);
	strlcat(mask, user->vhost, 128);

	cb = chanban_find(c, mask);

	if (cb)
		return;

	chanban_add(c, mask);

	cmode(sender, channel, "+b", mask);
}

/* initialize core services */
void initialize_services(void)
{
	if (!chansvs.me || !globsvs.me || !opersvs.me)
	{
		chansvs.me = add_service(chansvs.nick, chansvs.user, chansvs.host, chansvs.real, cservice);
		globsvs.me = add_service(globsvs.nick, globsvs.user, globsvs.host, globsvs.real, gservice);
		opersvs.me = add_service(opersvs.nick, opersvs.user, opersvs.host, opersvs.real, oservice);

		chansvs.disp = chansvs.me->disp;
		globsvs.disp = globsvs.me->disp;
		opersvs.disp = opersvs.me->disp;

		if (nicksvs.enable == TRUE)
		{
			nicksvs.me = add_service(nicksvs.nick, nicksvs.user, nicksvs.host, nicksvs.real, nickserv);
			nicksvs.disp = nicksvs.me->disp;
		}
	}
}

/* bring on the services clients */
void services_init(void)
{
	node_t *n;
	uint32_t i;
	service_t *svs;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, services[i].head)
		{
			svs = n->data;

			svs->me = introduce_nick(svs->name, svs->user, svs->host, svs->real, "io");
		}
	}
}

void verbose(mychan_t *mychan, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	if (MC_VERBOSE & mychan->flags)
		notice(chansvs.nick, mychan->name, buf);
}

void snoop(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (!config_options.chan)
		return;

	if (me.bursting)
		return;

	if (!channel_find(config_options.chan))
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	msg(config_options.chan, "%s", buf);
}

/* protocol wrapper for nickchange/nick burst */
void handle_nickchange(user_t *u)
{
	myuser_t *mu;

	if (me.loglevel & LG_DEBUG && runflags & RF_LIVE)
		notice(globsvs.nick, u->nick, "Services are presently running in debug mode, attached to a console. "
			"You should take extra caution when utilizing your services passwords.");

	if (nicksvs.enable != TRUE)
		return;

	if (!(mu = myuser_find(u->nick)))
	{
		if (!nicksvs.spam)
			return;

		if (!(u->flags & UF_SEENINFO))
		{
			notice(nicksvs.nick, u->nick, "Welcome to %s, %s! Here on %s, we provide services to enable the "
				"registration of nicknames and channels! For details, type \2/%s%s help\2 and \2/%s%s help\2.",
				me.netname, u->nick, me.netname, (ircd->uses_rcommand == FALSE) ? "msg " : "",
				nicksvs.disp, (ircd->uses_rcommand == FALSE) ? "msg " : "", chansvs.disp);

			u->flags |= UF_SEENINFO;
		}

		return;
	}

	if (u->myuser == mu && u->myuser->identified)
		return;

	notice(nicksvs.nick, u->nick, "This nickname is registered. Please choose a different nickname, or identify via \2/%s%s identify <password>\2.",
			(ircd->uses_rcommand == FALSE) ? "msg " : "", nicksvs.disp);
}

struct command_ *cmd_find(char *svs, char *origin, char *command, struct command_ table[])
{
	user_t *u = user_find(origin);
	struct command_ *c;

	for (c = table; c->name; c++)
	{
		if (!strcasecmp(command, c->name))
		{
			/* no special access required, so go ahead... */
			if (c->access == AC_NONE)
				return c;

			/* sra? */
			if ((c->access == AC_SRA) && (is_sra(u->myuser)))
				return c;

			/* ircop? */
			if ((c->access == AC_IRCOP) && (is_sra(u->myuser) || (is_ircop(u))))
				return c;

			/* otherwise... */
			else
			{
				notice(svs, origin, "You are not authorized to perform this operation.");
				snoop("DENIED CMD: \2%s\2 used %s", origin, command);
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(svs, origin, "Invalid command. Please use \2HELP\2 for help.");
	return NULL;
}
