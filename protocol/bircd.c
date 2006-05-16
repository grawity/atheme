/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for P10 ircd's.
 * Some sources used: Run's documentation, beware's description,
 * raw data sent by asuka.
 *
 * $Id: bircd.c 5131 2006-04-29 19:09:24Z jilles $
 */

#include "atheme.h"
#include "protocol/asuka.h"

DECLARE_MODULE_V1("protocol/asuka", TRUE, _modinit, NULL, "$Id: bircd.c 5131 2006-04-29 19:09:24Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Asuka = {
        "Asuka 1.2.1 and later",        /* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        TRUE,                           /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        FALSE,                          /* Whether or not we support channel owners. */
        FALSE,                          /* Whether or not we support channel protection. */
        FALSE,                          /* Whether or not we support halfops. */
	TRUE,				/* Whether or not we use P10 */
	TRUE,				/* Whether or not we use vhosts. */
	0,				/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_ASUKA,			/* Protocol type */
	0,                              /* Permanent cmodes */
	"b",                            /* Ban-like cmodes */
	0,                              /* Except mchar */
	0                               /* Invex mchar */
};

struct cmode_ asuka_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR },
  { 'C', CMODE_NOCTCP },
  { 'D', CMODE_DELAYED },
  { 'u', CMODE_NOQUIT },
  { '\0', 0 }
};

struct cmode_ asuka_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ asuka_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ asuka_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

static void check_hidehost(user_t *u);

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t asuka_server_login(void)
{
	int8_t ret;

	me.recvsvr = FALSE;
	ret = sts("PASS :%s", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	/* SERVER irc.undernet.org 1 933022556 947908144 J10 AA]]] :[127.0.0.1] A Undernet Server */
	sts("SERVER %s 1 %ld %ld J10 %s]]] +s :%s", me.name, me.start, CURRTIME, me.numeric, me.desc);

	services_init();

	sts("%s EB", me.numeric);

	return 0;
}

/* introduce a client */
static void asuka_introduce_nick(char *nick, char *user, char *host, char *real, char *uid)
{
	sts("%s N %s 1 %ld %s %s +%s%sk ]]]]]] %s :%s", me.numeric, nick, CURRTIME, user, host, "io", chansvs.fantasy ? "" : "d", uid, real);
}

/* invite a user to a channel */
static void asuka_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	/* target is a nick, weird eh? -- jilles */
	sts("%s I %s %s", sender->uid, target->nick, channel->name);
}

static void asuka_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts("%s Q :%s", u->uid, reason);
}

/* WALLOPS wrapper */
static void asuka_wallops(char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	if (config_options.silent)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s WA :%s", me.numeric, buf);
}

/* join a channel */
static void asuka_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	/* If the channel doesn't exist, we need to create it. */
	if (isnew)
	{
		sts("%s C %s %ld", u->uid, c->name, c->ts);
		if (modes[0] && modes[1])
			sts("%s M %s %s %ld", u->uid, c->name, modes, c->ts);
	}
	else
	{
		sts("%s J %s %ld", u->uid, c->name, c->ts);
		sts("%s M %s +o %s %ld", me.numeric, c->name, u->uid, c->ts);
	}
}

/* kicks a user from a channel */
static void asuka_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *fptr = user_find_named(from);
	user_t *user = user_find_named(to);

	if (!chan || !user || !fptr)
		return;

	sts("%s K %s %s :%s", fptr->uid, channel, user->uid, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void asuka_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	user_t *u = user_find_named(from);
	char buf[BUFSIZE];

	if (!u)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s P %s :%s", u->uid, target, buf);
}

/* NOTICE wrapper */
static void asuka_notice(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *u = user_find_named(from);
	user_t *t = user_find_named(target);

	if (u == NULL && (from == NULL || (irccasecmp(from, me.name) && irccasecmp(from, ME))))
	{
		slog(LG_DEBUG, "asuka_notice(): unknown source %s for notice to %s", from, target);
		return;
	}

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s O %s :%s", u ? u->uid : me.numeric, t ? t->uid : target, buf);
}

static void asuka_wallchops(user_t *sender, channel_t *channel, char *message)
{
	sts("%s WC %s :%s", sender->uid, channel->name, message);
}

static void asuka_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *source_p, *target_p;

	source_p = user_find_named(from);
	target_p = user_find_named(target);

	if (!target_p)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s %d %s %s", source_p ? source_p->uid : me.numeric, numeric, target_p->uid, buf);
}

/* KILL wrapper */
static void asuka_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];
	user_t *fptr = user_find_named(from);
	user_t *tptr = user_find_named(nick);

	if (!tptr)
		return;

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts("%s D %s :%s!%s!%s (%s)", fptr ? fptr->uid : me.numeric, tptr->uid, from, from, from, buf);
}

/* PART wrapper */
static void asuka_part(char *chan, char *nick)
{
	user_t *u = user_find_named(nick);
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!u || !c)
		return;

	if (!(cu = chanuser_find(c, u)))
		return;

	sts("%s L %s", u->uid, c->name);

	chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
static void asuka_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts("%s GL * +%s@%s %ld :%s", me.numeric, user, host, duration, reason);
}

/* server-to-server UNKLINE wrapper */
static void asuka_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts("%s GL * -%s@%s", me.numeric, user, host);
}

/* topic wrapper */
static void asuka_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	channel_t *c;

	c = channel_find(channel);
	if (c == NULL)
		return;
	sts("%s T %s %ld %ld :%s", chansvs.me->me->uid, channel, c->ts, ts, topic);
}

/* mode wrapper */
static void asuka_mode_sts(char *sender, char *target, char *modes)
{
	user_t *fptr = user_find_named(sender);
	channel_t *cptr = channel_find(target);

	if (!fptr || !cptr)
		return;

	if (chanuser_find(cptr, fptr))
		sts("%s M %s %s %ld", fptr->uid, target, modes, cptr->ts);
	else
		sts("%s M %s %s %ld", me.numeric, target, modes, cptr->ts);
}

/* ping wrapper */
static void asuka_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("%s G !%ld %s %ld", me.numeric, CURRTIME, me.name, CURRTIME);
}

/* protocol-specific stuff to do on login */
static void asuka_on_login(char *origin, char *user, char *wantedhost)
{
	user_t *u = user_find_named(origin);

	if (!u)
		return;

	sts("%s AC %s %s", me.numeric, u->uid, u->myuser->name);
	check_hidehost(u);
}

/* P10 does not support logout, so kill the user
 * we can't keep track of which logins are stale and which aren't -- jilles */
static boolean_t asuka_on_logout(char *origin, char *user, char *wantedhost)
{
	user_t *u = user_find_named(origin);

	if (!me.connected)
		return FALSE;

	if (u != NULL)
	{
		skill(me.name, u->nick, "Forcing logout %s -> %s", u->nick, user);
		user_delete(u);
		return TRUE;
	}
	else
		return FALSE;
}

static void asuka_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts("%s JU * !+%s %ld :%s", me.numeric, server, CURRTIME, reason);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);
	server_t *source_server;
	user_t *source_user;
	char *source;
	time_t ts = 0;

	if (!c || parc < 2)
		return;

	if (origin == NULL)
		source = me.actual;
	else if ((source_server = server_find(origin)) != NULL)
		source = source_server->name;
	else if ((source_user = user_find(origin)) != NULL)
		source = source_user->nick;
	else
		source = origin;

	/* Let's accept any topic
	 * The criteria asuka uses for acceptance are broken,
	 * and it will not propagate anything not accepted
	 * -- jilles */
	if (parc > 2)
		ts = atoi(parv[parc - 2]);
	if (ts == 0)
		ts = CURRTIME;
	handle_topic(c, source, ts, parv[parc - 1]);
}

/* AB G !1119920789.573932 services.atheme.org 1119920789.573932 */
static void m_ping(char *origin, uint8_t parc, char *parv[])
{
	/* reply to PING's */
	sts("%s Z %s %s %s", me.numeric, parv[0], parv[1], parv[2]);
}

static void m_pong(char *origin, uint8_t parc, char *parv[])
{
	me.uplinkpong = CURRTIME;

	/* -> :test.projectxero.net PONG test.projectxero.net :shrike.malkier.net */
	if (me.bursting)
	{
#ifdef HAVE_GETTIMEOFDAY
		e_time(burstime, &burstime);

		slog(LG_INFO, "m_pong(): finished synching with uplink (%d %s)", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");

		wallops("Finished synching to network in %d %s.", (tv2ms(&burstime) > 1000) ? (tv2ms(&burstime) / 1000) : tv2ms(&burstime), (tv2ms(&burstime) > 1000) ? "s" : "ms");
#else
		slog(LG_INFO, "m_pong(): finished synching with uplink");
		wallops("Finished synching to network.");
#endif

		me.bursting = FALSE;
	}
}

static void m_privmsg(char *origin, uint8_t parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(origin, parv[0], FALSE, parv[1]);
}

static void m_notice(char *origin, uint8_t parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(origin, parv[0], TRUE, parv[1]);
}

static void m_create(char *origin, uint8_t parc, char *parv[])
{
	char buf[BUFSIZE];
	uint8_t chanc;
	char *chanv[256];
	uint8_t i;

	chanc = sjtoken(parv[0], ',', chanv);

	for (i = 0; i < chanc; i++)
	{
		channel_t *c = channel_add(chanv[i], atoi(parv[1]));

		buf[0] = '@';
		buf[1] = '\0';

		strlcat(buf, origin, BUFSIZE);

		chanuser_add(c, buf);
	}
}

static void m_join(char *origin, uint8_t parc, char *parv[])
{
	char buf[BUFSIZE];
	uint8_t chanc;
	char *chanv[256];
	uint8_t i;
	user_t *u;
	node_t *n, *tn;
	chanuser_t *cu;

	/* JOIN 0 is really a part from all channels */
	if (!strcmp(parv[0], "0"))
	{
		u = user_find(origin);
		if (u == NULL)
			return;
		LIST_FOREACH_SAFE(n, tn, u->channels.head)
		{
			cu = (chanuser_t *)n->data;
			chanuser_delete(cu->chan, u);
		}
		return;
	}
	if (parc < 2)
		return;

	chanc = sjtoken(parv[0], ',', chanv);

	for (i = 0; i < chanc; i++)
	{
		channel_t *c = channel_find(chanv[i]);

		if (!c)
			c = channel_add(chanv[i], atoi(parv[1]));

		chanuser_add(c, origin);
	}
}

static void m_burst(char *origin, uint8_t parc, char *parv[])
{
	channel_t *c;
	uint8_t modec;
	char *modev[16];
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	int j;
	char prefix[16];
	char newnick[16+NICKLEN];
	char *p;
	time_t ts;

	/* S BURST <channel> <ts> [parameters]
	 * parameters can be:
	 * +<simple mode>
	 * %<bans separated with spaces>
	 * <nicks>
	 */
	if (parc < 2)
	{
		slog(LG_DEBUG, "m_burst(): too few parameters");
		return;
	}

	ts = atoi(parv[1]);

	c = channel_find(parv[0]);

	if (c == NULL)
	{
		slog(LG_DEBUG, "m_burst(): new channel: %s", parv[0]);
		c = channel_add(parv[0], ts);
	}
	else if (ts < c->ts)
	{
		chanuser_t *cu;
		node_t *n;

		c->modes = 0;
		c->limit = 0;
		if (c->key)
			free(c->key);
		c->key = NULL;
		chanban_clear(c);
		handle_topic(c, "", 0, "");
		LIST_FOREACH(n, c->members.head)
		{
			cu = (chanuser_t *)n->data;
			if (cu->user->server == me.me)
			{
				/* it's a service, reop */
				sts("%s M %s +o %s %ld", me.numeric, c->name, CLIENT_NAME(cu->user), ts);
				cu->modes = CMODE_OP;
			}
			else
				cu->modes = 0;
		}

		slog(LG_INFO, "m_burst(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);
		c->ts = ts;
	}

	j = 2;
	while (j < parc)
	{
		if (parv[j][0] == '+')
		{
			modec = 0;
			modev[modec++] = parv[j++];
			if (strchr(modev[0], 'k') && j < parc)
				modev[modec++] = parv[j++];
			if (strchr(modev[0], 'l') && j < parc)
				modev[modec++] = parv[j++];
			channel_mode(NULL, c, modec, modev);
		}
		else if (parv[j][0] == '%')
		{
			userc = sjtoken(parv[j++] + 1, ' ', userv);
			for (i = 0; i < userc; i++)
				chanban_add(c, userv[i], 'b');
		}
		else
		{
			userc = sjtoken(parv[j++], ',', userv);

			prefix[0] = '\0';
			for (i = 0; i < userc; i++)
			{
				p = strchr(userv[i], ':');
				if (p != NULL)
				{
					*p = '\0';
					prefix[0] = '\0';
					prefix[1] = '\0';
					prefix[2] = '\0';
					p++;
					while (*p)
					{
						if (*p == 'o')
							prefix[prefix[0] ? 1 : 0] = '@';
						else if (*p == 'v')
							prefix[prefix[0] ? 1 : 0] = '+';
						p++;
					}
				}
				strlcpy(newnick, prefix, sizeof newnick);
				strlcat(newnick, userv[i], sizeof newnick);
				chanuser_add(c, newnick);
			}
		}
	}
}

static void m_part(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_part(): user left channel: %s -> %s", origin, parv[0]);

	chanuser_delete(channel_find(parv[0]), user_find(origin));
}

static void m_nick(char *origin, uint8_t parc, char *parv[])
{
	server_t *s;
	user_t *u;
	struct in_addr ip;
	char ipstring[64];

	/* got the right number of args for an introduction? */
	if (parc >= 8)
	{
		/* -> AB N jilles 1 1137687480 jilles jaguar.test +oiwgrx jilles B]AAAB ABAAE :Jilles Tjoelker */
		/* -> AB N test4 1 1137690148 jilles jaguar.test +iw B]AAAB ABAAG :Jilles Tjoelker */
		s = server_find(origin);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", origin);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		ipstring[0] = '\0';
		if (strlen(parv[parc - 3]) == 6)
		{
			ip.s_addr = ntohl(base64touint(parv[parc - 3]));
			if (!inet_ntop(AF_INET, &ip, ipstring, sizeof ipstring))
				ipstring[0] = '\0';
		}
		u = user_add(parv[0], parv[3], parv[4], NULL, ipstring, parv[parc - 2], parv[parc - 1], s, atoi(parv[2]));

		if (parv[5][0] == '+')
		{
			user_mode(u, parv[5]);
			if (strchr(parv[5], 'r'))
			{
				handle_burstlogin(u, parv[6]);
				/* killed to force logout? */
				if (user_find(parv[parc - 2]) == NULL)
					return;
			}
			if (strchr(parv[5], 'x'))
			{
				u->flags |= UF_HIDEHOSTREQ;
				/* this must be after setting the account name */
				check_hidehost(u);
			}
		}

		handle_nickchange(u);
	}
	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		node_t *n;

		u = user_find(origin);
		if (!u)
		{
			slog(LG_DEBUG, "m_nick(): nickname change from nonexistant user: %s", origin);
			return;
		}

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", u->nick, parv[0]);

		/* remove the current one from the list */
		n = node_find(u, &userlist[u->hash]);
		node_del(n, &userlist[u->hash]);
		node_free(n);

		/* change the nick */
		strlcpy(u->nick, parv[0], NICKLEN);
		u->ts = atoi(parv[1]);

		/* readd with new nick (so the hash works) */
		n = node_create();
		u->hash = UHASH((unsigned char *)u->nick);
		node_add(u, n, &userlist[u->hash]);

		handle_nickchange(u);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_nick(): got NICK with wrong (%d) number of params", parc);

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_nick():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_quit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", origin);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(user_find(origin));
}

static void m_mode(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;

	if (!origin)
	{
		slog(LG_DEBUG, "m_mode(): received MODE without origin");
		return;
	}

	if (parc < 2)
	{
		slog(LG_DEBUG, "m_mode(): missing parameters in MODE");
		return;
	}

	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
	else
	{
		/* Yes this is a nick and not a UID -- jilles */
		u = user_find_named(parv[0]);
		if (u == NULL)
		{
			slog(LG_DEBUG, "m_mode(): user mode for unknown user %s", parv[0]);
			return;
		}
		user_mode(u, parv[1]);
		if (strchr(parv[1], 'x'))
		{
			u->flags |= UF_HIDEHOSTREQ;
			check_hidehost(u);
		}
	}
}

static void m_clearmode(char *origin, uint8_t parc, char *parv[])
{
	channel_t *chan;
	char *p, c;
	node_t *n;
	chanuser_t *cu;
	int i;

	if (parc < 2)
	{
		slog(LG_DEBUG, "m_clearmode(): missing parameters in CLEARMODE");
		return;
	}

	/* -> ABAAA CM # b */
	/* Note: this is an IRCop command, do not enforce mode locks. */
	chan = channel_find(parv[0]);
	if (chan == NULL)
	{
		slog(LG_DEBUG, "m_clearmode(): unknown channel %s", parv[0]);
		return;
	}
	p = parv[1];
	while ((c = *p++))
	{
		if (c == 'b')
			chanban_clear(chan);
		else if (c == 'k')
		{
			if (chan->key)
				free(chan->key);
			chan->key = NULL;
		}
		else if (c == 'l')
			chan->limit = 0;
		else if (c == 'o')
		{
			LIST_FOREACH(n, chan->members.head)
			{
				cu = (chanuser_t *)n->data;
				if (cu->user->server == me.me)
				{
					/* it's a service, reop */
					sts("%s M %s +o %s %ld", me.numeric,
							chan->name,
							cu->user->uid,
							chan->ts);
				}
				else
					cu->modes &= ~CMODE_OP;
			}
		}
		else if (c == 'v')
		{
			LIST_FOREACH(n, chan->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes &= ~CMODE_VOICE;
			}
		}
		else
			for (i = 0; mode_list[i].mode != '\0'; i++)
			{
				if (c == mode_list[i].mode)
					chan->modes &= ~mode_list[i].value;
			}
	}
}

static void m_kick(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(parv[1]);
	channel_t *c = channel_find(parv[0]);

	/* -> :rakaur KICK #shrike rintaun :test */
	slog(LG_DEBUG, "m_kick(): user was kicked: %s -> %s", parv[1], parv[0]);

	if (!u)
	{
		slog(LG_DEBUG, "m_kick(): got kick for nonexistant user %s", parv[1]);
		return;
	}

	if (!c)
	{
		slog(LG_DEBUG, "m_kick(): got kick in nonexistant channel: %s", parv[0]);
		return;
	}

	if (!chanuser_find(c, u))
	{
		slog(LG_DEBUG, "m_kick(): got kick for %s not in %s", u->nick, c->name);
		return;
	}

	chanuser_delete(c, u);

	/* if they kicked us, let's rejoin */
	if (is_internal_client(u))
	{
		slog(LG_DEBUG, "m_kick(): %s got kicked from %s; rejoining", u->nick, parv[0]);
		join(parv[0], u->nick);
	}
}

static void m_kill(char *origin, uint8_t parc, char *parv[])
{
	if (parc < 1)
		return;
	handle_kill(origin, parv[0], parc > 1 ? parv[1] : "<No reason given>");
}

static void m_squit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

/* SERVER ircu.devel.atheme.org 1 1119902586 1119908830 J10 ABAP] + :lets lol */
static void m_server(char *origin, uint8_t parc, char *parv[])
{
	/* We dont care about the max connections. */
	parv[5][2] = '\0';

	slog(LG_DEBUG, "m_server(): new server: %s, id %s", parv[0], parv[5]);
	server_add(parv[0], atoi(parv[1]), origin ? origin : me.name, parv[5], parv[7]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);

	me.recvsvr = TRUE;
}

static void m_stats(char *origin, uint8_t parc, char *parv[])
{
	handle_stats(user_find(origin), parv[0][0]);
}

static void m_admin(char *origin, uint8_t parc, char *parv[])
{
	handle_admin(user_find(origin));
}

static void m_version(char *origin, uint8_t parc, char *parv[])
{
	handle_version(user_find(origin));
}

static void m_info(char *origin, uint8_t parc, char *parv[])
{
	handle_info(user_find(origin));
}

static void m_motd(char *origin, uint8_t parc, char *parv[])
{
	handle_motd(user_find(origin));
}

static void m_whois(char *origin, uint8_t parc, char *parv[])
{
	handle_whois(user_find(origin), parc >= 2 ? parv[1] : "*");
}

static void m_trace(char *origin, uint8_t parc, char *parv[])
{
	handle_trace(user_find(origin), parc >= 1 ? parv[0] : "*", parc >= 2 ? parv[1] : NULL);
}

static void m_pass(char *origin, uint8_t parc, char *parv[])
{
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void m_error(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_eos(char *origin, uint8_t parc, char *parv[])
{
	server_t *source = server_find(origin);

	/* acknowledge a local END_OF_BURST */
	if (source->uplink == me.me)
		sts("%s EA", me.numeric);
}

static void check_hidehost(user_t *u)
{
	static boolean_t warned = FALSE;

	/* do they qualify? */
	if (!(u->flags & UF_HIDEHOSTREQ) || u->myuser == NULL)
		return;
	/* don't use this if they have some other kind of vhost */
	if (strcmp(u->host, u->vhost))
	{
		slog(LG_DEBUG, "check_hidehost(): +x overruled by other vhost for %s", u->nick);
		return;
	}
	if (me.hidehostsuffix == NULL)
	{
		if (!warned)
		{
			wallops("Misconfiguration: serverinfo::hidehostsuffix not set");
			warned = TRUE;
		}
		return;
	}
	snprintf(u->vhost, sizeof u->vhost, "%s.%s", u->myuser->name,
			me.hidehostsuffix);
	slog(LG_DEBUG, "check_hidehost(): %s -> %s", u->nick, u->vhost);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &asuka_server_login;
	introduce_nick = &asuka_introduce_nick;
	quit_sts = &asuka_quit_sts;
	wallops = &asuka_wallops;
	join_sts = &asuka_join_sts;
	kick = &asuka_kick;
	msg = &asuka_msg;
	notice_sts = &asuka_notice;
	wallchops = &asuka_wallchops;
	numeric_sts = &asuka_numeric_sts;
	skill = &asuka_skill;
	part = &asuka_part;
	kline_sts = &asuka_kline_sts;
	unkline_sts = &asuka_unkline_sts;
	topic_sts = &asuka_topic_sts;
	mode_sts = &asuka_mode_sts;
	ping_sts = &asuka_ping_sts;
	ircd_on_login = &asuka_on_login;
	ircd_on_logout = &asuka_on_logout;
	jupe = &asuka_jupe;
	invite_sts = &asuka_invite_sts;

	parse = &p10_parse;

	mode_list = asuka_mode_list;
	ignore_mode_list = asuka_ignore_mode_list;
	status_mode_list = asuka_status_mode_list;
	prefix_mode_list = asuka_prefix_mode_list;

	ircd = &Asuka;

	pcommand_add("G", m_ping);
	pcommand_add("Z", m_pong);
	pcommand_add("P", m_privmsg);
	pcommand_add("O", m_notice);
	pcommand_add("C", m_create);
	pcommand_add("J", m_join);
	pcommand_add("EB", m_eos);
	pcommand_add("B", m_burst);
	pcommand_add("L", m_part);
	pcommand_add("N", m_nick);
	pcommand_add("Q", m_quit);
	pcommand_add("M", m_mode);
	pcommand_add("OM", m_mode); /* OPMODE, treat as MODE */
	pcommand_add("CM", m_clearmode);
	pcommand_add("K", m_kick);
	pcommand_add("D", m_kill);
	pcommand_add("SQ", m_squit);
	pcommand_add("S", m_server);
	pcommand_add("SERVER", m_server);
	pcommand_add("R", m_stats);
	pcommand_add("AD", m_admin);
	pcommand_add("V", m_version);
	pcommand_add("F", m_info);
	pcommand_add("W", m_whois);
	pcommand_add("TR", m_trace);
	pcommand_add("PASS", m_pass);
	pcommand_add("ERROR", m_error);
	pcommand_add("T", m_topic);
	pcommand_add("MO", m_motd);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
