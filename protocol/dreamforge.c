/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: dreamforge.c 902 2005-07-17 02:26:34Z alambert $
 */

#include "atheme.h"
#include "protocol/dreamforge.h"

/* *INDENT-OFF* */

ircd_t DreamForge = {
        "DreamForge 4.6.7 and later",   /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        FALSE,                          /* Whether or not we support channel owners. */
        FALSE,                          /* Whether or not we support channel protection. */
        FALSE,                          /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	FALSE,				/* Whether or not we use vHosts. */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+"                             /* Mode we set for halfops. */
};

struct cmode_ dreamforge_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'k', CMODE_KEY      },
  { 'l', CMODE_LIMIT    },
  { 'c', CMODE_NOCOLOR  },
  { 'R', CMODE_REGONLY  },
  { '\0', 0 }
};

struct cmode_ dreamforge_ignore_mode_list[] = {
  { 'e', CMODE_EXEMPT },
  { 'I', CMODE_INVEX  },
  { '\0', 0 }
};

struct cmode_ dreamforge_status_mode_list[] = {
  { 'o', CMODE_OP      },
  { 'v', CMODE_VOICE   },
  { '\0', 0 }
};

struct cmode_ dreamforge_prefix_mode_list[] = {
  { '@', CMODE_OP      },
  { '+', CMODE_VOICE   },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t dreamforge_server_login(void)
{
        int8_t ret;

        ret = sts("PASS %s", curr_uplink->pass);
        if (ret == 1)
                return 1;

        me.bursting = TRUE;

        sts("PROTOCTL NOQUIT WATCH=128 SAFELIST");
        sts("SERVER %s 1 :%s", me.name, me.desc);

	services_init();

        return 0;
}

/* introduce a client */
static user_t *dreamforge_introduce_nick(char *nick, char *user, char *host, char *real, char *modes)
{
	user_t *u;

	sts("NICK %s 1 %ld %s %s %s 0 :%s", nick, CURRTIME, user, host, me.name, real);
	sts(":%s MODE %s +%s", nick, nick, modes);

	u = user_add(nick, user, host, NULL, NULL, me.me);
	if (strchr(modes, 'o'))
		u->flags |= UF_IRCOP;

	return u;
}

static void dreamforge_quit_sts(user_t *u, char *reason)
{
        if (!me.connected)
                return;

        sts(":%s QUIT :%s", u->nick, reason);

        user_delete(u->nick);
}

/* WALLOPS wrapper */
static void dreamforge_wallops(char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        if (config_options.silent)
                return;

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

        sts(":%s GLOBOPS :%s", chansvs.nick, buf);
}

/* join a channel */
static void dreamforge_join(char *chan, char *nick)
{
	channel_t *c = channel_find(chan);
	chanuser_t *cu;

	if (!c)
	{
		sts(":%s JOIN %s", nick, chan);

		c = channel_add(chan, CURRTIME);
	}
	else
	{
		if ((cu = chanuser_find(c, user_find(nick))))
		{
			slog(LG_DEBUG, "join(): i'm already in `%s'", c->name);
			return;
		}

		sts(":%s JOIN %s", nick, chan);
		sts(":%s MODE %s +o %s %ld", nick, chan, nick, c->ts);
	}

	cu = chanuser_add(c, nick);
	cu->modes |= CMODE_OP;
}

/* kicks a user from a channel */
static void dreamforge_kick(char *from, char *channel, char *to, char *reason)
{
        channel_t *chan = channel_find(channel);
        user_t *user = user_find(to);

        if (!chan || !user)
                return;

        sts(":%s KICK %s %s :%s", from, channel, to, reason);

        chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void dreamforge_msg(char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s PRIVMSG %s :%s", chansvs.nick, target, buf);
}

/* NOTICE wrapper */
static void dreamforge_notice(char *from, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s NOTICE %s :%s", from, target, buf);
}

static void dreamforge_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void dreamforge_skill(char *from, char *nick, char *fmt, ...)
{
        va_list ap;
        char buf[BUFSIZE];

        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZE, fmt, ap);
        va_end(ap);

        sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void dreamforge_part(char *chan, char *nick)
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
static void dreamforge_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

        sts(":%s AKILL %s %s %ld %s %ld :%s", me.name, host, user, duration, chansvs.nick, time(NULL),
                reason);
}

/* server-to-server UNKLINE wrapper */
static void dreamforge_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s RAKILL %s %s", me.name, host, user);
}

/* topic wrapper */
static void dreamforge_topic_sts(char *channel, char *setter, char *topic)
{
        if (!me.connected)
                return;

        sts(":%s TOPIC %s %s %ld :%s", chansvs.nick, channel, setter, CURRTIME, topic);
}

/* mode wrapper */
static void dreamforge_mode_sts(char *sender, char *target, char *modes)
{
        if (!me.connected)
                return;

        sts(":%s MODE %s %s", sender, target, modes);
}

/* ping wrapper */
static void dreamforge_ping_sts(void)
{
        if (!me.connected)
                return;

        sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void dreamforge_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

        /* Can only record identified state if logged in to correct nick,
         * sorry -- jilles
         */
        if (irccasecmp(origin, user))
                return;

	sts(":%s SVSMODE %s +rd %ld", chansvs.nick, origin, time(NULL));
}

/* protocol-specific stuff to do on login */
static void dreamforge_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

        if (irccasecmp(origin, user))
                return;

	sts(":%s SVSMODE %s -r+d %ld", chansvs.nick, origin, time(NULL));
}

static void dreamforge_jupe(char *server, char *reason)
{
        if (!me.connected)
                return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
        sts(":%s SERVER %s 2 :%s", me.name, server, reason);
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
	service_t *sptr;

	/* we should have no more and no less */
	if (parc != 2)
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

	sptr = find_service(parv[0]);

	if (sptr)
		sptr->handler(origin, parc, parv);
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

	if (parc == 8)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		if ((k = kline_find(parv[3], parv[4])))
		{
			/* the new user matches a kline.
			 * the server introducing the user probably wasn't around when
			 * we added the kline or isn't accepting klines from us.
			 * either way, we'll KILL the user and send the server
			 * a new KLINE.
			 */

			skill(opersvs.nick, parv[0], k->reason);
			kline_sts(parv[5], k->user, k->host, (k->expires - CURRTIME), k->reason);

			return;
		}

		user_add(parv[0], parv[3], parv[4], NULL, NULL, s);

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

                /* fix up +e if necessary -- jilles */
                if (u->myuser != NULL && irccasecmp(u->nick, parv[0]) &&
                                !irccasecmp(parv[0], u->myuser->name))
                        /* changed nick to registered one, reset +e */
                        sts(":%s MODE %s +rd %ld", me.name, parv[0], CURRTIME);

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
		channel_mode(channel_find(parv[0]), parc - 2, &parv[1]);
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
	server_add(parv[0], atoi(parv[1]), NULL, parv[2]);

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
		  sts(":%s 213 %s C *@127.0.0.1 A %s %d uplink", me.name, u->nick, (is_ircop(u)) ? curr_uplink->name : "127.0.0.1", me.port);
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
		  sts(":%s 244 %s H * * %s", me.name, u->nick, (is_ircop(u)) ? curr_uplink->name : "127.0.0.1");
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
	else
	{
		channel_t *c = channel_find(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_join(): new channel: %s", parv[0]);
			c = channel_add(parv[0], CURRTIME);
			cu = chanuser_add(c, origin);
			cu->modes |= CMODE_OP;
		}
		else
			cu = chanuser_add(c, origin);
	}
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

void _modinit(module_t *m)
{
        /* Symbol relocation voodoo. */
        server_login = &dreamforge_server_login;
        introduce_nick = &dreamforge_introduce_nick;
        quit_sts = &dreamforge_quit_sts;
        wallops = &dreamforge_wallops;
        join = &dreamforge_join;
        kick = &dreamforge_kick;
        msg = &dreamforge_msg;
        notice = &dreamforge_notice;
        numeric_sts = &dreamforge_numeric_sts;
        skill = &dreamforge_skill;
        part = &dreamforge_part;
        kline_sts = &dreamforge_kline_sts;
        unkline_sts = &dreamforge_unkline_sts;
        topic_sts = &dreamforge_topic_sts;
        mode_sts = &dreamforge_mode_sts;
        ping_sts = &dreamforge_ping_sts;
        ircd_on_login = &dreamforge_on_login;   
        ircd_on_logout = &dreamforge_on_logout;
	jupe = &dreamforge_jupe;

	mode_list = dreamforge_mode_list;
	ignore_mode_list = dreamforge_ignore_mode_list;
	status_mode_list = dreamforge_status_mode_list;
	prefix_mode_list = dreamforge_prefix_mode_list;

	ircd = &DreamForge;

        pcommand_add("PING", m_ping);
        pcommand_add("PONG", m_pong);
        pcommand_add("PRIVMSG", m_privmsg);
        pcommand_add("PART", m_part);
        pcommand_add("NICK", m_nick);
        pcommand_add("QUIT", m_quit);
        pcommand_add("MODE", m_mode);
        pcommand_add("KICK", m_kick);
        pcommand_add("KILL", m_kill);
        pcommand_add("SQUIT", m_squit);
        pcommand_add("SERVER", m_server);
        pcommand_add("STATS", m_stats);
        pcommand_add("ADMIN", m_admin);
        pcommand_add("VERSION", m_version);
        pcommand_add("INFO", m_info);
        pcommand_add("JOIN", m_join);
        pcommand_add("PASS", m_pass);
        pcommand_add("ERROR", m_error);
        pcommand_add("TOPIC", m_topic);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
