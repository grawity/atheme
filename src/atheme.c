/*
 * Copyright (c) 2005 Atheme Development Group.
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains the main() routine.
 *
 * $Id: atheme.c 908 2005-07-17 04:00:28Z w00t $
 */

#include "atheme.h"

extern char **environ;
char *config_file;

/* *INDENT-OFF* */
static void print_help(void)
{
printf(
"usage: atheme [-c config] [-dhnv]\n\n"

"-c <file>    Specify the config file\n"
"-d           Start in debugging mode\n"
"-h           Print this message and exit\n"
"-n           Don't fork into the background (log screen + log file)\n"
"-v           Print version information and exit\n");
}

static void print_version(void)
{
printf(
"Atheme IRC Services (atheme-%s.%s)\n\n"
"Copyright (c) 2005 Atheme Development Group\n"
"Rights to this code are documented in doc/LICENSE.\n", version, generation
);
}
/* *INDENT-ON* */

int main(int argc, char *argv[])
{
	boolean_t have_conf = FALSE;
	char buf[32];
	int i, pid, r;
	FILE *restart_file, *pid_file;
	struct rlimit rlim;
	curr_uplink = NULL;

	/* change to our local directory */
	if (chdir(PREFIX) < 0)
	{
		perror(PREFIX);
		return 20;
	}

	/* it appears certian systems *ahem*linux*ahem*
	 * don't dump cores by default, so we do this here.
	 */
	if (!getrlimit(RLIMIT_CORE, &rlim))
	{
		rlim.rlim_cur = rlim.rlim_max;
		setrlimit(RLIMIT_CORE, &rlim);
	}

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

	/* we're starting up */
	if (!(restart_file = fopen("var/atheme.restart", "r")))
		runflags |= RF_STARTING;
	else
	{
		fclose(restart_file);
		remove("var/atheme.restart");
	}

	me.start = time(NULL);
	CURRTIME = me.start;

	/* set signal handlers */
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGFPE, sighandler);
	signal(SIGILL, sighandler);
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

	/* open log */
	log_open();

	/* since me.loglevel isn't there until after the
	 * config routines run, we set the default here
	 */
	me.loglevel |= LG_ERROR;

	printf("atheme: version atheme-%s\n", version);

	if (!(runflags & RF_STARTING))
		slog(LG_INFO, "main(): restarted; not sending anything to stdout");

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

	/* setup stuff */
	event_init();
	initBlockHeap();
	init_nodes();
	hooks_init();
	servtree_init();

	modules_init();
	pcommand_init();

	conf_init();
	conf_parse();

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

	/* initialize services */
	initialize_services();

	/* load our db */
	if (db_load)
		db_load();
	else
	{
		fprintf(stderr, "atheme: no backend modules loaded, see your configuration file.\n");
		exit(EXIT_FAILURE);
	}

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

	/* no longer starting */
	runflags &= ~RF_STARTING;

	/* If we are using numeric, and the ircd protocol uses it too, fire up the uid system. */
	if (me.numeric && ircd->uses_uid)
		init_uid();

	/* we probably have a few open already... */
	me.maxfd = 3;

	/* DB commit interval is configurable */
	event_add("db_save", db_save, NULL, config_options.commit_interval);

	/* check expires every hour */
	event_add("expire_check", expire_check, NULL, 3600);

	/* check kline expires every minute */
	event_add("kline_expire", kline_expire, NULL, 60);

	uplink_connect();
	me.connected = FALSE;

	/* main loop */
	io_loop();
	me.connected = FALSE;

	/* we're shutting down */
	db_save(NULL);
	quit_sts(chansvs.me->me, "shutting down");
	close(servsock);

	remove("var/atheme.pid");

	/* should we restart? */
	if (runflags & RF_RESTART)
	{
		slog(LG_INFO, "main(): restarting in %d seconds", me.restarttime);
		restart_file = fopen("var/atheme.restart", "w");
		fclose(restart_file);

		sleep(me.restarttime);

		execve(argv[0], argv, environ);
	}

	slog(LG_INFO, "main(): shutting down: io_loop() exited");

	fclose(log_file);

	return 0;
}
