/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the signal handling routines.
 *
 * $Id: signal.c 908 2005-07-17 04:00:28Z w00t $
 */

#include "atheme.h"

void sighandler(int signum)
{
        /* rehash */
        if (signum == SIGHUP)
        {
                slog(LG_INFO, "sighandler(): got SIGHUP, rehashing %s", config_file);

                wallops("Got SIGHUP; reloading \2%s\2.", config_file);

                snoop("UPDATE: \2%s\2", "system console");
                wallops("Updating database by request of \2%s\2.", "system console");
                expire_check(NULL);
                db_save(NULL);

                snoop("REHASH: \2%s\2", "system console");
                wallops("Rehashing \2%s\2 by request of \2%s\2.", config_file, "system console");

                if (!conf_rehash())
                        wallops("REHASH of \2%s\2 failed. Please corrrect any errors in the " "file and try again.", config_file);

                return;
        }

        /* usually caused by ^C */
        else if (signum == SIGINT && (runflags & RF_LIVE))
        {
                wallops("Exiting on signal %d.", signum);
                quit_sts(chansvs.me->me, "caught interrupt");
                me.connected = FALSE;
                slog(LG_INFO, "sighandler(): caught interrupt; exiting...");
                runflags |= RF_SHUTDOWN;
        }

        else if (signum == SIGINT && !(runflags & RF_LIVE))
        {
                wallops("Got SIGINT; restarting in \2%d\2 seconds.", me.restarttime);

                snoop("UPDATE: \2%s\2", "system console");
                wallops("Updating database by request of \2%s\2.", "system console");
                expire_check(NULL);
                db_save(NULL);

                snoop("RESTART: \2%s\2", "system console");
                wallops("Restarting in \2%d\2 seconds by request of \2%s\2.", me.restarttime, "system console");

                slog(LG_INFO, "sighandler(): restarting...");
                runflags |= RF_RESTART;
        }

        else if (signum == SIGTERM)
        {
                wallops("Exiting on signal %d.", signum);
                slog(LG_INFO, "sighandler(): got SIGTERM; exiting...");
                runflags |= RF_SHUTDOWN;
        }

        else if (signum == SIGUSR1)
        {
                wallops("Panic! Out of memory.");
                quit_sts(chansvs.me->me, "out of memory!");
                me.connected = FALSE;
                slog(LG_INFO, "sighandler(): out of memory; exiting");
                runflags |= RF_SHUTDOWN;
        }

        else if (signum == SIGUSR2)
        {
                wallops("Got SIGUSER2; restarting in \2%d\2 seconds.", me.restarttime);

                snoop("UPDATE: \2%s\2", "system console");
                wallops("Updating database by request of \2%s\2.", "system console");
                expire_check(NULL);
                db_save(NULL);

                snoop("RESTART: \2%s\2", "system console");
                wallops("Restarting in \2%d\2 seconds by request of \2%s\2.", me.restarttime, "system console");

                slog(LG_INFO, "sighandler(): restarting...");
                runflags |= RF_RESTART;
        }
}

