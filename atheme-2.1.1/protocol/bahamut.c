/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: bahamut.c 7281 2006-11-25 02:01:13Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/bahamut.h"

DECLARE_MODULE_V1("protocol/bahamut", TRUE, _modinit, NULL, "$Id: bahamut.c 7281 2006-11-25 02:01:13Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Bahamut = {
        "Bahamut 1.8.x",                /* IRCd name */
        "$",                            /* TLD Prefix, used by Global. */
        FALSE,                          /* Whether or not we use IRCNet/TS6 UID */
        FALSE,                          /* Whether or not we use RCOMMAND */
        FALSE,                          /* Whether or not we support channel owners. */
        FALSE,                          /* Whether or not we support channel protection. */
        FALSE,                          /* Whether or not we support halfops. */
	FALSE,				/* Whether or not we use P10 */
	FALSE,				/* Whether or not we use vHosts. */
	CMODE_OPERONLY,			/* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_BAHAMUT,		/* Protocol type */
	0,                              /* Permanent cmodes */
	"beI",                          /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I'                             /* Invex mchar */
};

struct cmode_ bahamut_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'c', CMODE_NOCOLOR  },
  { 'M', CMODE_MODREG   },
  { 'R', CMODE_REGONLY  },
  { 'O', CMODE_OPERONLY },
  { '\0', 0 }
};

struct extmode bahamut_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ bahamut_status_mode_list[] = {
  { 'o', CMODE_OP    },
  { 'v', CMODE_VOICE },
  { '\0', 0 }
};

struct cmode_ bahamut_prefix_mode_list[] = {
  { '@', CMODE_OP    },
  { '+', CMODE_VOICE },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t bahamut_server_login(void)
{
	int8_t ret;

	ret = sts("PASS %s :TS", curr_uplink->pass);
	if (ret == 1)
		return 1;

	me.bursting = TRUE;

	sts("CAPAB SSJOIN NOQUIT BURST ZIP NICKIP TSMODE");
	sts("SERVER %s 1 :%s", me.name, me.desc);
	sts("SVINFO 5 3 0 :%ld", CURRTIME);

	services_init();

	return 0;
}

/* introduce a client */
static void bahamut_introduce_nick(user_t *u)
{
	sts("NICK %s 1 %ld +%s %s %s %s 0 0 :%s", u->nick, u->ts, "io", u->user, u->host, me.name, u->gecos);
}

/* invite a user to a channel */
static void bahamut_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void bahamut_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void bahamut_wallops_sts(const char *text)
{
	sts(":%s GLOBOPS :%s", me.name, text);
}

/* join a channel */
static void bahamut_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
		sts(":%s SJOIN %ld %s %s :@%s", me.name, c->ts, c->name,
				modes, u->nick);
	else
		sts(":%s SJOIN %ld %s + :@%s", me.name, c->ts, c->name,
				u->nick);
}

static void bahamut_chan_lowerts(channel_t *c, user_t *u)
{
	slog(LG_DEBUG, "bahamut_chan_lowerts(): lowering TS for %s to %ld",
			c->name, (long)c->ts);
	sts(":%s SJOIN %ld %s %s :@%s", me.name, c->ts, c->name,
				channel_modes(c, TRUE), u->nick);
	chanban_clear(c);
	/*handle_topic(c, "", 0, "");*/
	/* Don't destroy keeptopic info, I'll admit this is ugly -- jilles */
	if (c->topic != NULL)
		free(c->topic);
	if (c->topic_setter != NULL)
		free(c->topic_setter);
	c->topic = c->topic_setter = NULL;
	c->topicts = 0;
}

/* kicks a user from a channel */
static void bahamut_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void bahamut_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void bahamut_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void bahamut_notice_global_sts(user_t *from, const char *mask, const char *text)
{
	node_t *n;
	tld_t *tld;

	if (!strcmp(mask, "*"))
	{
		LIST_FOREACH(n, tldlist.head)
		{
			tld = n->data;
			sts(":%s NOTICE %s*%s :%s", from ? from->nick : me.name, ircd->tldprefix, tld->name, text);
		}
	}
	else
		sts(":%s NOTICE %s%s :%s", from ? from->nick : me.name, ircd->tldprefix, mask, text);
}

static void bahamut_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void bahamut_wallchops(user_t *sender, channel_t *channel, char *message)
{
	sts(":%s NOTICE @%s :%s", sender->nick, channel->name, message);
}

static void bahamut_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void bahamut_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void bahamut_part(char *chan, char *nick)
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
static void bahamut_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s AKILL %s %s %ld %s %ld :%s", me.name, host, user, duration, opersvs.nick, time(NULL), reason);
}

/* server-to-server UNKLINE wrapper */
static void bahamut_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s RAKILL %s %s", me.name, host, user);
}

/* topic wrapper */
static void bahamut_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s TOPIC %s %s %ld :%s", chansvs.nick, channel, setter, ts, topic);
}

/* mode wrapper */
static void bahamut_mode_sts(char *sender, char *target, char *modes)
{
	channel_t *c = channel_find(target);

	if (!me.connected || !c)
		return;

	sts(":%s MODE %s %ld %s", sender, target, c->ts, modes);
}

/* ping wrapper */
static void bahamut_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void bahamut_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	/* Can only do this for nickserv, and can only record identified
	 * state if logged in to correct nick, sorry -- jilles
	 */
	if (nicksvs.no_nick_ownership || irccasecmp(origin, user))
		return;

	sts(":%s SVSMODE %s +rd %ld", nicksvs.nick, origin, time(NULL));
}

/* protocol-specific stuff to do on login */
static boolean_t bahamut_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return FALSE;

	if (nicksvs.no_nick_ownership || irccasecmp(origin, user))
		return FALSE;

	sts(":%s SVSMODE %s -r+d %ld", nicksvs.nick, origin, time(NULL));
	return FALSE;
}

static void bahamut_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
}

static void bahamut_fnc_sts(user_t *source, user_t *u, char *newnick, int type)
{
	sts(":%s SVSNICK %s %s %lu", source->nick, u->nick, newnick,
			(unsigned long)(CURRTIME - 60));
}

static void bahamut_holdnick_sts(user_t *source, int duration, const char *nick, myuser_t *account)
{
	sts(":%s SVSHOLD %s %d :Reserved by %s for nickname owner (%s)",
			source->nick, nick, duration, source->nick,
			account != NULL ? account->name : nick);
}

static void m_topic(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c = channel_find(parv[0]);

	if (!c)
		return;

	handle_topic_from(si, c, parv[1], atol(parv[2]), parv[3]);
}

static void m_ping(sourceinfo_t *si, int parc, char *parv[])
{
	/* reply to PING's */
	sts(":%s PONG %s %s", me.name, me.name, parv[0]);
}

static void m_pong(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;

	/* someone replied to our PING */
	if (!parv[0])
		return;
	s = server_find(parv[0]);
	if (s == NULL)
		return;
	/* Postpone EOB for our uplink until topic burst is also done */
	if (s->uplink != me.me)
		handle_eob(s);

	if (irccasecmp(me.actual, parv[0]))
		return;

	me.uplinkpong = CURRTIME;

	/* -> :test.projectxero.net PONG test.projectxero.net :shrike.malkier.net */
}

static void m_burst(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;

	/* Ignore "BURST" at start of burst */
	if (parc != 1)
		return;

	s = server_find(me.actual);
	if (s != NULL)
		handle_eob(s);

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

static void m_privmsg(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], FALSE, parv[1]);
}

static void m_notice(sourceinfo_t *si, int parc, char *parv[])
{
	if (parc != 2)
		return;

	handle_message(si, parv[0], TRUE, parv[1]);
}

static void m_sjoin(sourceinfo_t *si, int parc, char *parv[])
{
	/*
	 *  -> :proteus.malkier.net SJOIN 1073516550 #shrike +tn :@sycobuny @+rakaur
	 *      also:
	 *  -> :nenolod_ SJOIN 1117334567 #chat
	 */

	channel_t *c;
	boolean_t keep_new_modes = TRUE;
	uint8_t userc;
	char *userv[256];
	uint8_t i;
	time_t ts;
	char *p;

	if (parc >= 4 && si->s != NULL)
	{
		/* :origin SJOIN ts chan modestr [key or limits] :users */
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (!c)
		{
			slog(LG_DEBUG, "m_sjoin(): new channel: %s", parv[1]);
			c = channel_add(parv[1], ts);
		}

		if (ts == 0 || c->ts == 0)
		{
			if (c->ts != 0)
				slog(LG_INFO, "m_sjoin(): server %s changing TS on %s from %ld to 0", si->s->name, c->name, (long)c->ts);
			c->ts = 0;
			hook_call_event("channel_tschange", c);
		}
		else if (ts < c->ts)
		{
			chanuser_t *cu;
			node_t *n;

			/* the TS changed.  a TS change requires the following things
			 * to be done to the channel:  reset all modes to nothing, remove
			 * all status modes on known users on the channel (including ours),
			 * and set the new TS.
			 * also clear all bans and the topic
			 */

			clear_simple_modes(c);
			chanban_clear(c);
			handle_topic_from(si, c, "", 0, "");

			LIST_FOREACH(n, c->members.head)
			{
				cu = (chanuser_t *)n->data;
				if (cu->user->server == me.me)
				{
					/* it's a service, reop */
					sts(":%s PART %s :Reop", cu->user->nick, c->name);
					sts(":%s SJOIN %ld %s + :@%s", me.name, ts, c->name, cu->user->nick);
					cu->modes = CMODE_OP;
				}
				else
					cu->modes = 0;
			}

			slog(LG_DEBUG, "m_sjoin(): TS changed for %s (%ld -> %ld)", c->name, c->ts, ts);

			c->ts = ts;
			hook_call_event("channel_tschange", c);
		}
		else if (ts > c->ts)
			keep_new_modes = FALSE;

		if (keep_new_modes)
			channel_mode(NULL, c, parc - 3, parv + 2);

		userc = sjtoken(parv[parc - 1], ' ', userv);

		if (keep_new_modes)
			for (i = 0; i < userc; i++)
				chanuser_add(c, userv[i]);
		else
			for (i = 0; i < userc; i++)
			{
				p = userv[i];
				while (*p == '@' || *p == '%' || *p == '+')
					p++;
				chanuser_add(c, p);
			}

		if (c->nummembers == 0 && !(c->modes & ircd->perm_mode))
			channel_delete(c->name);
	}
	else if (parc >= 2 && si->su != NULL)
	{
		c = channel_find(parv[1]);
		ts = atol(parv[0]);

		if (c == NULL || ts < c->ts)
		{
			/* just request a resynch, this will include
			 * the user joining -- jilles */
			slog(LG_DEBUG, "m_sjoin(): requesting resynch for %s",
					parv[1]);
			sts("RESYNCH %s", parv[1]);
			return;
		}

		chanuser_add(c, CLIENT_NAME(si->su));
	}
	else
	{
		slog(LG_DEBUG, "m_sjoin(): invalid source/parameters: origin %s parc %d",
				si->su != NULL ? si->su->nick : (si->s != NULL ? si->s->name : "<none>"), parc);
	}
}

static void m_part(sourceinfo_t *si, int parc, char *parv[])
{
	uint8_t chanc;
	char *chanv[256];
	int i;

	chanc = sjtoken(parv[0], ',', chanv);
	for (i = 0; i < chanc; i++)
	{
		slog(LG_DEBUG, "m_part(): user left channel: %s -> %s", si->su->nick, chanv[i]);

		chanuser_delete(channel_find(chanv[i]), si->su);
	}
}

static void m_nick(sourceinfo_t *si, int parc, char *parv[])
{
	server_t *s;
	user_t *u;
	struct in_addr ip;
	char ipstring[64];

	/* -> NICK jilles 1 1136143909 +oi ~jilles 192.168.1.5 jaguar.test 0 3232235781 :Jilles Tjoelker */
	if (parc == 10)
	{
		s = server_find(parv[6]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[6]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		ip.s_addr = ntohl(strtoul(parv[8], NULL, 10));
		ipstring[0] = '\0';
		if (!inet_ntop(AF_INET, &ip, ipstring, sizeof ipstring))
			ipstring[0] = '\0';
		u = user_add(parv[0], parv[4], parv[5], NULL, ipstring, NULL, parv[9], s, atoi(parv[2]));

		user_mode(u, parv[3]);

		/* Ok, we have the user ready to go.
		 * Here's the deal -- if the user's SVID is before
		 * the start time, and not 0, then check to see
		 * if it's a registered account or not.
		 *
		 * If it IS registered, deal with that accordingly,
		 * via handle_burstlogin(). --nenolod
		 */
		/* Changed to just check umode +r for now -- jilles */
		/* This is ok because this ircd clears +r on nick changes
		 * -- jilles */
		if (strchr(parv[3], 'r'))
			handle_burstlogin(u, parv[0]);

		handle_nickchange(u);
	}

	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		if (!si->su)
		{
			slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
			return;
		}

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

		/* fix up +r if necessary -- jilles */
		if (nicksvs.me != NULL && si->su->myuser != NULL && !(si->su->myuser->flags & MU_WAITAUTH) && irccasecmp(si->su->nick, parv[0]) && !irccasecmp(parv[0], si->su->myuser->name))
			/* changed nick to registered one, reset +r */
			sts(":%s SVSMODE %s +rd %ld", nicksvs.nick, parv[0], time(NULL));

		user_changenick(si->su, parv[0], atoi(parv[1]));

		handle_nickchange(si->su);
	}
	else
	{
		int i;
		slog(LG_DEBUG, "m_nick(): got NICK with wrong number of params");

		for (i = 0; i < parc; i++)
			slog(LG_DEBUG, "m_nick():   parv[%d] = %s", i, parv[i]);
	}
}

static void m_quit(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_quit(): user leaving: %s", si->su->nick);

	/* user_delete() takes care of removing channels and so forth */
	user_delete(si->su);
}

static void m_mode(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;

	if (*parv[0] == '#')
	{
		c = channel_find(parv[0]);
		if (c == NULL)
		{
			slog(LG_DEBUG, "m_mode(): unknown channel %s", parv[0]);
			return;
		}
		if (atol(parv[1]) > c->ts)
			return;
		channel_mode(NULL, channel_find(parv[0]), parc - 2, &parv[2]);
	}
	else
		user_mode(user_find(parv[0]), parv[1]);
}

static void m_kick(sourceinfo_t *si, int parc, char *parv[])
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

static void m_kill(sourceinfo_t *si, int parc, char *parv[])
{
	handle_kill(si, parv[0], parc > 1 ? parv[1] : "<No reason given>");
}

static void m_squit(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_squit(): server leaving: %s from %s", parv[0], parv[1]);
	server_delete(parv[0]);
}

static void m_server(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_DEBUG, "m_server(): new server: %s", parv[0]);
	server_add(parv[0], atoi(parv[1]), si->s ? si->s->name : me.name, NULL, parv[2]);

	if (cnt.server == 2)
		me.actual = sstrdup(parv[0]);
	else
	{
		/* elicit PONG for EOB detection; pinging uplink is
		 * already done elsewhere -- jilles
		 */
		sts(":%s PING %s %s", me.name, me.name, parv[0]);
	}

	me.recvsvr = TRUE;
}

static void m_stats(sourceinfo_t *si, int parc, char *parv[])
{
	handle_stats(si->su, parv[0][0]);
}

static void m_admin(sourceinfo_t *si, int parc, char *parv[])
{
	handle_admin(si->su);
}

static void m_version(sourceinfo_t *si, int parc, char *parv[])
{
	handle_version(si->su);
}

static void m_info(sourceinfo_t *si, int parc, char *parv[])
{
	handle_info(si->su);
}

static void m_whois(sourceinfo_t *si, int parc, char *parv[])
{
	handle_whois(si->su, parv[1]);
}

static void m_trace(sourceinfo_t *si, int parc, char *parv[])
{
	handle_trace(si->su, parv[0], parc >= 2 ? parv[1] : NULL);
}

static void m_join(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *u = si->su;
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

static void m_pass(sourceinfo_t *si, int parc, char *parv[])
{
	if (strcmp(curr_uplink->pass, parv[0]))
	{
		slog(LG_INFO, "m_pass(): password mismatch from uplink; aborting");
		runflags |= RF_SHUTDOWN;
	}
}

static void m_error(sourceinfo_t *si, int parc, char *parv[])
{
	slog(LG_INFO, "m_error(): error from server: %s", parv[0]);
}

static void m_motd(sourceinfo_t *si, int parc, char *parv[])
{
	handle_motd(si->su);
}

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &bahamut_server_login;
	introduce_nick = &bahamut_introduce_nick;
	quit_sts = &bahamut_quit_sts;
	wallops_sts = &bahamut_wallops_sts;
	join_sts = &bahamut_join_sts;
	chan_lowerts = &bahamut_chan_lowerts;
	kick = &bahamut_kick;
	msg = &bahamut_msg;
	notice_user_sts = &bahamut_notice_user_sts;
	notice_global_sts = &bahamut_notice_global_sts;
	notice_channel_sts = &bahamut_notice_channel_sts;
	wallchops = &bahamut_wallchops;
	numeric_sts = &bahamut_numeric_sts;
	skill = &bahamut_skill;
	part = &bahamut_part;
	kline_sts = &bahamut_kline_sts;
	unkline_sts = &bahamut_unkline_sts;
	topic_sts = &bahamut_topic_sts;
	mode_sts = &bahamut_mode_sts;
	ping_sts = &bahamut_ping_sts;
	ircd_on_login = &bahamut_on_login;
	ircd_on_logout = &bahamut_on_logout;
	jupe = &bahamut_jupe;
	fnc_sts = &bahamut_fnc_sts;
	invite_sts = &bahamut_invite_sts;
	holdnick_sts = &bahamut_holdnick_sts;

	mode_list = bahamut_mode_list;
	ignore_mode_list = bahamut_ignore_mode_list;
	status_mode_list = bahamut_status_mode_list;
	prefix_mode_list = bahamut_prefix_mode_list;

	ircd = &Bahamut;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
	pcommand_add("SJOIN", m_sjoin, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("PART", m_part, 1, MSRC_USER);
	pcommand_add("NICK", m_nick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("QUIT", m_quit, 1, MSRC_USER);
	pcommand_add("MODE", m_mode, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KICK", m_kick, 2, MSRC_USER | MSRC_SERVER);
	pcommand_add("KILL", m_kill, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SQUIT", m_squit, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("SERVER", m_server, 3, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("STATS", m_stats, 2, MSRC_USER);
	pcommand_add("ADMIN", m_admin, 1, MSRC_USER);
	pcommand_add("VERSION", m_version, 1, MSRC_USER);
	pcommand_add("INFO", m_info, 1, MSRC_USER);
	pcommand_add("WHOIS", m_whois, 2, MSRC_USER);
	pcommand_add("TRACE", m_trace, 1, MSRC_USER);
	pcommand_add("JOIN", m_join, 1, MSRC_USER);
	pcommand_add("PASS", m_pass, 1, MSRC_UNREG);
	pcommand_add("ERROR", m_error, 1, MSRC_UNREG | MSRC_SERVER);
	pcommand_add("TOPIC", m_topic, 4, MSRC_USER | MSRC_SERVER);
	pcommand_add("MOTD", m_motd, 1, MSRC_USER);
	pcommand_add("BURST", m_burst, 0, MSRC_SERVER);

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
