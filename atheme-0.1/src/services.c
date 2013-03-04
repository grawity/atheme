/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains client interaction routines.
 *
 * $Id: services.c 325 2005-06-04 20:58:06Z nenolod $
 */

#include "atheme.h"

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
	strlcat(mask, user->host, 128);

	cb = chanban_find(c, mask);

	if (cb)
		return;

	chanban_add(c, mask);

	cmode(sender, channel, "+b", mask);
}

/* bring on the services clients */
void services_init(void)
{
	chansvs.me = introduce_nick(chansvs.nick, chansvs.user, chansvs.host, chansvs.real, "io");
	globsvs.me = introduce_nick(globsvs.nick, globsvs.user, globsvs.host, globsvs.real, "io");
	opersvs.me = introduce_nick(opersvs.nick, opersvs.user, opersvs.host, opersvs.real, "io");

	if (nicksvs.enable == TRUE)
		nicksvs.me = introduce_nick(nicksvs.nick, nicksvs.user, nicksvs.host, nicksvs.real, "io");
}

void verbose(mychan_t *mychan, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);

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

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);

	msg(config_options.chan, "%s", buf);
}

/* protocol wrapper for nickchange/nick burst */
void handle_nickchange(user_t *u)
{
	myuser_t *mu;

	if (nicksvs.enable != TRUE)
		return;

	if (!(mu = myuser_find(u->nick)))
		return;

	if (u->myuser == mu && u->myuser->identified)
		return;

	notice(nicksvs.nick, u->nick, "This nickname is registered. Please choose a different nickname, or identify via \2/msg %s identify <password>\2.",
			nicksvs.nick);
}

void expire_check(void *arg)
{
	uint32_t i, j, w, tcnt;
	myuser_t *mu;
	mychan_t *mc, *tmc;
	node_t *n1, *n2, *tn, *n3;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n1, mulist[i].head)
		{
			mu = (myuser_t *)n1->data;

			if (MU_HOLD & mu->flags)
				continue;

			if (((CURRTIME - mu->lastlogin) >= me.expire) || ((mu->flags & MU_WAITAUTH) && (CURRTIME - mu->registered >= 86400)))
			{
				/* kill all their channels */
				for (j = 0; j < HASHSIZE; j++)
				{
					LIST_FOREACH(tn, mclist[j].head)
					{
						mc = (mychan_t *)tn->data;

						if (mc->founder == mu && mc->successor)
						{
							/* make sure they're within limits */
							for (w = 0, tcnt = 0; w < HASHSIZE; w++)
							{
								LIST_FOREACH(n3, mclist[i].head)
								{
									tmc = (mychan_t *)n3->data;

									if (is_founder(tmc, mc->successor))
										tcnt++;
								}
							}

							if ((tcnt >= me.maxchans) && (!is_sra(mc->successor)))
								continue;

							snoop("SUCCESSION: \2%s\2 -> \2%s\2 from \2%s\2", mc->successor->name, mc->name, mc->founder->name);

							chanacs_delete(mc, mc->successor, CA_SUCCESSOR);
							chanacs_add(mc, mc->successor, CA_FOUNDER);
							mc->founder = mc->successor;
							mc->successor = NULL;

							if (mc->founder->user)
								notice(chansvs.nick, mc->founder->user->nick, "You are now founder on \2%s\2.", mc->name);

							return;
						}
						else if (mc->founder == mu)
						{
							snoop("EXPIRE: \2%s\2 from \2%s\2", mc->name, mu->name);

							part(mc->name, chansvs.nick);
							mychan_delete(mc->name);
						}
					}
				}

				snoop("EXPIRE: \2%s\2 from \2%s\2 ", mu->name, mu->email);
				myuser_delete(mu->name);
			}
		}
	}

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n2, mclist[i].head)
		{
			mc = (mychan_t *)n2->data;

			if (MU_HOLD & mc->founder->flags)
				continue;

			if (MC_HOLD & mc->flags)
				continue;

			if ((CURRTIME - mc->used) >= me.expire)
			{
				snoop("EXPIRE: \2%s\2 from \2%s\2", mc->name, mc->founder->name);

				part(mc->name, chansvs.nick);
				mychan_delete(mc->name);
			}
		}
	}
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
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(svs, origin, "Invalid command. Please use \2HELP\2 for help.");
	return NULL;
}
