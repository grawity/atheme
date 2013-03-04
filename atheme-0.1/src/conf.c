/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the routines that deal with the configuration.
 *
 * $Id: conf.c 317 2005-06-02 17:52:21Z nenolod $
 */

#include "atheme.h"

#define PARAM_ERROR(ce) { slog(LG_INFO, "%s:%i: no parameter for " \
                          "configuration option: %s", \
                          (ce)->ce_fileptr->cf_filename, \
                          (ce)->ce_varlinenum, (ce)->ce_varname); \
  return 1; }

static int c_serverinfo(CONFIGENTRY *);
static int c_cservice(CONFIGENTRY *);
static int c_gservice(CONFIGENTRY *);
static int c_oservice(CONFIGENTRY *);
static int c_general(CONFIGENTRY *);
static int c_nickserv(CONFIGENTRY *);

static int c_si_name(CONFIGENTRY *);
static int c_si_desc(CONFIGENTRY *);
static int c_si_uplink(CONFIGENTRY *);
static int c_si_port(CONFIGENTRY *);
static int c_si_pass(CONFIGENTRY *);
static int c_si_vhost(CONFIGENTRY *);
static int c_si_recontime(CONFIGENTRY *);
static int c_si_restarttime(CONFIGENTRY *);
static int c_si_expire(CONFIGENTRY *);
static int c_si_netname(CONFIGENTRY *);
static int c_si_adminname(CONFIGENTRY *);
static int c_si_adminemail(CONFIGENTRY *);
static int c_si_mta(CONFIGENTRY *);
static int c_si_loglevel(CONFIGENTRY *);
static int c_si_maxusers(CONFIGENTRY *);
static int c_si_maxchans(CONFIGENTRY *);
static int c_si_auth(CONFIGENTRY *);
static int c_si_casemapping(CONFIGENTRY *);

/* CService client information. */
static int c_ci_nick(CONFIGENTRY *);
static int c_ci_user(CONFIGENTRY *);
static int c_ci_host(CONFIGENTRY *);
static int c_ci_real(CONFIGENTRY *);

/* GService client information. */
static int c_gl_nick(CONFIGENTRY *);
static int c_gl_user(CONFIGENTRY *);
static int c_gl_host(CONFIGENTRY *);
static int c_gl_real(CONFIGENTRY *);

/* OService client information. */
static int c_oi_nick(CONFIGENTRY *);
static int c_oi_user(CONFIGENTRY *);
static int c_oi_host(CONFIGENTRY *);
static int c_oi_real(CONFIGENTRY *);

/* NickServ client information. */
static int c_ni_nick(CONFIGENTRY *);
static int c_ni_user(CONFIGENTRY *);
static int c_ni_host(CONFIGENTRY *);
static int c_ni_real(CONFIGENTRY *);
static int c_ni_enable(CONFIGENTRY *);

static int c_gi_chan(CONFIGENTRY *);
static int c_gi_silent(CONFIGENTRY *);
static int c_gi_join_chans(CONFIGENTRY *);
static int c_gi_leave_chans(CONFIGENTRY *);
static int c_gi_uflags(CONFIGENTRY *);
static int c_gi_cflags(CONFIGENTRY *);
static int c_gi_raw(CONFIGENTRY *);
static int c_gi_flood_msgs(CONFIGENTRY *);
static int c_gi_flood_time(CONFIGENTRY *);
static int c_gi_kline_time(CONFIGENTRY *);
static int c_gi_sras(CONFIGENTRY *);

struct ConfTable
{
	char *name;
	int rehashable;
	int (*handler) (CONFIGENTRY *);
};

/* *INDENT-OFF* */

static struct Token uflags[] = {
  { "HOLD",     MU_HOLD     },
  { "NEVEROP",  MU_NEVEROP  },
  { "NOOP",     MU_NOOP     },
  { "HIDEMAIL", MU_HIDEMAIL },
  { "NONE",     0           },
  { NULL, 0 }
};

static struct Token cflags[] = {
  { "HOLD",    MC_HOLD    },
  { "NEVEROP", MC_NEVEROP },
  { "SECURE",  MC_SECURE  },
  { "VERBOSE", MC_VERBOSE },
  { "NONE",    0          },
  { NULL, 0 }
};

static struct ConfTable conf_root_table[] = {
  { "SERVERINFO", 1, c_serverinfo },
  { "CHANSERV", 1, c_cservice },
  { "CSERVICE", 1, c_cservice },
  { "GLOBAL",   1, c_gservice },
  { "GSERVICE", 1, c_gservice },
  { "OPERSERV", 1, c_oservice },
  { "OSERVICE", 1, c_oservice },
  { "NICKSERV", 1, c_nickserv },
  { "GENERAL", 1, c_general },
  { NULL, 0, NULL }
};

static struct ConfTable conf_si_table[] = {
  { "NAME",        0, c_si_name        },
  { "DESC",        0, c_si_desc        },
  { "UPLINK",      0, c_si_uplink      },
  { "PORT",        0, c_si_port        },
  { "PASS",        1, c_si_pass        },
  { "VHOST",       0, c_si_vhost       },
  { "RECONTIME",   1, c_si_recontime   },
  { "RESTARTTIME", 1, c_si_restarttime },
  { "EXPIRE",      1, c_si_expire      },
  { "NETNAME",     1, c_si_netname     },
  { "ADMINNAME",   1, c_si_adminname   },
  { "ADMINEMAIL",  1, c_si_adminemail  },
  { "MTA",         1, c_si_mta         },
  { "LOGLEVEL",    1, c_si_loglevel    },
  { "MAXUSERS",    1, c_si_maxusers    },
  { "MAXCHANS",    1, c_si_maxchans    },
  { "AUTH",        1, c_si_auth        },
  { "CASEMAPPING", 0, c_si_casemapping },
  { NULL, 0, NULL }
};

static struct ConfTable conf_ci_table[] = {
  { "NICK",        1, c_ci_nick        },
  { "USER",        0, c_ci_user        },
  { "HOST",        0, c_ci_host        },
  { "REAL",        0, c_ci_real        },
  { NULL, 0, NULL }
};

static struct ConfTable conf_gl_table[] = {
  { "NICK",        1, c_gl_nick        },
  { "USER",        0, c_gl_user        },
  { "HOST",        0, c_gl_host        },
  { "REAL",        0, c_gl_real        },
  { NULL, 0, NULL }
};

static struct ConfTable conf_oi_table[] = {
  { "NICK",        1, c_oi_nick        },
  { "USER",        0, c_oi_user        },
  { "HOST",        0, c_oi_host        },
  { "REAL",        0, c_oi_real        },
  { NULL, 0, NULL }
};

static struct ConfTable conf_ni_table[] = {
  { "NICK",        1, c_ni_nick        },
  { "USER",        0, c_ni_user        },
  { "HOST",        0, c_ni_host        },
  { "REAL",        0, c_ni_real        },
  { "ENABLE",      0, c_ni_enable      },
  { NULL, 0, NULL }
};

static struct ConfTable conf_gi_table[] = {
  { "CHAN",        1, c_gi_chan        },
  { "SILENT",      1, c_gi_silent      },
  { "JOIN_CHANS",  1, c_gi_join_chans  },
  { "LEAVE_CHANS", 1, c_gi_leave_chans },
  { "UFLAGS",      1, c_gi_uflags      },
  { "CFLAGS",      1, c_gi_cflags      },
  { "RAW",         1, c_gi_raw         },
  { "FLOOD_MSGS",  1, c_gi_flood_msgs  },
  { "FLOOD_TIME",  1, c_gi_flood_time  },
  { "KLINE_TIME",  1, c_gi_kline_time  },
  { "SRAS",        1, c_gi_sras        },
  { NULL, 0, NULL }
};

/* *INDENT-ON* */

void conf_parse(void)
{
	CONFIGFILE *cfptr, *cfp;
	CONFIGENTRY *ce;
	struct ConfTable *ct = NULL;

	cfptr = cfp = config_load(config_file);

	if (cfp == NULL)
	{
		slog(LG_INFO, "conf_parse(): unable to open configuration file: %s", strerror(errno));

		exit(EXIT_FAILURE);
	}

	for (; cfptr; cfptr = cfptr->cf_next)
	{
		for (ce = cfptr->cf_entries; ce; ce = ce->ce_next)
		{
			for (ct = conf_root_table; ct->name; ct++)
			{
				if (!strcasecmp(ct->name, ce->ce_varname))
				{
					ct->handler(ce);
					break;
				}
			}
			if (ct->name == NULL)
			{
				slog(LG_INFO, "%s:%d: invalid configuration option: %s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
			}
		}
	}

	config_free(cfp);
}

void conf_init(void)
{
	if (me.pass)
		free(me.pass);
	if (me.netname)
		free(me.netname);
	if (me.adminname)
		free(me.adminname);
	if (me.adminemail)
		free(me.adminemail);
	if (me.mta)
		free(me.mta);
	if (chansvs.nick)
		free(chansvs.nick);
	if (config_options.chan)
		free(config_options.chan);
	if (config_options.global)
		free(config_options.global);

	me.pass = me.netname = me.adminname = me.adminemail = me.mta = chansvs.nick = config_options.chan = config_options.global = NULL;

	me.recontime = me.restarttime = me.expire = me.maxusers = me.maxchans = config_options.flood_msgs = config_options.flood_time = config_options.kline_time = 0;

	/* we don't reset loglevel because too much stuff uses it */
	config_options.defuflags = config_options.defcflags = 0x00000000;

	config_options.silent = config_options.join_chans = config_options.leave_chans = config_options.raw = FALSE;

	me.auth = AUTH_NONE;

	if (!(runflags & RF_REHASHING))
	{
		if (me.name)
			free(me.name);
		if (me.desc)
			free(me.desc);
		if (me.uplink)
			free(me.uplink);
		if (me.vhost)
			free(me.vhost);
		if (chansvs.user)
			free(chansvs.user);
		if (chansvs.host)
			free(chansvs.host);
		if (chansvs.real)
			free(chansvs.real);

		me.name = me.desc = me.uplink = me.vhost = chansvs.user = chansvs.host = chansvs.real = NULL;

		me.port = 0;

		set_match_mapping(MATCH_RFC1459);	/* default to RFC compliancy */
	}
}

static int subblock_handler(CONFIGENTRY *ce, struct ConfTable *table)
{
	struct ConfTable *ct = NULL;

	for (ce = ce->ce_entries; ce; ce = ce->ce_next)
	{
		for (ct = table; ct->name; ct++)
		{
			if (!strcasecmp(ct->name, ce->ce_varname))
			{
				ct->handler(ce);
				break;
			}
		}
		if (ct->name == NULL)
		{
			slog(LG_INFO, "%s:%d: invalid configuration option: %s", ce->ce_fileptr->cf_filename, ce->ce_varlinenum, ce->ce_varname);
		}
	}
	return 0;
}

static int c_serverinfo(CONFIGENTRY *ce)
{
	subblock_handler(ce, conf_si_table);
	return 0;
}

static int c_cservice(CONFIGENTRY *ce)
{
	subblock_handler(ce, conf_ci_table);
	return 0;
}

static int c_gservice(CONFIGENTRY *ce)
{
	subblock_handler(ce, conf_gl_table);
	return 0;
}

static int c_oservice(CONFIGENTRY *ce)
{
	subblock_handler(ce, conf_oi_table);
	return 0;
}

static int c_nickserv(CONFIGENTRY *ce)
{
	subblock_handler(ce, conf_ni_table);
	return 0;
}

static int c_general(CONFIGENTRY *ce)
{
	subblock_handler(ce, conf_gi_table);
	return 0;
}

static int c_si_name(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.name = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_desc(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.desc = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_uplink(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.uplink = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_port(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.port = ce->ce_vardatanum;

	return 0;
}

static int c_si_pass(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.pass = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_vhost(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.vhost = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_recontime(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.recontime = ce->ce_vardatanum;

	return 0;
}

static int c_si_restarttime(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.restarttime = ce->ce_vardatanum;

	return 0;
}

static int c_si_expire(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.expire = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_si_netname(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.netname = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_adminname(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.adminname = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_adminemail(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.adminemail = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_mta(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.mta = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_si_loglevel(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("DEBUG", ce->ce_vardata))
		me.loglevel |= LG_DEBUG;

	else if (!strcasecmp("ERROR", ce->ce_vardata))
		me.loglevel |= LG_ERROR;

	else if (!strcasecmp("INFO", ce->ce_vardata))
		me.loglevel |= LG_INFO;

	else if (!strcasecmp("NONE", ce->ce_vardata))
		me.loglevel |= LG_NONE;

	else
		me.loglevel |= LG_ERROR;

	return 0;
}

static int c_si_maxusers(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxusers = ce->ce_vardatanum;

	return 0;

}

static int c_si_maxchans(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	me.maxchans = ce->ce_vardatanum;

	return 0;
}

static int c_si_auth(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("EMAIL", ce->ce_vardata))
		me.auth = AUTH_EMAIL;

	else
		me.auth = AUTH_NONE;

	return 0;
}

static int c_si_casemapping(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	if (!strcasecmp("ASCII", ce->ce_vardata))
		set_match_mapping(MATCH_ASCII);

	else
		set_match_mapping(MATCH_RFC1459);

	return 0;
}

static int c_ci_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ci_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	chansvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gi_chan(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.chan = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gi_silent(CONFIGENTRY *ce)
{
	config_options.silent = TRUE;
	return 0;
}

static int c_gi_join_chans(CONFIGENTRY *ce)
{
	config_options.join_chans = TRUE;
	return 0;
}

static int c_gi_leave_chans(CONFIGENTRY *ce)
{
	config_options.leave_chans = TRUE;
	return 0;
}

static int c_gi_uflags(CONFIGENTRY *ce)
{
	CONFIGENTRY *flce;

	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
	{
		int val;

		val = token_to_value(uflags, flce->ce_varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			config_options.defuflags |= val;

		else
		{
			slog(LG_INFO, "%s:%d: unknown flag: %s", flce->ce_fileptr->cf_filename, flce->ce_varlinenum, flce->ce_varname);
		}
	}

	return 0;
}

static int c_gi_cflags(CONFIGENTRY *ce)
{
	CONFIGENTRY *flce;

	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
	{
		int val;

		val = token_to_value(cflags, flce->ce_varname);

		if ((val != TOKEN_UNMATCHED) && (val != TOKEN_ERROR))
			config_options.defcflags |= val;

		else
		{
			slog(LG_INFO, "%s:%d: unknown flag: %s", flce->ce_fileptr->cf_filename, flce->ce_varlinenum, flce->ce_varname);
		}
	}

	return 0;
}

static int c_gi_raw(CONFIGENTRY *ce)
{
	config_options.raw = TRUE;
	return 0;
}

static int c_gi_flood_msgs(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.flood_msgs = ce->ce_vardatanum;

	return 0;
}

static int c_gi_flood_time(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.flood_time = ce->ce_vardatanum;

	return 0;
}

static int c_gi_kline_time(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	config_options.kline_time = (ce->ce_vardatanum * 60 * 60 * 24);

	return 0;
}

static int c_oi_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_oi_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_oi_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_oi_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	opersvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	nicksvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_ni_enable(CONFIGENTRY *ce)
{
	nicksvs.enable = TRUE;
	return 0;
}

static int c_gl_nick(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.nick = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gl_user(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.user = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gl_host(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.host = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gl_real(CONFIGENTRY *ce)
{
	if (ce->ce_vardata == NULL)
		PARAM_ERROR(ce);

	globsvs.real = sstrdup(ce->ce_vardata);

	return 0;
}

static int c_gi_sras(CONFIGENTRY *ce)
{
	CONFIGENTRY *flce;

	for (flce = ce->ce_entries; flce; flce = flce->ce_next)
		sra_add(flce->ce_varname);

	return 0;
}

static void copy_me(struct me *src, struct me *dst)
{
	dst->pass = sstrdup(src->pass);
	dst->recontime = src->recontime;
	dst->restarttime = src->restarttime;
	dst->expire = src->expire;
	dst->netname = sstrdup(src->netname);
	dst->adminname = sstrdup(src->adminname);
	dst->adminemail = sstrdup(src->adminemail);
	dst->mta = sstrdup(src->mta);
	dst->loglevel = src->loglevel;
	dst->maxusers = src->maxusers;
	dst->maxchans = src->maxchans;
	dst->auth = src->auth;
}

static void free_cstructs(struct me *mesrc, struct chansvs *svssrc)
{
	free(mesrc->pass);
	free(mesrc->netname);
	free(mesrc->adminname);
	free(mesrc->adminemail);
	free(mesrc->mta);

	free(svssrc->nick);
}

boolean_t conf_rehash(void)
{
	struct me *hold_me = scalloc(sizeof(struct me), 1);	/* and keep_me_warm; */
	sra_t *sra;
	node_t *n, *tn;

	/* we're rehashing */
	slog(LG_INFO, "conf_rehash(): rehashing");
	runflags |= RF_REHASHING;

	copy_me(&me, hold_me);

	/* reset everything */
	conf_init();

	LIST_FOREACH_SAFE(n, tn, sralist.head)
	{
		sra = (sra_t *)n->data;

		sra_delete(sra->myuser);
	}

	/* now reload */
	conf_parse();

	part(config_options.chan, chansvs.nick);

	/* now recheck */
	if (!conf_check())
	{
		slog(LG_INFO, "conf_rehash(): conf file was malformed, aborting rehash");

		/* freeing the new conf strings */
		free_cstructs(&me, &chansvs);

		/* return everything to the way it was before */
		copy_me(hold_me, &me);

		free(hold_me);

		return FALSE;
	}

	if (config_options.chan)
		join(config_options.chan, chansvs.nick);

	runflags &= ~RF_REHASHING;

	return TRUE;
}

boolean_t conf_check(void)
{
	if (!me.name)
	{
		slog(LG_INFO, "conf_check(): no `name' set in %s", config_file);
		return FALSE;
	}

	if (!me.desc)
		me.desc = sstrdup("Shrike IRC Services");

	if (!me.uplink)
	{
		slog(LG_INFO, "conf_check(): no `uplink' set in %s", config_file);
		return FALSE;
	}

	if (!me.port)
	{
		slog(LG_INFO, "conf_check(): no `port' set in %s; defaulting to 6667", config_file);
		me.port = 6667;
	}

	if (!me.pass)
	{
		slog(LG_INFO, "conf_check(): no `pass' set in %s", config_file);
		return FALSE;
	}

	if ((!me.recontime) || (me.recontime < 10))
	{
		slog(LG_INFO, "conf_check(): invalid `recontime' set in %s; " "defaulting to 10", config_file);
		me.recontime = 10;
	}

	if ((!me.restarttime) || (me.restarttime < 10))
	{
		slog(LG_INFO, "conf_check(): invalid `restarttime' set in %s; " "defaulting to 10", config_file);
		me.restarttime = 10;
	}

	if (!me.netname)
	{
		slog(LG_INFO, "conf_check(): no `netname' set in %s", config_file);
		return FALSE;
	}

	if (!me.adminname)
	{
		slog(LG_INFO, "conf_check(): no `adminname' set in %s", config_file);
		return FALSE;
	}

	if (!me.adminemail)
	{
		slog(LG_INFO, "conf_check(): no `adminemail' set in %s", config_file);
		return FALSE;
	}

	if (!me.mta && me.auth == AUTH_EMAIL)
	{
		slog(LG_INFO, "conf_check(): no `mta' set in %s (but `auth' is email)", config_file);
		return FALSE;
	}

	if (!me.maxusers)
	{
		slog(LG_INFO, "conf_check(): no `maxusers' set in %s; " "defaulting to 5", config_file);
		me.maxusers = 5;
	}

	if (!me.maxchans)
	{
		slog(LG_INFO, "conf_check(): no `maxchans' set in %s; " "defaulting to 5", config_file);
		me.maxchans = 5;
	}

	if (me.auth != 0 && me.auth != 1)
	{
		slog(LG_INFO, "conf_check(): no `auth' set in %s; " "defaulting to NONE", config_file);
		me.auth = AUTH_NONE;
	}

	if (!chansvs.nick || !chansvs.user || !chansvs.host || !chansvs.real)
	{
		slog(LG_INFO, "conf_check(): invalid clientinfo{} block in %s", config_file);
		return FALSE;
	}

	if ((strchr(chansvs.user, ' ')) || (strlen(chansvs.user) > 10))
	{
		slog(LG_INFO, "conf_check(): invalid `clientinfo::user' in %s", config_file);
		return FALSE;
	}

	if (config_options.flood_msgs && !config_options.flood_time)
		config_options.flood_time = 10;

	return TRUE;
}
