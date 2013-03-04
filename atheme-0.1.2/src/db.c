/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains database routines.
 *
 * $Id: db.c 251 2005-05-31 00:54:02Z nenolod $
 */

#include "atheme.h"

/* write atheme.db */
void db_save(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	node_t *n, *tn;
	FILE *f;
	uint32_t i, muout = 0, mcout = 0, caout = 0, kout = 0;

	/* back them up first */
	if ((rename("etc/atheme.db", "etc/atheme.db.save")) < 0)
	{
		slog(LG_ERROR, "db_save(): cannot backup atheme.db");
		return;
	}

	if (!(f = fopen("etc/atheme.db", "w")))
	{
		slog(LG_ERROR, "db_save(): cannot write to atheme.db");
		return;
	}

	/* write the database version */
	fprintf(f, "DBV 2\n");

	slog(LG_DEBUG, "db_save(): saving myusers");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			mu = (myuser_t *)n->data;

			/* MU <name> <pass> <email> <registered> [lastlogin] [failnum] [lastfail]
			 * [lastfailon] [flags] [key]
			 */
			fprintf(f, "MU %s %s %s %ld", mu->name, mu->pass, mu->email, (long)mu->registered);

			if (mu->lastlogin)
				fprintf(f, " %ld", (long)mu->lastlogin);
			else
				fprintf(f, " 0");

			if (mu->failnum)
			{
				fprintf(f, " %d %s %ld", mu->failnum, mu->lastfail, (long)mu->lastfailon);
			}
			else
				fprintf(f, " 0 0 0");

			if (mu->flags)
				fprintf(f, " %d", mu->flags);
			else
				fprintf(f, " 0");

			if (mu->key)
				fprintf(f, " %ld\n", mu->key);
			else
				fprintf(f, "\n");

			muout++;
		}
	}

	slog(LG_DEBUG, "db_save(): saving mychans");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			/* MC <name> <pass> <founder> <registered> [used] [flags]
			 * [mlock_on] [mlock_off] [mlock_limit] [mlock_key]
			 */
			fprintf(f, "MC %s %s %s %ld %ld", mc->name, mc->pass, mc->founder->name, (long)mc->registered, (long)mc->used);

			if (mc->flags)
				fprintf(f, " %d", mc->flags);
			else
				fprintf(f, " 0");

			if (mc->mlock_on)
				fprintf(f, " %d", mc->mlock_on);
			else
				fprintf(f, " 0");

			if (mc->mlock_off)
				fprintf(f, " %d", mc->mlock_off);
			else
				fprintf(f, " 0");

			if (mc->mlock_limit)
				fprintf(f, " %d", mc->mlock_limit);
			else
				fprintf(f, " 0");

			if (mc->mlock_key)
				fprintf(f, " %s\n", mc->mlock_key);
			else
				fprintf(f, "\n");

			if (mc->url)
				fprintf(f, "UR %s %s\n", mc->name, mc->url);

			if (mc->entrymsg)
				fprintf(f, "EM %s %s\n", mc->name, mc->entrymsg);

			mcout++;
		}
	}

	slog(LG_DEBUG, "db_save(): saving chanacs");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			LIST_FOREACH(tn, mc->chanacs.head)
			{
				ca = (chanacs_t *)tn->data;

				/* CA <mychan> <myuser|hostmask> <level> */
				fprintf(f, "CA %s %s %s\n", ca->mychan->name, (ca->host) ? ca->host : ca->myuser->name, bitmask_to_flags(ca->level, chanacs_flags));

				caout++;
			}
		}
	}

	slog(LG_DEBUG, "db_save(): saving klines");

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		/* KL <user> <host> <duration> <settime> <setby> <reason> */
		fprintf(f, "KL %s %s %ld %ld %s %s\n", k->user, k->host, k->duration, (long)k->settime, k->setby, k->reason);

		kout++;
	}

	/* DE <muout> <mcout> <caout> <kout> */
	fprintf(f, "DE %d %d %d %d\n", muout, mcout, caout, kout);

	fclose(f);
	remove("etc/atheme.db.save");
}

/* loads atheme.db */
void db_load(void)
{
	sra_t *sra;
	myuser_t *mu;
	mychan_t *mc;
	kline_t *k;
	node_t *n;
	uint32_t i = 0, linecnt = 0, muin = 0, mcin = 0, cain = 0, kin = 0;
	FILE *f = fopen("etc/atheme.db", "r");
	char *item, *s, dBuf[BUFSIZE];

	if (!f)
	{
		slog(LG_ERROR, "db_load(): can't open atheme.db for reading");

		/* restore the backup */
		if ((rename("etc/atheme.db.save", "etc/atheme.db")) < 0)
		{
			slog(LG_ERROR, "db_load(): unable to restore backup, looking for shrike database to import");
			f = fopen("etc/atheme.db", "r");

			if (!f)
			{
				slog(LG_ERROR, "db_load(): can't find a shrike database to import. bailing out of db_load()");
				return;
			}
		}
		else
		{
			slog(LG_ERROR, "db_load(): restored from backup");
			f = fopen("etc/atheme.db", "r");
		}
	}

	slog(LG_DEBUG, "db_load(): ----------------------- loading ------------------------");

	/* start reading it, one line at a time */
	while (fgets(dBuf, BUFSIZE, f))
	{
		linecnt++;

		/* check for unimportant lines */
		item = strtok(dBuf, " ");
		strip(item);

		if (*item == '#' || *item == '\n' || *item == '\t' || *item == ' ' || *item == '\0' || *item == '\r' || !*item)
			continue;

		/* database version */
		if (!strcmp("DBV", item))
		{
			i = atoi(strtok(NULL, " "));
			if (i > 2)
			{
				slog(LG_INFO, "db_load(): database version is %d; i only understand " "2 (Atheme) or 1 (Shrike)", i);
				exit(EXIT_FAILURE);
			}
		}

		/* myusers */
		if (!strcmp("MU", item))
		{
			char *muname, *mupass, *muemail;

			if ((s = strtok(NULL, " ")))
			{
				if ((mu = myuser_find(s)))
					continue;

				muin++;

				muname = s;
				mupass = strtok(NULL, " ");
				muemail = strtok(NULL, " ");

				mu = myuser_add(muname, mupass, muemail);

				mu->registered = atoi(strtok(NULL, " "));

				mu->lastlogin = atoi(strtok(NULL, " "));

				mu->failnum = atoi(strtok(NULL, " "));
				mu->lastfail = sstrdup(strtok(NULL, " "));
				mu->lastfailon = atoi(strtok(NULL, " "));
				mu->flags = atol(strtok(NULL, " "));

				if ((s = strtok(NULL, " ")))
					mu->key = atoi(s);
			}
		}

		/* mychans */
		if (!strcmp("MC", item))
		{
			char *mcname, *mcpass;

			if ((s = strtok(NULL, " ")))
			{
				if ((mc = mychan_find(s)))
					continue;

				mcin++;

				mcname = s;
				mcpass = strtok(NULL, " ");

				mc = mychan_add(mcname, mcpass);

				mc->founder = myuser_find(strtok(NULL, " "));
				mc->registered = atoi(strtok(NULL, " "));
				mc->used = atoi(strtok(NULL, " "));
				mc->flags = atoi(strtok(NULL, " "));

				mc->mlock_on = atoi(strtok(NULL, " "));
				mc->mlock_off = atoi(strtok(NULL, " "));
				mc->mlock_limit = atoi(strtok(NULL, " "));

				if ((s = strtok(NULL, " ")))
				{
					strip(s);
					mc->mlock_key = sstrdup(s);
				}
			}
		}

		/* Channel URLs */
		if (!strcmp("UR", item))
		{
			char *chan, *url;

			chan = strtok(NULL, " ");
			url = strtok(NULL, " ");

			if (chan && url)
			{
				mc = mychan_find(chan);

				if (mc)
					mc->url = sstrdup(url);
			}
		}

		/* Channel entry messages */
		if (!strcmp("EM", item))
		{
			char *chan, *message;

			chan = strtok(NULL, " ");
			message = strtok(NULL, "");

			if (chan && message)
			{
				mc = mychan_find(chan);

				if (mc)
					mc->entrymsg = sstrdup(message);
			}
		}

		/* chanacs */
		if (!strcmp("CA", item))
		{
			chanacs_t *ca;
			char *cachan, *causer;

			cachan = strtok(NULL, " ");
			causer = strtok(NULL, " ");

			if (cachan && causer)
			{
				mc = mychan_find(cachan);
				mu = myuser_find(causer);

				cain++;

				if (i == DB_ATHEME)
				{
					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, flags_to_bitmask(strtok(NULL, " "), chanacs_flags, 0x0));
					else
						ca = chanacs_add(mc, mu, flags_to_bitmask(strtok(NULL, " "), chanacs_flags, 0x0));

					if (ca->level & CA_SUCCESSOR)
						ca->mychan->successor = ca->myuser;
				}
				else if (i == DB_SHRIKE)	/* DB_SHRIKE */
				{
					int fl = atol(strtok(NULL, " "));
					int fl2 = 0x0;

					switch (fl)
					{
					  case SHRIKE_CA_VOP:
						  fl2 = CA_VOP;
					  case SHRIKE_CA_AOP:
						  fl2 = CA_AOP;
					  case SHRIKE_CA_SOP:
						  fl2 = CA_SOP;
					  case SHRIKE_CA_SUCCESSOR:
						  fl2 = CA_SUCCESSOR;
					  case SHRIKE_CA_FOUNDER:
						  fl2 = CA_FOUNDER;
					}

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl2);
					else
						ca = chanacs_add(mc, mu, fl2);

					if (ca->level & CA_SUCCESSOR)
						ca->mychan->successor = ca->myuser;
				}
			}
		}

		/* klines */
		if (!strcmp("KL", item))
		{
			char *user, *host, *reason, *setby, *tmp;
			time_t settime;
			long duration;

			user = strtok(NULL, " ");
			host = strtok(NULL, " ");
			tmp = strtok(NULL, " ");
			duration = atol(tmp);
			tmp = strtok(NULL, " ");
			settime = atol(tmp);
			setby = strtok(NULL, " ");
			reason = strtok(NULL, "");

			strip(reason);

			k = kline_add(user, host, reason, duration);
			k->settime = settime;
			k->setby = sstrdup(setby);

			kin++;
		}

		/* end */
		if (!strcmp("DE", item))
		{
			i = atoi(strtok(NULL, " "));
			if (i != muin)
				slog(LG_ERROR, "db_load(): got %d myusers; expected %d", muin, i);

			i = atoi(strtok(NULL, " "));
			if (i != mcin)
				slog(LG_ERROR, "db_load(): got %d mychans; expected %d", mcin, i);

			i = atoi(strtok(NULL, " "));
			if (i != cain)
				slog(LG_ERROR, "db_load(): got %d chanacs; expected %d", cain, i);

			if ((s = strtok(NULL, " ")))
				if ((i = atoi(s)) != kin)
					slog(LG_ERROR, "db_load(): got %d klines; expected %d", kin, i);
		}
	}

	/* now we update the sra list */
	LIST_FOREACH(n, sralist.head)
	{
		sra = (sra_t *)n->data;

		if (!sra->myuser)
		{
			sra->myuser = myuser_find(sra->name);

			if (sra->myuser)
			{
				slog(LG_DEBUG, "db_load(): updating %s to SRA", sra->name);
				sra->myuser->sra = sra;
			}
		}
	}

	fclose(f);

	slog(LG_DEBUG, "db_load(): ------------------------- done -------------------------");
}
