/*
 * Copyright (c) 2005 Atheme Development Group.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: atheme.c 3585 2005-11-06 21:52:51Z alambert $
 */

#include "atheme.h"

chansvs_t chansvs;
globsvs_t globsvs;
opersvs_t opersvs;
memosvs_t memosvs;
helpsvs_t helpsvs;
nicksvs_t nicksvs;
usersvs_t usersvs;

me_t me;
cnt_t cnt;

extern char **environ;
char *config_file;
boolean_t cold_start = FALSE;

/* *INDENT-OFF* */
static void print_help(void)
{
	printf("usage: atheme [-c config] [-dhnv]\n\n"
	       "-c <file>    Specify the config file\n"
	       "-d           Start in debugging mode\n"
	       "-h           Print this message and exit\n"
	       "-n           Don't fork into the background (log screen + log file)\n"
	       "-v           Print version information and exit\n");
}

static void print_version(void)
{
	printf("Atheme IRC Services (atheme-%s.%s)\n\n"
	       "Copyright (c) 2005 Atheme Development Group\n"
	       "Rights to this code are documented in doc/LICENSE.\n", version, generation);
}
/* *INDENT-ON* */

int main(int argc, char *argv[])
{
	boolean_t have_conf = FALSE;
	char buf[32];
	int i, pid, r;
	FILE *restart_file, *pid_file;
#ifndef _WIN32
	struct rlimit rlim;
#endif
	curr_uplink = NULL;

	/* change to our local directory */
	if (chdir(PREFIX) < 0)
	{
		perror(PREFIX);
		return 20;
	}

#ifndef _WIN32
	/* it appears certian systems *ahem*linux*ahem*
	 * don't dump cores by default, so we do this here.
	 */
	if (!getrlimit(RLIMIT_CORE, &rlim))
	{
		rlim.rlim_cur = rlim.rlim_max;
		setrlimit(RLIMIT_CORE, &rlim);
	}
#endif
	
	/* do command-line options */
	while ((r = getopt(argc, argv, "c:dhnv")) != -1)
	{
		switch (r)
		{
		  case 'c':
			  config_file = sstrdup(optarg);
			  have_conf = TRUE;
			  break;
		  case 'd':
			  me.loglevel |= LG_DEBUG;
			  break;
		  case 'h':
			  print_help();
			  exit(EXIT_SUCCESS);
			  break;
		  case 'n':
			  runflags |= RF_LIVE;
			  break;
		  case 'v':
			  print_version();
			  exit(EXIT_SUCCESS);
			  break;
		  default:
			  printf("usage: atheme [-c conf] [-dhnv]\n");
			  exit(EXIT_SUCCESS);
			  break;
		}
	}

	if (have_conf == FALSE)
		config_file = sstrdup("etc/atheme.conf");

	cold_start = TRUE;

	runflags |= RF_STARTING;

	me.start = time(NULL);
	CURRTIME = me.start;
	me.execname = argv[0];

	/* set signal handlers */
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGFPE, sighandler);
	signal(SIGILL, sighandler);
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, sighandler);
	signal(SIGHUP, sighandler);
	signal(SIGTRAP, sighandler);
	signal(SIGIOT, sighandler);
	signal(SIGALRM, SIG_IGN);
	signal(SIGUSR2, sighandler);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGWINCH, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGUSR1, sighandler);
#endif

	/* open log */
	log_open();

	/* since me.loglevel isn't there until after the
	 * config routines run, we set the default here
	 */
	me.loglevel |= LG_ERROR;

	printf("atheme: version atheme-%s\n", version);

	/* check for pid file */
	if ((pid_file = fopen("var/atheme.pid", "r")))
	{
		if (fgets(buf, 32, pid_file))
		{
			pid = atoi(buf);

			if (!kill(pid, 0))
			{
				fprintf(stderr, "atheme: daemon is already running\n");
				exit(EXIT_FAILURE);
			}
		}

		fclose(pid_file);
	}

#if HAVE_UMASK
	/* file creation mask */
	umask(077);
#endif

	libclaro_init(slog);

	init_nodes();
	init_newconf();
	servtree_init();
	init_ircpacket();

	modules_init();
	pcommand_init();

	conf_init();
	conf_parse();

	authcookie_init();

	if (!pmodule_loaded)
	{
		fprintf(stderr, "atheme: no protocol modules loaded, see your configuration file.\n");
		exit(EXIT_FAILURE);
	}

	if (!backend_loaded)
	{
		fprintf(stderr, "atheme: no backend modules loaded, see your configuration file.\n");
		exit(EXIT_FAILURE);
	}

	/* check our config file */
	if (!conf_check())
		exit(EXIT_FAILURE);

	/* we've done the critical startup steps now */
	cold_start = FALSE;

	/* load our db */
	if (db_load)
		db_load();
	else
	{
		/* XXX: We should have bailed by now! --nenolod */
		fprintf(stderr, "atheme: no backend modules loaded, see your configuration file.\n");
		exit(EXIT_FAILURE);
	}

#ifndef _WIN32
	/* fork into the background */
	if (!(runflags & RF_LIVE))
	{
		if ((i = fork()) < 0)
		{
			fprintf(stderr, "atheme: can't fork into the background\n");
			exit(EXIT_FAILURE);
		}

		/* parent */
		else if (i != 0)
		{
			printf("atheme: pid %d\n", i);
			printf("atheme: running in background mode from %s\n", PREFIX);
			exit(EXIT_SUCCESS);
		}

		/* parent is gone, just us now */
		if (setpgid(0, 0) < 0)
		{
			fprintf(stderr, "atheme: unable to set process group\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		printf("atheme: pid %d\n", getpid());
		printf("atheme: running in foreground mode from %s\n", PREFIX);
	}
#else
	printf("atheme: running in foreground mode from %s\n", PREFIX);
#endif

#ifndef _WIN32
	/* write pid */
	if ((pid_file = fopen("var/atheme.pid", "w")))
	{
		fprintf(pid_file, "%d\n", getpid());
		fclose(pid_file);
	}
	else
	{
		fprintf(stderr, "atheme: unable to write pid file\n");
		exit(EXIT_FAILURE);
	}
#endif

	/* no longer starting */
	runflags &= ~RF_STARTING;

	/* we probably have a few open already... */
	me.maxfd = 3;

	/* DB commit interval is configurable */
	event_add("db_save", db_save, NULL, config_options.commit_interval);

	/* check expires every hour */
	event_add("expire_check", expire_check, NULL, 3600);

	/* check kline expires every minute */
	event_add("kline_expire", kline_expire, NULL, 60);

	/* check authcookie expires every ten minutes */
	event_add("authcookie_expire", authcookie_expire, NULL, 600);

	uplink_connect();
	me.connected = FALSE;

	/* main loop */
	io_loop();

	/* we're shutting down */
	db_save(NULL);
	if (chansvs.me != NULL && chansvs.me->me != NULL)
		quit_sts(chansvs.me->me, "shutting down");

	remove("var/atheme.pid");
	sendq_flush(curr_uplink->conn);
	connection_close(curr_uplink->conn);

	me.connected = FALSE;

	/* should we restart? */
	if (runflags & RF_RESTART)
	{
		slog(LG_INFO, "main(): restarting in %d seconds", me.restarttime);

#ifndef _WIN32
		execve("bin/atheme", argv, environ);
#endif
	}

	slog(LG_INFO, "main(): shutting down: io_loop() exited");

	fclose(log_file);

	return 0;
}
