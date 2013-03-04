/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains functionality which implements the OService KLINE command.
 *
 * $Id: kline.c 151 2005-05-29 06:34:36Z nenolod $
 */

#include "atheme.h"

void os_kline(char *origin)
{
	user_t *u;
	kline_t *k;
	char *cmd = strtok(NULL, " ");
	char *s;

	if (!cmd)
	{
		notice(opersvs.nick, origin, "Insufficient parameters for \2KLINE\2.");
		notice(opersvs.nick, origin, "Syntax: KLINE ADD|DEL|LIST");
		return;
	}

	if (!strcasecmp(cmd, "ADD"))
	{
		char *target = strtok(NULL, " ");
		char *token = strtok(NULL, " ");
		char *treason, reason[BUFSIZE];
		long duration;

		if (!target || !token)
		{
			notice(opersvs.nick, origin, "Insufficient parameters for \2KLINE ADD\2.");
			notice(opersvs.nick, origin, "Syntax: KLINE ADD <nick|hostmask> [!P|!T <minutes>] " "<reason>");
			return;
		}

		if (!strcasecmp(token, "!P"))
		{
			duration = 0;
			treason = strtok(NULL, "");
			strlcpy(reason, treason, BUFSIZE);
		}
		else if (!strcasecmp(token, "!T"))
		{
			s = strtok(NULL, " ");
			duration = (atol(s) * 60);
			treason = strtok(NULL, "");
			strlcpy(reason, treason, BUFSIZE);
		}
		else
		{
			duration = config_options.kline_time;
			strlcpy(reason, token, BUFSIZE);
			treason = strtok(NULL, "");

			if (treason)
			{
				strlcat(reason, " ", BUFSIZE);
				strlcat(reason, treason, BUFSIZE);
			}
		}

		if (!(strchr(target, '@')))
		{
			if (!(u = user_find(target)))
			{
				notice(opersvs.nick, origin, "No such user: \2%s\2.", target);
				return;
			}

			if ((k = kline_find(u->user, u->host)))
			{
				notice(opersvs.nick, origin, "KLINE \2%s@%s\2 is already matched in the database.", u->user, u->host);
				return;
			}

			k = kline_add(u->user, u->host, reason, duration);
			k->setby = sstrdup(origin);
		}
		else
		{
			char *userbuf = strtok(target, "@");
			char *hostbuf = strtok(NULL, "@");
			char *tmphost;
			int i = 0;

			/* make sure there's at least 5 non-wildcards */
			for (tmphost = hostbuf; *tmphost; tmphost++)
			{
				if (*tmphost != '*' || *tmphost != '?')
					i++;
			}

			if (i < 5)
			{
				notice(opersvs.nick, origin, "Invalid host: \2%s\2.", hostbuf);
				return;
			}

			if ((k = kline_find(userbuf, hostbuf)))
			{
				notice(opersvs.nick, origin, "KLINE \2%s@%s\2 is already matched in the database.", userbuf, hostbuf);
				return;
			}

			k = kline_add(userbuf, hostbuf, reason, duration);
			k->setby = sstrdup(origin);
		}

		if (duration)
			notice(opersvs.nick, origin, "TKLINE on \2%s@%s\2 was successfully added and will " "expire in %s.", k->user, k->host, timediff(duration));
		else
			notice(opersvs.nick, origin, "KLINE on \2%s@%s\2 was successfully added.", k->user, k->host);

		snoop("KLINE:ADD: \2%s@%s\2 by \2%s\2 for \2%s\2", k->user, k->host, origin, k->reason);

		return;
	}

	if (!strcasecmp(cmd, "DEL"))
	{
		char *target = strtok(NULL, " ");
		char *userbuf, *hostbuf;
		uint32_t number;

		if (!target)
		{
			notice(opersvs.nick, origin, "Insuccicient parameters for \2KLINE DEL\2.");
			notice(opersvs.nick, origin, "Syntax: KLINE DEL <hostmask>");
			return;
		}

		if (strchr(target, ','))
		{
			int start = 0, end = 0, i;
			char t[16];

			s = strtok(target, ",");

			do
			{
				if (strchr(s, ':'))
				{
					for (i = 0; *s != ':'; s++, i++)
						t[i] = *s;

					t[++i] = '\0';
					start = atoi(t);

					s++;	/* skip past the : */

					for (i = 0; *s != '\0'; s++, i++)
						t[i] = *s;

					t[++i] = '\0';
					end = atoi(t);

					for (i = start; i <= end; i++)
					{
						if (!(k = kline_find_num(i)))
						{
							notice(opersvs.nick, origin, "No such KLINE with number \2%d\2.", i);
							continue;
						}


						notice(opersvs.nick, origin, "KLINE on \2%s@%s\2 has been successfully removed.", k->user, k->host);

						snoop("KLINE:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);

						kline_delete(k->user, k->host);
					}

					continue;
				}

				number = atoi(s);

				if (!(k = kline_find_num(number)))
				{
					notice(opersvs.nick, origin, "No such KLINE with number \2%d\2.", number);
					return;
				}

				notice(opersvs.nick, origin, "KLINE on \2%s@%s\2 has been successfully removed.", k->user, k->host);

				snoop("KLINE:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);

				kline_delete(k->user, k->host);

			} while ((s = strtok(NULL, ",")));

			return;
		}

		if (!strchr(target, '@'))
		{
			int start = 0, end = 0, i;
			char t[16];

			if (strchr(target, ':'))
			{
				for (i = 0; *target != ':'; target++, i++)
					t[i] = *target;

				t[++i] = '\0';
				start = atoi(t);

				target++;	/* skip past the : */

				for (i = 0; *target != '\0'; target++, i++)
					t[i] = *target;

				t[++i] = '\0';
				end = atoi(t);

				for (i = start; i <= end; i++)
				{
					if (!(k = kline_find_num(i)))
					{
						notice(opersvs.nick, origin, "No such KLINE with number \2%d\2.", i);
						continue;
					}

					notice(opersvs.nick, origin, "KLINE on \2%s@%s\2 has been successfully removed.", k->user, k->host);

					snoop("KLINE:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);

					kline_delete(k->user, k->host);
				}

				return;
			}

			number = atoi(target);

			if (!(k = kline_find_num(number)))
			{
				notice(opersvs.nick, origin, "No such KLINE with number \2%d\2.", number);
				return;
			}

			notice(opersvs.nick, origin, "KLINE on \2%s@%s\2 has been successfully removed.", k->user, k->host);

			snoop("KLINE:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);

			kline_delete(k->user, k->host);

			return;
		}

		userbuf = strtok(target, "@");
		hostbuf = strtok(NULL, "@");

		if (!(k = kline_find(userbuf, hostbuf)))
		{
			notice(opersvs.nick, origin, "No such KLINE: \2%s@%s\2.", userbuf, hostbuf);
			return;
		}

		kline_delete(userbuf, hostbuf);

		notice(opersvs.nick, origin, "KLINE on \2%s@%s\2 has been successfully removed.", userbuf, hostbuf);

		snoop("KLINE:DEL: \2%s@%s\2 by \2%s\2", k->user, k->host, origin);

		return;
	}

	if (!strcasecmp(cmd, "LIST"))
	{
		boolean_t full = FALSE;
		node_t *n;
		int i = 0;

		s = strtok(NULL, " ");

		if (s && !strcasecmp(s, "FULL"))
			full = TRUE;

		if (full)
			notice(opersvs.nick, origin, "KLINE list (with reasons):");
		else
			notice(opersvs.nick, origin, "KLINE list:");

		LIST_FOREACH(n, klnlist.head)
		{
			k = (kline_t *)n->data;

			i++;

			if (k->duration && full)
				notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - expires in \2%s\2 - (%s)", k->number, k->user, k->host, k->setby, timediff(k->expires - CURRTIME), k->reason);
			else if (k->duration && !full)
				notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - expires in \2%s\2", k->number, k->user, k->host, k->setby, timediff(k->expires - CURRTIME));
			else if (!k->duration && full)
				notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - \2permanent\2 - (%s)", k->number, k->user, k->host, k->setby, k->reason);
			else
				notice(opersvs.nick, origin, "%d: %s@%s - by \2%s\2 - \2permanent\2", k->number, k->user, k->host, k->setby);
		}

		notice(opersvs.nick, origin, "Total of \2%d\2 %s in KLINE list.", i, (i == 1) ? "entry" : "entries");
	}
}
