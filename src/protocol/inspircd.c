/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: inspircd.c 329 2005-06-04 21:20:38Z nenolod $
 */

#include "atheme.h"
#include "protocol/unreal.h"

char *tldprefix = "$";

/* *INDENT-OFF* */

struct cmode_ mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'k', CMODE_KEY      },
  { 'l', CMODE_LIMIT    },
  { 'c', CMODE_NOCOLOR  },
  { 'M', CMODE_MODREG   },
  { 'R', CMODE_REGONLY  },
  { 'O', CMODE_OPERONLY },
  { 'A', CMODE_ADMONLY  },
  { 'Q', CMODE_PEACE    },
  { 'S', CMODE_STRIP    },
  { 'K', CMODE_NOKNOCK  },
  { 'V', CMODE_NOINVITE },
  { 'C', CMODE_NOCTCP   },
  { 'u', CMODE_HIDING   },
  { 'z', CMODE_SSLONLY  },
  { 'N', CMODE_STICKY   },
  { '\0', 0 }
};

struct cmode_ ignore_mode_list[] = {
  { 'e', CMODE_EXEMPT },
  { 'I', CMODE_INVEX  },
  { '\0', 0 }
};

struct cmode_ status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

/* *INDENT-ON* */

static char *xsumtable = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static char *CreateSum()
{
	int q = 0;
        static char sum[8];
        sum[7] = '\0';
        for(q = 0; q < 7; q++)
                sum[q] = xsumtable[rand()%52];
        return sum;
}

/* login to our uplink */
uint8_t server_login(void)
{
	srand(time(NULL));

        me.bursting = TRUE;

        sts(":%s U %s %s :%s", CreateSum(), me.name, me.pass, me.desc);

        return 0;
}

/* introduce a client */
user_t *introduce_nick(char *nick, char *user, char *host, char *real, char *modes)
{
	user_t *u;

	/*         ts  ni ho vh id mod ip      sv  rn */
	sts(":%s N %ld %s %s %s %s +%s 0.0.0.0 %s :%s", CreateSum(), CURRTIME, nick, host, host, user, modes, me.name, real);

	u = user_add(nick, user, host, me.me);
	if (strchr(modes, 'o'))
		u->flags |= UF_IRCOP;

	return u;
}

void quit_sts(user_t *u, char *reason)
{
        if (!me.connected || !u)
                return;

        sts(":%s Q %s %s", CreateSum(), u->nick, reason);

        user_delete(u->nick);
}

/* WALLOPS wrapper */
void wallops(char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        if (config_options.silent)
                return;

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);

        sts(":%s @ %s :%s", CreateSum(), chansvs.nick, buf);
}

/* join a channel */
void join(char *chan, char *nick)
{
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!c)
	{
		sts(":%s J %s @%s", CreateSum(), nick, chan);

		c = channel_add(chan, CURRTIME);
	}
	else
	{
		if ((cu = chanuser_find(c, user_find(chansvs.nick))))
		{
			slog(LG_DEBUG, "join(): i'm already in `%s'", c->name);
			return;
		}

		sts(":%s J %s @%s", CreateSum(), nick, chan);
	}

	cu = chanuser_add(c, nick);
	cu->modes |= CMODE_OP;
}

/* kicks a user from a channel */
void kick(char *from, char *channel, char *to, char *reason)
{
        channel_t *chan = channel_find(channel);
        user_t *user = user_find(to);

        if (!chan || !user)
                return;

        sts(":%s K %s %s %s :%s", CreateSum(), from, channel, to, reason);

        chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
void msg(char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s P %s %s :%s", CreateSum(), chansvs.nick, target, buf);
}

/* NOTICE wrapper */
void notice(char *from, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s V %s %s :%s", CreateSum(), from, target, buf);
}

void numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s * %s %d %s %s", CreateSum(), from, numeric, target, buf);
}

/* KILL wrapper */
void skill(char *from, char *nick, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s K %s %s :%s", CreateSum(), from, nick, buf);
}

/* PART wrapper */
void part(char *chan, char *nick)
{
        user_t *u = user_find(nick);
        channel_t *c = channel_find(chan);
        chanuser_t *cu;

        if (!u || !c)
                return;

        if (!(cu = chanuser_find(c, u)))
                return;

        sts(":%s L %s %s", CreateSum(), u->nick, c->name);

        chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
void kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s # %s@%s %s %ld 0 :%s", CreateSum(), user, host, chansvs.nick, time(NULL), 
		reason);
}

/* server-to-server UNKLINE wrapper */
void unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s . %s@%s %s", CreateSum(), user, host, chansvs.nick);
}

/* topic wrapper */
void topic_sts(char *channel, char *setter, char *topic)
{
        if (!me.connected)
                return;

        sts(":%s t %s %s :%s (%s)", CreateSum(), chansvs.nick, channel, topic, setter);
}

/* mode wrapper */
void mode_sts(char *sender, char *target, char *modes)
{
        if (!me.connected)
                return;

        sts(":%s m %s %s %s", CreateSum(), sender, target, modes);
}

/* ping wrapper */
void ping_sts(void)
{
        if (!me.connected)
                return;

        sts("?", CreateSum());
}

/* protocol-specific stuff to do on login */
void ircd_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	sts(":%s m %s %s +r", CreateSum(), chansvs.nick, origin);
}

/* protocol-specific stuff to do on login */
void ircd_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	sts(":%s m %s %s -r", CreateSum(), chansvs.nick, origin);
}

static void m_topic(char *origin, uint8_t parc, char *parv[])
{
        channel_t *c = channel_find(parv[0]);

        if (!origin)
                return;   

        c->topic = sstrdup(parv[3]);
        c->topic_setter = sstrdup(parv[1]);
}

static void m_ping(char *origin, uint8_t parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s !", CreateSum());
}

static void m_pong(char *origin, uint8_t parc, char *parv[])
{
	/* someone replied to our PING */
	if ((!parv[0]) || (strcasecmp(me.actual, parv[0])))
		return;

	me.uplinkpong = CURRTIME;

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

/* inspircd is lame and puts the real origin as the first option */
static void m_privmsg(char *origin, uint8_t parc, char *parv[])
{
	user_t *u;
	char buf[BUFSIZE];

	/* we should have no more and no less */
	if (parc != 3)
		return;

	/* don't care about channels. */
	if (parv[1][0] == '#')
		return;

	if (!(u = user_find(parv[0])))
	{
		slog(LG_DEBUG, "m_privmsg(): got message from nonexistant user `%s'", origin);
		return;
	}

	/* run it through flood checks */
	if ((config_options.flood_msgs) && (!is_sra(u->myuser)) && (!is_ircop(u)))
	{
		/* check if they're being ignored */
		if (u->offenses > 10)
		{
			if ((CURRTIME - u->lastmsg) > 30)
			{
				u->offenses -= 10;
				u->lastmsg = CURRTIME;
				u->msgs = 0;
			}
			else
				return;
		}

		if ((CURRTIME - u->lastmsg) > config_options.flood_time)
		{
			u->lastmsg = CURRTIME;
			u->msgs = 0;
		}

		u->msgs++;

		if (u->msgs > config_options.flood_msgs)
		{
			/* they're flooding. */
			if (!u->offenses)
			{
				/* ignore them the first time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 11;

				notice(parv[1], parv[0], "You have triggered services flood protection.");
				notice(parv[1], parv[0], "This is your first offense. You will be ignored for " "30 seconds.");

				snoop("FLOOD: \2%s\2", u->nick);

				return;
			}

			if (u->offenses == 1)
			{
				/* ignore them the second time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 12;

				notice(parv[1], parv[0], "You have triggered services flood protection.");
				notice(parv[1], parv[0], "This is your last warning. You will be ignored for " "30 seconds.");

				snoop("FLOOD: \2%s\2", u->nick);

				return;
			}

			if (u->offenses == 2)
			{
				kline_t *k;

				/* kline them the third time */
				k = kline_add("*", u->host, "ten minute ban: flooding services", 600);
				k->setby = sstrdup(chansvs.nick);

				snoop("FLOOD:KLINE: \2%s\2", u->nick);

				return;
			}
		}
	}

	/* is it for our CService client? */
	if (!irccasecmp(parv[1], chansvs.nick))
	{
		/* Yes, I know this is craq++'ed. Deal with it. --nenolod */
		parv[1] = parv[2];
		parv[2] = NULL;

		/* for nick@server messages.. */
		snprintf(buf, sizeof(buf), "%s@%s", chansvs.nick, me.name);

		/* okay, give it to our client */
		cservice(parv[0], parc, parv);
		return;
	}

	/* is it for GService? */
	if (!irccasecmp(parv[1], globsvs.nick))
	{
		parv[1] = parv[2];
		parv[2] = NULL;

		gservice(parv[0], parc, parv);
		return;
	}

	/* OService perhaps? */
	if (!irccasecmp(parv[1], opersvs.nick))
	{
		parv[1] = parv[2];
		parv[2] = NULL;

		oservice(parv[0], parc, parv);
		return;
	}

	/* ...NickServ? */
	if (!irccasecmp(parv[1], nicksvs.nick))
	{
		parv[1] = parv[2];
		parv[2] = NULL;

		nickserv(parv[0], parc, parv);
		return;
	}
}

static void m_sjoin(char *origin, uint8_t parc, char *parv[])
{
	int8_t i;

	for (i = 1; i < parc; i++)
	{
		char buf[128];
		channel_t *c;

		memset(buf, 0, 128);

		slog(LG_DEBUG, "m_sjoin(): Processing %s", parv[i]);

		if (*parv[i] != '#')
		{
			buf[0] = *parv[i];
			parv[i]++;
		}

		c = channel_find(parv[i]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel %s", parv[i]);
			c = channel_add(parv[i], CURRTIME);
		}

		strlcat(buf, parv[0], 128);

		slog(LG_DEBUG, "m_sjoin(): converted into %s", buf);
		chanuser_add(c, buf);
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
	kline_t *k;

	if (parc == 9)
	{
		s = server_find(parv[7]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[7]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[1]);

		if ((k = kline_find(parv[3], parv[4])))
		{
			/* the new user matches a kline.
			 * the server introducing the user probably wasn't around when
			 * we added the kline or isn't accepting klines from us.
			 * either way, we'll KILL the user and send the server
			 * a new KLINE.
			 */

			skill(chansvs.nick, parv[1], k->reason);
			kline_sts(parv[7], k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		user_add(parv[1], parv[4], parv[2], s);

		user_mode(user_find(parv[1]), parv[7]);

		handle_nickchange(user_find(parv[1]));
	}

	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		node_t *n;

		u = user_find(parv[0]);
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
		free(u->nick);
		u->nick = sstrdup(parv[1]);

		/* readd with new nick (so the hash works) */
		n = node_create();
		u->hash = UHASH((unsigned char *)u->nick);
		node_add(u, n, &userlist[u->hash]);

		handle_nickchange(u);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_nick(): got NICK with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_nick():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_quit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", origin);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(parv[0]);
}

static void m_mode(char *origin, uint8_t parc, char *parv[])
{
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

	if (*parv[1] == '#')
		channel_mode(channel_find(parv[1]), parc - 1, &parv[3]);
	else
		user_mode(user_find(parv[1]), parv[2]);
}

static void m_smode(char *origin, uint8_t parc, char *parv[])
{
	if (parc < 2)
	{
		slog(LG_DEBUG, "m_mode(): missing parameters in MODE");
		return;
	}

	if (*parv[0] == '#')
		channel_mode(channel_find(parv[0]), parc - 1, &parv[2]);
	else
		user_mode(user_find(parv[0]), parv[1]);
}

static void m_kick(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(parv[1]);
	channel_t *c = channel_find(parv[2]);

	/* -> :rakaur KICK #shrike rintaun :test */
	slog(LG_DEBUG, "m_kick(): user was kicked: %s -> %s", parv[1], parv[2]);

	if (!u)
	{
		slog(LG_DEBUG, "m_kick(): got kick for nonexistant user %s", parv[1]);
		return;
	}

	if (!c)
	{
		slog(LG_DEBUG, "m_kick(): got kick in nonexistant channel: %s", parv[2]);
		return;
	}

	if (!chanuser_find(c, u))
	{
		slog(LG_DEBUG, "m_kick(): got kick for %s not in %s", u->nick, c->name);
		return;
	}

	chanuser_delete(c, u);

	/* if they kicked us, let's rejoin */
	if (!irccasecmp(chansvs.nick, parv[1]))
	{
		slog(LG_DEBUG, "m_kick(): i got kicked from `%s'; rejoining", parv[2]);
		join(parv[2], parv[1]);
	}
}

static void m_kill(char *origin, uint8_t parc, char *parv[])
{
	mychan_t *mc;
	node_t *n;
	int i;

	slog(LG_DEBUG, "m_kill(): killed user: %s", parv[1]);
	user_delete(parv[1]);

	if (!irccasecmp(chansvs.nick, parv[1]) || !irccasecmp(opersvs.nick, parv[1]) || !irccasecmp(globsvs.nick, parv[1]))
	{
		services_init();

		if (config_options.chan)
			join(config_options.chan, chansvs.nick);

		for (i = 0; i < HASHSIZE; i++)
		{
			LIST_FOREACH(n, mclist[i].head)
			{
				mc = (mychan_t *)n->data;

				if ((config_options.join_chans) && (mc->chan) && (mc->chan->nummembers >= 1))
					join(mc->name, chansvs.nick);

				if ((config_options.join_chans) && (!config_options.leave_chans) && (mc->chan) && (mc->chan->nummembers == 0))
					join(mc->name, chansvs.nick);
			}
		}
	}
}

static void m_squit(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

static void m_server(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[1]), parv[2]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);

	services_init();
}

static void m_error(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_eos(char *origin, uint8_t parc, char *parv[])
{
        sts(":%s H %s", CreateSum(), me.name);
	sts(":%s / %s", CreateSum(), chansvs.nick);
	sts(":%s v %s :atheme-%s. %s %s%s%s%s%s%s%s%s%s",
            CreateSum(), me.name, version, me.name,
            (match_mapping) ? "A" : "",
            (me.loglevel & LG_DEBUG) ? "d" : "",
            (me.auth) ? "e" : "",
            (config_options.flood_msgs) ? "F" : "",
            (config_options.leave_chans) ? "l" : "", (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");
}

/* *INDENT-OFF* */

/* irc we understand */
struct message_ messages[] = {
  { "?",    m_ping    },
  { "!",    m_pong    },
  { "P",    m_privmsg },
  { "J",    m_sjoin   },
  { "L",    m_part    },
  { "N",    m_nick    },
  { "n",    m_nick    },
  { "Q",    m_quit    },
  { "M",    m_smode   },
  { "m",    m_mode    },
  { "k",    m_kick    },
  { "K",    m_kill    },
  { "&",    m_squit   },
  { "s",    m_server  },
  { "E",    m_error   },
  { "t",    m_topic   },
  { "T",    m_topic   },
  { "F",    m_eos     },
  { NULL }
};

/* *INDENT-ON* */

/* find a matching function */
struct message_ *msg_find(const char *name)
{
	struct message_ *m;

	for (m = messages; m->name; m++)
		if (!strcmp(name, m->name))
			return m;

	return NULL;
}
