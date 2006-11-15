/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for bahamut-based ircd.
 *
 * $Id: sorcery.c 6861 2006-10-22 14:08:20Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/sorcery.h"

DECLARE_MODULE_V1("protocol/sorcery", TRUE, _modinit, NULL, "$Id: sorcery.c 6861 2006-10-22 14:08:20Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Sorcery = {
        "SorceryNet IRCd 1.3.1+",       /* IRCd name */
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
	PROTOCOL_SORCERY,		/* Protocol type */
	0,                              /* Permanent cmodes */
	"b",                            /* Ban-like cmodes */
	0,                              /* Except mchar */
	0                               /* Invex mchar */
};

struct cmode_ sorcery_mode_list[] = {
  { 'i', CMODE_INVITE   },
  { 'm', CMODE_MOD      },
  { 'n', CMODE_NOEXT    },
  { 'p', CMODE_PRIV     },
  { 's', CMODE_SEC      },
  { 't', CMODE_TOPIC    },
  { 'c', CMODE_NOCOLOR  },
  { 'R', CMODE_REGONLY  },
  { 'O', CMODE_OPERONLY },
  { '\0', 0 }
};

struct extmode sorcery_ignore_mode_list[] = {
  { '\0', 0 }
};

struct cmode_ sorcery_status_mode_list[] = {
  { 'o', CMODE_OP      },
  { 'v', CMODE_VOICE   },
  { '\0', 0 }
};

struct cmode_ sorcery_prefix_mode_list[] = {
  { '@', CMODE_OP      },
  { '+', CMODE_VOICE   },
  { '\0', 0 }
};

/* *INDENT-ON* */

/* login to our uplink */
static uint8_t sorcery_server_login(void)
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
static void sorcery_introduce_nick(user_t *u)
{
	sts("NICK %s 1 %ld %s %s %s :%s", u->nick, u->ts, u->user, u->host, me.name, u->gecos);
	sts(":%s MODE %s +%s", u->nick, u->nick, "io");
}

/* invite a user to a channel */
static void sorcery_invite_sts(user_t *sender, user_t *target, channel_t *channel)
{
	sts(":%s INVITE %s %s", sender->nick, target->nick, channel->name);
}

static void sorcery_quit_sts(user_t *u, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s QUIT :%s", u->nick, reason);
}

/* WALLOPS wrapper */
static void sorcery_wallops_sts(const char *text)
{
	sts(":%s GLOBOPS :%s", me.name, text);
}

/* join a channel */
static void sorcery_join_sts(channel_t *c, user_t *u, boolean_t isnew, char *modes)
{
	if (isnew)
	{
		sts(":%s JOIN %s", u->nick, c->name);
		if (modes[0] && modes[1])
			sts(":%s MODE %s %s", u->nick, c->name, modes);
	}
	else
	{
		sts(":%s JOIN %s", u->nick, c->name);
	}
	sts(":%s MODE %s +o %s %ld", u->nick, c->name, u->nick, c->ts);
}

/* kicks a user from a channel */
static void sorcery_kick(char *from, char *channel, char *to, char *reason)
{
	channel_t *chan = channel_find(channel);
	user_t *user = user_find(to);

	if (!chan || !user)
		return;

	sts(":%s KICK %s %s :%s", from, channel, to, reason);

	chanuser_delete(chan, user);
}

/* PRIVMSG wrapper */
static void sorcery_msg(char *from, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s PRIVMSG %s :%s", from, target, buf);
}

/* NOTICE wrapper */
static void sorcery_notice_user_sts(user_t *from, user_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->nick, text);
}

static void sorcery_notice_global_sts(user_t *from, const char *mask, const char *text)
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

static void sorcery_notice_channel_sts(user_t *from, channel_t *target, const char *text)
{
	sts(":%s NOTICE %s :%s", from ? from->nick : me.name, target->name, text);
}

static void sorcery_numeric_sts(char *from, int numeric, char *target, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s %d %s %s", from, numeric, target, buf);
}

/* KILL wrapper */
static void sorcery_skill(char *from, char *nick, char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZE];

	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZE, fmt, ap);
	va_end(ap);

	sts(":%s KILL %s :%s!%s!%s (%s)", from, nick, from, from, from, buf);
}

/* PART wrapper */
static void sorcery_part(char *chan, char *nick)
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
static void sorcery_kline_sts(char *server, char *user, char *host, long duration, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s AKILL %s %s %ld %s %ld :%s", me.name, host, user, duration, opersvs.nick, time(NULL), reason);
}

/* server-to-server UNKLINE wrapper */
static void sorcery_unkline_sts(char *server, char *user, char *host)
{
	if (!me.connected)
		return;

	sts(":%s RAKILL %s %s", me.name, host, user);
}

/* topic wrapper */
static void sorcery_topic_sts(char *channel, char *setter, time_t ts, char *topic)
{
	if (!me.connected)
		return;

	sts(":%s TOPIC %s %s %ld :%s", chansvs.nick, channel, setter, ts, topic);
}

/* mode wrapper */
static void sorcery_mode_sts(char *sender, char *target, char *modes)
{
	if (!me.connected)
		return;

	sts(":%s MODE %s %s", sender, target, modes);
}

/* ping wrapper */
static void sorcery_ping_sts(void)
{
	if (!me.connected)
		return;

	sts("PING :%s", me.name);
}

/* protocol-specific stuff to do on login */
static void sorcery_on_login(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return;

	/* nothing to do here */
}

/* protocol-specific stuff to do on login */
static boolean_t sorcery_on_logout(char *origin, char *user, char *wantedhost)
{
	if (!me.connected)
		return FALSE;

	/* nothing to do here */
	return FALSE;
}

static void sorcery_jupe(char *server, char *reason)
{
	if (!me.connected)
		return;

	sts(":%s SQUIT %s :%s", opersvs.nick, server, reason);
	sts(":%s SERVER %s 2 :%s", me.name, server, reason);
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
	handle_eob(s);

	if (irccasecmp(me.actual, parv[0]))
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
	user_t *u;
	server_t *s;

	if (parc == 7)
	{
		s = server_find(parv[5]);
		if (!s)
		{
			slog(LG_DEBUG, "m_nick(): new user on nonexistant server: %s", parv[5]);
			return;
		}

		slog(LG_DEBUG, "m_nick(): new user on `%s': %s", s->name, parv[0]);

		u = user_add(parv[0], parv[3], parv[4], NULL, NULL, NULL, parv[6], s, atoi(parv[2]));

		handle_nickchange(u);
	}

	/* if it's only 2 then it's a nickname change */
	else if (parc == 2)
	{
		node_t *n;

                if (!si->su)
                {       
                        slog(LG_DEBUG, "m_nick(): server trying to change nick: %s", si->s != NULL ? si->s->name : "<none>");
                        return;
                }

		slog(LG_DEBUG, "m_nick(): nickname change from `%s': %s", si->su->nick, parv[0]);

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
	if (*parv[0] == '#')
		channel_mode(NULL, channel_find(parv[0]), parc - 1, &parv[1]);
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
	chanuser_t *cu;
	node_t *n, *tn;
	uint8_t chanc;
	char *chanv[256];
	int i;

	/* JOIN 0 is really a part from all channels */
	if (parv[0][0] == '0')
	{
		LIST_FOREACH_SAFE(n, tn, si->su->channels.head)
		{
			cu = (chanuser_t *) n->data;
			chanuser_delete(cu->chan, si->su);
		}
	}
	else
	{
		chanc = sjtoken(parv[0], ',', chanv);
		for (i = 0; i < chanc; i++)
		{
			channel_t *c = channel_find(chanv[i]);

			if (!c)
			{
				slog(LG_DEBUG, "m_join(): new channel: %s", parv[0]);
				c = channel_add(chanv[i], CURRTIME);
			}
			chanuser_add(c, si->su->nick);
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

void _modinit(module_t * m)
{
	/* Symbol relocation voodoo. */
	server_login = &sorcery_server_login;
	introduce_nick = &sorcery_introduce_nick;
	quit_sts = &sorcery_quit_sts;
	wallops_sts = &sorcery_wallops_sts;
	join_sts = &sorcery_join_sts;
	kick = &sorcery_kick;
	msg = &sorcery_msg;
	notice_user_sts = &sorcery_notice_user_sts;
	notice_global_sts = &sorcery_notice_global_sts;
	notice_channel_sts = &sorcery_notice_channel_sts;
	numeric_sts = &sorcery_numeric_sts;
	skill = &sorcery_skill;
	part = &sorcery_part;
	kline_sts = &sorcery_kline_sts;
	unkline_sts = &sorcery_unkline_sts;
	topic_sts = &sorcery_topic_sts;
	mode_sts = &sorcery_mode_sts;
	ping_sts = &sorcery_ping_sts;
	ircd_on_login = &sorcery_on_login;
	ircd_on_logout = &sorcery_on_logout;
	jupe = &sorcery_jupe;
	invite_sts = &sorcery_invite_sts;

	mode_list = sorcery_mode_list;
	ignore_mode_list = sorcery_ignore_mode_list;
	status_mode_list = sorcery_status_mode_list;
	prefix_mode_list = sorcery_prefix_mode_list;

	ircd = &Sorcery;

	pcommand_add("PING", m_ping, 1, MSRC_USER | MSRC_SERVER);
	pcommand_add("PONG", m_pong, 1, MSRC_SERVER);
	pcommand_add("PRIVMSG", m_privmsg, 2, MSRC_USER);
	pcommand_add("NOTICE", m_notice, 2, MSRC_UNREG | MSRC_USER | MSRC_SERVER);
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

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = TRUE;
}
