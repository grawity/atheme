/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: bahamut.c 329 2005-06-04 21:20:38Z nenolod $
 */

#include "atheme.h"
#include "protocol/bahamut.h"

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

/* login to our uplink */
uint8_t server_login(void)
{
        int8_t ret;

        ret = sts("PASS %s :TS", me.pass);
        if (ret == 1)
                return 1;

        me.bursting = TRUE;

        sts("CAPAB CAPAB SSJOIN NOQUIT BURST UNCONNECT ZIP NICKIP TSMODE");
        sts("SERVER %s 1 :%s", me.name, me.desc);
        sts("SVINFO 5 3 0 :%ld", CURRTIME);

	services_init();

        return 0;
}

/* introduce a client */
user_t *introduce_nick(char *nick, char *user, char *host, char *real, char *modes)
{
	user_t *u;

	sts("NICK %s 1 %ld +%s %s %s %s 0 0 :%s", nick, CURRTIME, modes, user, host, me.name, real);

	u = user_add(nick, user, host, me.me);
	if (strchr(modes, 'o'))
		u->flags |= UF_IRCOP;

	return u;
}

void quit_sts(user_t *u, char *reason)
{
        if (!me.connected)
                return;

        sts(":%s QUIT :%s", u->nick, reason);

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

        sts(":%s GLOBOPS :%s", chansvs.nick, buf);
}

/* join a channel */
void join(char *chan, char *nick)
{
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!c)
	{
		sts(":%s SJOIN %ld %s +nt :@%s", me.name, CURRTIME, chan, nick);

		c = channel_add(chan, CURRTIME);
	}
	else
	{
		if ((cu = chanuser_find(c, user_find(chansvs.nick))))
		{
			slog(LG_DEBUG, "join(): i'm already in `%s'", c->name);
			return;
		}

		sts(":%s SJOIN %ld %s + :@%s", me.name, c->ts, chan, nick);
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

        sts(":%s KICK %s %s :%s", from, channel, to, reason);

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

        sts(":%s PRIVMSG %s :%s", chansvs.nick, target, buf);
}

/* NOTICE wrapper */
void notice(char *from, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s NOTICE %s :%s", from, target, buf);
}

void numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
void skill(char *from, char *nick, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
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

        sts(":%s PART %s", u->nick, c->name);

        chanuser_delete(c, u);
}

/* server-to-server KLINE wrapper */
void kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s AKILL %s %s %ld %s %ld :%s", me.name, host, user, duration, chansvs.nick, time(NULL), 
		reason);
}

/* server-to-server UNKLINE wrapper */
void unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s RAKILL %s %s", me.name, host, user);
}

/* topic wrapper */
void topic_sts(char *channel, char *setter, char *topic)
{
        if (!me.connected)
                return;

        sts(":%s TOPIC %s %s %ld :%s", chansvs.nick, channel, setter, CURRTIME, topic);
}

/* mode wrapper */
void mode_sts(char *sender, char *target, char *modes)
{
	channel_t *c = channel_find(target);

        if (!me.connected || !c)
                return;

        sts(":%s MODE %s %ld %s", sender, target, c->ts, modes);
}

/* ping wrapper */
void ping_sts(void)
{
        if (!me.connected)
                return;

        sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
void ircd_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	sts(":%s SVSMODE %s +rd %ld", chansvs.nick, origin, time(NULL));
}

/* protocol-specific stuff to do on login */
void ircd_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	sts(":%s SVSMODE %s -r+d %ld", chansvs.nick, origin, time(NULL));
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
	sts(":%s PONG %s %s", me.name, me.name, parv[0]);
}

static void m_pong(char *origin, uint8_t parc, char *parv[])
{
	/* someone replied to our PING */
	if ((!parv[0]) || (strcasecmp(me.actual, parv[0])))
		return;

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
	user_t *u;
	char buf[BUFSIZE];

	/* we should have no more and no less */
	if (parc != 2)
		return;

	/* don't care about channels. */
	if (parv[0][0] == '#')
		return;

	if (!(u = user_find(origin)))
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

				notice(parv[0], origin, "You have triggered services flood protection.");
				notice(parv[0], origin, "This is your first offense. You will be ignored for " "30 seconds.");

				snoop("FLOOD: \2%s\2", u->nick);

				return;
			}

			if (u->offenses == 1)
			{
				/* ignore them the second time */
				u->lastmsg = CURRTIME;
				u->msgs = 0;
				u->offenses = 12;

				notice(parv[0], origin, "You have triggered services flood protection.");
				notice(parv[0], origin, "This is your last warning. You will be ignored for " "30 seconds.");

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
	if (!irccasecmp(parv[0], chansvs.nick))
	{
		/* for nick@server messages.. */
		snprintf(buf, sizeof(buf), "%s@%s", chansvs.nick, me.name);

		/* okay, give it to our client */
		cservice(origin, parc, parv);
		return;
	}

	/* is it for GService? */
	if (!irccasecmp(parv[0], globsvs.nick))
	{
		gservice(origin, parc, parv);
		return;
	}

	/* OService perhaps? */
	if (!irccasecmp(parv[0], opersvs.nick))
	{
		oservice(origin, parc, parv);
		return;
	}

	/* ...NickServ? */
	if (!irccasecmp(parv[0], nicksvs.nick))
	{
		nickserv(origin, parc, parv);
		return;
	}
}

static void m_sjoin(char *origin, uint8_t parc, char *parv[])
{
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *	also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 */

	channel_t *c;
	uint8_t modec = 0;
	char *modev[16];
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;

	if (origin && parc == 4)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
		modev[0] = parv[2];

		if (parc > 4)
			modev[++modec] = parv[3];
		if (parc > 5)
			modev[++modec] = parv[4];

		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts);
		}

		if (ts < c->ts)
		{
			chanuser_t *cu;
			node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 */

			c->modes = 0;
			c->limit = 0;
			if (c->key)
				free(c->key);
			c->key = NULL;

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes = 0;
			}

			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
		}

		channel_mode(c, modec, modev);

		userc = sjtoken(parv[parc - 1], userv);

		for (i = 0; i < userc; i++)
			chanuser_add(c, userv[i]);
	}
	else
	{
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (ts < c->ts)
		{
			chanuser_t *cu;
			node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 */

			c->modes = 0;
			c->limit = 0;
			if (c->key)
				free(c->key);
			c->key = NULL;

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				cu->modes = 0;
			}

			slog(LG_INFO, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
		}

		chanuser_add(c, origin);
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

	slog(LG_DEBUG, "%d", parc);

	if (parc == 10)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		if ((k = kline_find(parv[4], parv[5])))
		{
			/* the new user matches a kline.
			 * the server introducing the user probably wasn't around when
			 * we added the kline or isn't accepting klines from us.
			 * either way, we'll KILL the user and send the server
			 * a new KLINE.
			 */

			skill(chansvs.nick, parv[0], k->reason);
			kline_sts(parv[6], k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		user_add(parv[0], parv[4], parv[5], s);

		user_mode(user_find(parv[0]), parv[3]);

		handle_nickchange(user_find(parv[0]));
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
		free(u->nick);
		u->nick = sstrdup(parv[0]);

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
	user_delete(origin);
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

	if (*parv[0] == '#')
		channel_mode(channel_find(parv[0]), parc - 1, &parv[2]);
	else
		user_mode(user_find(parv[0]), parv[1]);
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
	if (!irccasecmp(chansvs.nick, parv[1]))
	{
		slog(LG_DEBUG, "m_kick(): i got kicked from `%s'; rejoining", parv[0]);
		join(parv[0], parv[1]);
	}
}

static void m_kill(char *origin, uint8_t parc, char *parv[])
{
	mychan_t *mc;
	node_t *n;
	int i;

	slog(LG_DEBUG, "m_kill(): killed user: %s", parv[0]);
	user_delete(parv[0]);

	if (!irccasecmp(chansvs.nick, parv[0]))
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
}

static void m_stats(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);
	kline_t *k;
	node_t *n;
	int i;

	if (!parv[0][0])
		return;

	if (irccasecmp(me.name, parv[1]))
		return;

	snoop("STATS:%c: \2%s\2", parv[0][0], origin);

	switch (parv[0][0])
	{
	  case 'C':
	  case 'c':
		  sts(":%s 213 %s C *@127.0.0.1 A %s %d uplink", me.name, u->nick, (is_ircop(u)) ? me.uplink : "127.0.0.1", me.port);
		  break;

	  case 'E':
	  case 'e':
		  if (!is_ircop(u))
			  break;

		  sts(":%s 249 %s E :Last event to run: %s", me.name, u->nick, last_event_ran);

		  for (i = 0; i < MAX_EVENTS; i++)
		  {
			  if (event_table[i].active)
				  sts(":%s 249 %s E :%s (%d)", me.name, u->nick, event_table[i].name, event_table[i].frequency);
		  }

		  break;

	  case 'H':
	  case 'h':
		  sts(":%s 244 %s H * * %s", me.name, u->nick, (is_ircop(u)) ? me.uplink : "127.0.0.1");
		  break;

	  case 'I':
	  case 'i':
		  sts(":%s 215 %s I * * *@%s 0 nonopered", me.name, u->nick, me.name);
		  break;

	  case 'K':
		  if (!is_ircop(u))
			  break;

		  LIST_FOREACH(n, klnlist.head)
		  {
			  k = (kline_t *)n->data;

			  if (!k->duration)
				  sts(":%s 216 %s K %s * %s :%s", me.name, u->nick, k->host, k->user, k->reason);
		  }

		  break;

	  case 'k':
		  if (!is_ircop(u))
			  break;

		  LIST_FOREACH(n, klnlist.head)
		  {
			  k = (kline_t *)n->data;

			  if (k->duration)
				  sts(":%s 216 %s k %s * %s :%s", me.name, u->nick, k->host, k->user, k->reason);
		  }

		  break;

	  case 'T':
	  case 't':
		  if (!is_ircop(u))
			  break;

		  sts(":%s 249 %s :event      %7d", me.name, u->nick, cnt.event);
		  sts(":%s 249 %s :sra        %7d", me.name, u->nick, cnt.sra);
		  sts(":%s 249 %s :tld        %7d", me.name, u->nick, cnt.tld);
		  sts(":%s 249 %s :kline      %7d", me.name, u->nick, cnt.kline);
		  sts(":%s 249 %s :server     %7d", me.name, u->nick, cnt.server);
		  sts(":%s 249 %s :user       %7d", me.name, u->nick, cnt.user);
		  sts(":%s 249 %s :chan       %7d", me.name, u->nick, cnt.chan);
		  sts(":%s 249 %s :chanuser   %7d", me.name, u->nick, cnt.myuser);
		  sts(":%s 249 %s :mychan     %7d", me.name, u->nick, cnt.mychan);
		  sts(":%s 249 %s :chanacs    %7d", me.name, u->nick, cnt.chanacs);
		  sts(":%s 249 %s :node       %7d", me.name, u->nick, cnt.node);

		  sts(":%s 249 %s :bytes sent %7.2f%s", me.name, u->nick, bytes(cnt.bout), sbytes(cnt.bout));
		  sts(":%s 249 %s :bytes recv %7.2f%s", me.name, u->nick, bytes(cnt.bin), sbytes(cnt.bin));
		  break;

	  case 'u':
		  sts(":%s 242 %s :Services Up %s", me.name, u->nick, timediff(CURRTIME - me.start));
		  break;

	  default:
		  break;
	}

	sts(":%s 219 %s %c :End of STATS report", me.name, u->nick, parv[0][0]);
}

static void m_admin(char *origin, uint8_t parc, char *parv[])
{
	sts(":%s 256 %s :Administrative info about %s", me.name, origin, me.name);
	sts(":%s 257 %s :%s", me.name, origin, me.adminname);
	sts(":%s 258 %s :Atheme IRC Services (atheme-%s)", me.name, origin, version);
	sts(":%s 259 %s :<%s>", me.name, origin, me.adminemail);
}

static void m_version(char *origin, uint8_t parc, char *parv[])
{
	sts(":%s 351 %s :atheme-%s. %s %s%s%s%s%s%s%s%s%s TS5ow",
	    me.name, origin, version, me.name,
	    (match_mapping) ? "A" : "",
	    (me.loglevel & LG_DEBUG) ? "d" : "",
	    (me.auth) ? "e" : "",
	    (config_options.flood_msgs) ? "F" : "",
	    (config_options.leave_chans) ? "l" : "", (config_options.join_chans) ? "j" : "", (!match_mapping) ? "R" : "", (config_options.raw) ? "r" : "", (runflags & RF_LIVE) ? "n" : "");
        sts(":%s 351 %s :Compile time: %s, build-id %s, build %s", me.name, origin, creation, revision, generation);
        sts(":%s 351 %s :Compiled on: [%s]", me.name, origin, osinfo);
}

static void m_info(char *origin, uint8_t parc, char *parv[])
{
	uint8_t i;

	for (i = 0; infotext[i]; i++)
		sts(":%s 371 %s :%s", me.name, origin, infotext[i]);

	sts(":%s 374 %s :End of /INFO list", me.name, origin);
}

static void m_join(char *origin, uint8_t parc, char *parv[])
{
	user_t *u = user_find(origin);
	chanuser_t *cu;
	node_t *n, *tn;

	if (!u)
		return;

	/* JOIN 0 is really a part from all channels */
	if (parv[0][0] == '0')
	{
		LIST_FOREACH_SAFE(n, tn, u->channels.head)
		{
			cu = (chanuser_t *)n->data;
			chanuser_delete(cu->chan, u);
		}
	}
}

static void m_pass(char *origin, uint8_t parc, char *parv[])
{
	if (strcmp(me.pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void m_error(char *origin, uint8_t parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

/* *INDENT-OFF* */

/* irc we understand */
struct message_ messages[] = {
  { "PING",    m_ping    },
  { "PONG",    m_pong    },
  { "PRIVMSG", m_privmsg },
  { "SJOIN",   m_sjoin   },
  { "PART",    m_part    },
  { "NICK",    m_nick    },
  { "QUIT",    m_quit    },
  { "MODE",    m_mode    },
  { "KICK",    m_kick    },
  { "KILL",    m_kill    },
  { "SQUIT",   m_squit   },
  { "SERVER",  m_server  },
  { "STATS",   m_stats   },
  { "ADMIN",   m_admin   },
  { "VERSION", m_version },
  { "INFO",    m_info    },
  { "JOIN",    m_join    },
  { "PASS",    m_pass    },
  { "ERROR",   m_error   },
  { "TOPIC",   m_topic   },
  { NULL }
};

/* *INDENT-ON* */

/* find a matching function */
struct message_ *msg_find(const char *name)
{
	struct message_ *m;

	for (m = messages; m->name; m++)
		if (!strcasecmp(name, m->name))
			return m;

	return NULL;
}
