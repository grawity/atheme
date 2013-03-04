/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the implementation of the Atheme 0.1
 * flatfile database format, with metadata extensions.
 *
 * $Id: flatfile.c 948 2005-07-17 20:39:21Z nenolod $
 */

#include "atheme.h"

static BlockHeap *kline_heap;		/* 16 */
static BlockHeap *myuser_heap;		/* HEAP_USER */
static BlockHeap *mychan_heap;		/* HEAP_CHANNEL */
static BlockHeap *chanacs_heap; 	/* HEAP_CHANACS */
static BlockHeap *metadata_heap;	/* HEAP_CHANUSER */

/* write atheme.db */
static void flatfile_db_save(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	node_t *n, *tn;
	FILE *f;
	uint32_t i, muout = 0, mcout = 0, caout = 0, kout = 0;

	/* write to a temporary file first */
	if (!(f = fopen("etc/atheme.db.new", "w")))
	{
		slog(LG_ERROR, "db_save(): cannot write to atheme.db.new");
		wallops("\2DATABASE ERROR\2: db_save(): cannot write to atheme.db.new");
		return;
	}

	/* write the database version */
	fprintf(f, "DBV 4\n");

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
				fprintf(f, " %d\n", mu->key);
			else
				fprintf(f, "\n");

			muout++;

			LIST_FOREACH(tn, mu->metadata.head)
			{
				metadata_t *md = (metadata_t *)tn->data;

				fprintf(f, "MD U %s %s %s\n", mu->name, md->name, md->value);
			}
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

			LIST_FOREACH(tn, mc->chanacs.head)
			{
				ca = (chanacs_t *)tn->data;

				fprintf(f, "CA %s %s %s\n", ca->mychan->name, (ca->host) ? ca->host : ca->myuser->name,
						bitmask_to_flags(ca->level, chanacs_flags));

				caout++;
			}

			LIST_FOREACH(tn, mc->metadata.head)
			{
				metadata_t *md = (metadata_t *)tn->data;

				fprintf(f, "MD C %s %s %s\n", mc->name, md->name, md->value);
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

	/* now, replace the old database with the new one, using an atomic rename */
	if ((rename("etc/atheme.db.new", "etc/atheme.db")) < 0)
	{
		slog(LG_ERROR, "db_save(): cannot rename atheme.db.new to atheme.db");
		wallops("\2DATABASE ERROR\2: db_save(): cannot rename atheme.db.new to atheme.db");
		return;
	}
}

/* loads atheme.db */
static void flatfile_db_load(void)
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
		return;
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
			if (i > 4)
			{
				slog(LG_INFO, "db_load(): database version is %d; i only understand 4 (Atheme 0.2), 3 (Atheme 0.2 without CA_ACLVIEW), 2 (Atheme 0.1) or 1 (Shrike)", i);
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

		/* Metadata entry */
		if (!strcmp("MD", item))
		{
			char *type = strtok(NULL, " ");
			char *name = strtok(NULL, " ");
			char *property = strtok(NULL, " ");
			char *value = strtok(NULL, "");

			if (!type || !name || !property || !value)
				continue;

			strip(value);

			if (type[0] == 'U')
			{
				mu = myuser_find(name);
				metadata_add(mu, METADATA_USER, property, value);
			}
			else
			{
				mc = mychan_find(name);
				metadata_add(mc, METADATA_CHANNEL, property, value);
			}
		}

		/* Channel URLs */
		if (!strcmp("UR", item))
		{
			char *chan, *url;

			chan = strtok(NULL, " ");
			url = strtok(NULL, " ");

			strip(url);

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

			strip(message);

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

				if (i >= DB_ATHEME)
				{
					uint32_t fl = flags_to_bitmask(strtok(NULL, " "), chanacs_flags, 0x0);

					/* Compatibility with oldworld Atheme db's. --nenolod */
					if (fl == OLD_CA_AOP)
						fl = CA_AOP;

					/* previous to CA_ACLVIEW, everyone could view
					 * access lists. If they aren't AKICKed, upgrade
					 * them. This keeps us from breaking old XOPs.
					 */
					if (i < 4)
						if (!(fl & CA_AKICK))
							fl |= CA_ACLVIEW;

					if ((!mu) && (validhostmask(causer)))
						ca = chanacs_add_host(mc, causer, fl);
					else
						ca = chanacs_add(mc, mu, fl);

					/* Do we have enough flags to be the successor? */
					if (ca->level & CA_SUCCESSOR && !ca->mychan->successor)
						ca->mychan->successor = ca->myuser;
				}
				else if (i == DB_SHRIKE)	/* DB_SHRIKE */
				{
					uint32_t fl = atol(strtok(NULL, " "));
					uint32_t fl2 = 0x0;

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

static kline_t *flatfile_kline_add(char *user, char *host, char *reason, long duration)
{
        kline_t *k;
        node_t *n = node_create();
        static uint32_t kcnt = 0;
        
        slog(LG_DEBUG, "kline_add(): %s@%s -> %s (%ld)", user, host, reason, duration);
        
        k = BlockHeapAlloc(kline_heap);
         
        node_add(k, n, &klnlist);
 
        k->user = sstrdup(user);
        k->host = sstrdup(host);    
        k->reason = sstrdup(reason);
        k->duration = duration;
        k->settime = CURRTIME;
        k->expires = CURRTIME + duration;
        k->number = ++kcnt;
 
        cnt.kline++;
 
        kline_sts("*", user, host, duration, reason);

        return k;
}

static void flatfile_kline_delete(char *user, char *host)
{
        kline_t *k = kline_find(user, host);
        node_t *n;
        
        if (!k)
        {
                slog(LG_DEBUG, "kline_delete(): called for nonexistant kline: %s@%s", user, host);
                
                return;
        }
         
        slog(LG_DEBUG, "kline_delete(): %s@%s -> %s", k->user, k->host, k->reason);
        
        n = node_find(k, &klnlist);
        node_del(n, &klnlist);
        node_free(n);
        
        free(k->user);
        free(k->host);
        free(k->reason);
        free(k->setby); 
        
        BlockHeapFree(kline_heap, k);
        
        unkline_sts("*", user, host);
        
        cnt.kline--;
}

static kline_t *flatfile_kline_find(char *user, char *host)
{
        kline_t *k;
        node_t *n; 
        
        LIST_FOREACH(n, klnlist.head)
        {
                k = (kline_t *)n->data;
                
                if ((!match(k->user, user)) && (!match(k->host, host)))
                        return k;
        }
         
        return NULL;
}

static kline_t *flatfile_kline_find_num(uint32_t number)
{
        kline_t *k;
        node_t *n; 
        
        LIST_FOREACH(n, klnlist.head)
        {
                k = (kline_t *)n->data;
                
                if (k->number == number)
                        return k;
        }
         
        return NULL;
}
 
static void flatfile_kline_expire(void *arg)
{
        kline_t *k;
        node_t *n, *tn;
        
        LIST_FOREACH_SAFE(n, tn, klnlist.head)
        {
                k = (kline_t *)n->data;
                
                if (k->duration == 0)
                        continue;
                        
                if (k->expires <= CURRTIME)
                {
                        snoop("KLINE:EXPIRE: \2%s@%s\2 set \2%s\2 ago by \2%s\2", k->user, k->host, time_ago(k->settime), k->setby);
                        
                        kline_delete(k->user, k->host);
                }
        }
}

static myuser_t *flatfile_myuser_add(char *name, char *pass, char *email)
{
        myuser_t *mu;
        node_t *n;

        mu = myuser_find(name);

        if (mu)
        {
                slog(LG_DEBUG, "myuser_add(): myuser already exists: %s", name);
                return mu;
        }

        slog(LG_DEBUG, "myuser_add(): %s -> %s", name, email);

        n = node_create();
        mu = BlockHeapAlloc(myuser_heap);

        mu->name = sstrdup(name);
        mu->pass = sstrdup(pass);
        mu->email = sstrdup(email);
        mu->registered = CURRTIME;
        mu->hash = MUHASH((unsigned char *)name);

        node_add(mu, n, &mulist[mu->hash]);

        cnt.myuser++;

        return mu;
}

static void flatfile_myuser_delete(char *name)
{
        sra_t *sra;
        myuser_t *mu = myuser_find(name);
        mychan_t *mc;
        chanacs_t *ca;
        node_t *n, *tn;
        uint32_t i;
        
        if (!mu)
        {
                slog(LG_DEBUG, "myuser_delete(): called for nonexistant myuser: %s", name);
                return;
        }
         
        slog(LG_DEBUG, "myuser_delete(): %s", mu->name);
        
        /* remove their chanacs shiz */
        LIST_FOREACH_SAFE(n, tn, mu->chanacs.head)
        {
                ca = (chanacs_t *)n->data;
                
                chanacs_delete(ca->mychan, ca->myuser, ca->level);
        }
         
        /* remove them as successors */
        for (i = 0; i < HASHSIZE; i++);
        {
                LIST_FOREACH(n, mclist[i].head)
                {
                        mc = (mychan_t *)n->data;
                        
                        if ((mc->successor) && (mc->successor == mu))
                                mc->successor = NULL;
                }
        }
         
        /* remove them from the sra list */
        if ((sra = sra_find(mu)))
                sra_delete(mu);  
                
        n = node_find(mu, &mulist[mu->hash]);
        node_del(n, &mulist[mu->hash]);
        node_free(n);
        
        free(mu->name);
        free(mu->pass);
        free(mu->email);
        BlockHeapFree(myuser_heap, mu);

        cnt.myuser--;
}

static myuser_t *flatfile_myuser_find(char *name)
{
        myuser_t *mu;
        node_t *n;   
        uint32_t i;  
        
        for (i = 0; i < HASHSIZE; i++)
        {
                LIST_FOREACH(n, mulist[i].head)
                {
                        mu = (myuser_t *)n->data;
                        
                        if (!irccasecmp(name, mu->name))
                                return mu;
                }
        }
         
        return NULL;
}

static mychan_t *flatfile_mychan_add(char *name, char *pass)
{       
        mychan_t *mc;
        node_t *n;
        
        mc = mychan_find(name);

        if (mc)
        {       
                slog(LG_DEBUG, "mychan_add(): mychan already exists: %s", name);
                return mc;
        }
        
        slog(LG_DEBUG, "mychan_add(): %s", name);
        
        n = node_create();
        mc = BlockHeapAlloc(mychan_heap);
                
        mc->name = sstrdup(name);
        mc->pass = sstrdup(pass);
        mc->founder = NULL;
        mc->successor = NULL;
        mc->registered = CURRTIME;
        mc->chan = channel_find(name);
        mc->hash = MCHASH((unsigned char *)name);

        node_add(mc, n, &mclist[mc->hash]);

        cnt.mychan++;

        return mc;
}

static void flatfile_mychan_delete(char *name)
{
        mychan_t *mc = mychan_find(name);
        chanacs_t *ca;
        node_t *n, *tn;
 
        if (!mc)  
        {
                slog(LG_DEBUG, "mychan_delete(): called for nonexistant mychan: %s", name);
                return;
        }

        slog(LG_DEBUG, "mychan_delete(): %s", mc->name);
         
        /* remove the chanacs shiz */
        LIST_FOREACH_SAFE(n, tn, mc->chanacs.head)
        {
                ca = (chanacs_t *)n->data;
         
                if (ca->host)
                        chanacs_delete_host(ca->mychan, ca->host, ca->level);
                else
                        chanacs_delete(ca->mychan, ca->myuser, ca->level);
        }
 
        n = node_find(mc, &mclist[mc->hash]);
        node_del(n, &mclist[mc->hash]);
        node_free(n);  
        
        free(mc->name);
        free(mc->pass);
        BlockHeapFree(mychan_heap, mc);
                
        cnt.mychan--;
}        

static mychan_t *flatfile_mychan_find(char *name)
{
        mychan_t *mc; 
        node_t *n;    
        uint32_t i;   
        
        for (i = 0; i < HASHSIZE; i++)
        {
                LIST_FOREACH(n, mclist[i].head)
                {
                        mc = (mychan_t *)n->data;
         
                        if (!irccasecmp(name, mc->name))
                                return mc;
                }
        }
        
        return NULL;
}       

static chanacs_t *flatfile_chanacs_add(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
        chanacs_t *ca;
        node_t *n1;
        node_t *n2;

        if (*mychan->name != '#')
        {
                slog(LG_DEBUG, "chanacs_add(): got non #channel: %s", mychan->name);
                return NULL;
        }

        slog(LG_DEBUG, "chanacs_add(): %s -> %s", mychan->name, myuser->name);

        n1 = node_create();
        n2 = node_create();

        ca = BlockHeapAlloc(chanacs_heap);

        ca->mychan = mychan;
        ca->myuser = myuser;
        ca->level |= level;
 
        node_add(ca, n1, &mychan->chanacs);
        node_add(ca, n2, &myuser->chanacs);
        
        cnt.chanacs++;
        
        return ca;
}

static chanacs_t *flatfile_chanacs_add_host(mychan_t *mychan, char *host, uint32_t level)
{
        chanacs_t *ca;
        node_t *n;
        
        if (*mychan->name != '#')
        {
                slog(LG_DEBUG, "chanacs_add_host(): got non #channel: %s", mychan->name);
                return NULL;
        }
         
        slog(LG_DEBUG, "chanacs_add_host(): %s -> %s", mychan->name, host);
        
        n = node_create();
        
        ca = BlockHeapAlloc(chanacs_heap);
        
        ca->mychan = mychan;
        ca->myuser = NULL;  
        ca->host = sstrdup(host);
        ca->level |= level;
        
        node_add(ca, n, &mychan->chanacs);
        
        cnt.chanacs++;
        
        return ca;
}

static void flatfile_chanacs_delete(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
        chanacs_t *ca;
        node_t *n, *tn, *n2;
        
        LIST_FOREACH_SAFE(n, tn, mychan->chanacs.head)
        {
                ca = (chanacs_t *)n->data;
                
                if ((ca->myuser == myuser) && (ca->level == level))
                {
                        slog(LG_DEBUG, "chanacs_delete(): %s -> %s", ca->mychan->name, ca->myuser->name);
                        node_del(n, &mychan->chanacs);
                        node_free(n);
                        
                        n2 = node_find(ca, &myuser->chanacs);
                        node_del(n2, &myuser->chanacs);
                        node_free(n2);
                        
                        cnt.chanacs--;
                        
                        return;
                }
        }
}

static void flatfile_chanacs_delete_host(mychan_t *mychan, char *host, uint32_t level)
{
        chanacs_t *ca;
        node_t *n, *tn;
        
        LIST_FOREACH_SAFE(n, tn, mychan->chanacs.head)
        {
                ca = (chanacs_t *)n->data;
                
                if ((ca->host) && (!irccasecmp(host, ca->host)) && (ca->level == level))
                {
                        slog(LG_DEBUG, "chanacs_delete_host(): %s -> %s", ca->mychan->name, ca->host);
                        
                        free(ca->host);
                        node_del(n, &mychan->chanacs);
                        node_free(n);
                        
                        BlockHeapFree(chanacs_heap, ca);
                        
                        cnt.chanacs--;
                        
                        return;
                }
        }
}

static chanacs_t *flatfile_chanacs_find(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
        node_t *n;
        chanacs_t *ca;
        
        if ((!mychan) || (!myuser))
                return NULL;
                
        LIST_FOREACH(n, mychan->chanacs.head)
        {
                ca = (chanacs_t *)n->data;
                
                if (level != 0x0)
                {
                        if ((ca->myuser == myuser) && ((ca->level & level) == level))
                                return ca;
                }
                else if (ca->myuser == myuser)
                        return ca;
        }
         
        return NULL;
}

static chanacs_t *flatfile_chanacs_find_host(mychan_t *mychan, char *host, uint32_t level)
{
        node_t *n;
        chanacs_t *ca;
        
        if ((!mychan) || (!host))
                return NULL;
                
        LIST_FOREACH(n, mychan->chanacs.head)
        {
                ca = (chanacs_t *)n->data;
                
                if (level != 0x0)
                {
                        if ((ca->host) && (!match(ca->host, host)) && ((ca->level & level) == level))
                                return ca;
                }
                else if ((ca->host) && (!match(ca->host, host)))
                        return ca;
        }
         
        return NULL;
}

static chanacs_t *flatfile_chanacs_find_host_literal(mychan_t *mychan, char *host, uint32_t level)
{
        node_t *n;
        chanacs_t *ca;
        
        if ((!mychan) || (!host))
                return NULL;
                
        LIST_FOREACH(n, mychan->chanacs.head)
        {
                ca = (chanacs_t *)n->data;
                
                if (level != 0x0)
                {
                        if ((ca->host) && (!strcasecmp(ca->host, host)) && ((ca->level & level) == level))
                                return ca;
                }
                else if ((ca->host) && (!strcasecmp(ca->host, host)))
                        return ca;
        }
         
        return NULL;
}

static metadata_t *flatfile_metadata_add(void *target, int32_t type, char *name, char *value)
{
        myuser_t *mu = NULL;
        mychan_t *mc = NULL;
        metadata_t *md;
        node_t *n;
   
        if (type == METADATA_USER)
                mu = target;
        else
                mc = target;

        if ((md = metadata_find(target, type, name)))
                metadata_delete(target, type, name);

        md = BlockHeapAlloc(metadata_heap);

        md->name = sstrdup(name);
        md->value = sstrdup(value);

        n = node_create();
 
        if (type == METADATA_USER)
                node_add(md, n, &mu->metadata);
        else
                node_add(md, n, &mc->metadata);

	if (!strncmp("private:", md->name, 8))
		md->private = TRUE;

        return md;
}

static void flatfile_metadata_delete(void *target, int32_t type, char *name)
{
        node_t *n;
        myuser_t *mu;   
        mychan_t *mc;
	metadata_t *md = metadata_find(target, type, name);

	if (!md)
		return;

        if (type == METADATA_USER)
        {
                mu = target;
                n = node_find(md, &mu->metadata);
                node_del(n, &mu->metadata);
        }
        else
        {
                mc = target;
                n = node_find(md, &mc->metadata);
                node_del(n, &mc->metadata);
        }

        free(md->name);
        free(md->value);

        BlockHeapFree(metadata_heap, md);
}

static metadata_t *flatfile_metadata_find(void *target, int32_t type, char *name)
{
        node_t *n;      
        myuser_t *mu;
        mychan_t *mc;
        list_t *l = NULL;
        metadata_t *md;
                
        if (type == METADATA_USER)
        {
                mu = target;
                l = &mu->metadata;
        }
        else
        {
                mc = target;
                l = &mc->metadata;
        }

        LIST_FOREACH(n, l->head)
        {
                md = n->data;

                if (!strcasecmp(md->name, name))
                        return md;
        }

        return NULL; 
}

/* XXX This routine does NOT work right. */
static void flatfile_expire_check(void *arg)
{
        uint32_t i, j, w, tcnt;
        myuser_t *mu;
        mychan_t *mc, *tmc;
        node_t *n1, *n2, *tn, *n3;
                                                                
        for (i = 0; i < HASHSIZE; i++)
        {
                LIST_FOREACH(n1, mulist[i].head)
                {
                        mu = (myuser_t *)n1->data;
                                                                 
                        if (MU_HOLD & mu->flags)
                                continue;
                                                        
                        if (((CURRTIME - mu->lastlogin) >= config_options.expire) || ((mu->flags & MU_WAITAUTH) && (CURRTIME - mu->registered >= 86400)))
                        {
                                /* kill all their channels */
                                for (j = 0; j < HASHSIZE; j++)
                                {
                                        LIST_FOREACH(tn, mclist[j].head)
                                        {
                                                mc = (mychan_t *)tn->data;

                                                if (mc->founder == mu && mc->successor)
                                                {
                                                        /* make sure they're within limits */
                                                        for (w = 0, tcnt = 0; w < HASHSIZE; w++)
                                                        {
                                                                LIST_FOREACH(n3, mclist[i].head)
                                                                {
                                                                        tmc = (mychan_t *)n3->data;

                                                                        if (is_founder(tmc, mc->successor))
                                                                                tcnt++;
                                                                }
                                                        }

                                                        if ((tcnt >= me.maxchans) && (!is_sra(mc->successor)))
                                                                continue;

                                                        snoop("SUCCESSION: \2%s\2 -> \2%s\2 from \2%s\2", mc->successor->name, mc->name, mc->founder->name);
                                 
                                                        chanacs_delete(mc, mc->successor, CA_SUCCESSOR);
                                                        chanacs_add(mc, mc->successor, CA_FOUNDER);
                                                        mc->founder = mc->successor;
                                                        mc->successor = NULL;
                                                
                                                       if (mc->founder->user)
                                                                notice(chansvs.nick, mc->founder->user->nick, "You are now founder on \2%s\2.", mc->name);
                                                        
                                                        return;
                                                }
                                                else if (mc->founder == mu)
                                                {
                                                        snoop("EXPIRE: \2%s\2 from \2%s\2", mc->name, mu->name);

							if ((config_options.chan && irccasecmp(mc->name, config_options.chan)) || !config_options.chan)
	                                                        part(mc->name, chansvs.nick);

                                                        mychan_delete(mc->name);
                                                }
                                        }
                                }
                                                                
                                snoop("EXPIRE: \2%s\2 from \2%s\2 ", mu->name, mu->email);
                                myuser_delete(mu->name);
                        }
                }
        }
                                                        
        for (i = 0; i < HASHSIZE; i++)
        {
                LIST_FOREACH(n2, mclist[i].head)
                {
                        mc = (mychan_t *)n2->data;
                                                        
                        if (MU_HOLD & mc->founder->flags)
                                continue;
                                                  
                        if (MC_HOLD & mc->flags)
                                continue;
                                                        
                        if ((CURRTIME - mc->used) >= config_options.expire)
                        {
                                snoop("EXPIRE: \2%s\2 from \2%s\2", mc->name, mc->founder->name);
                                 
                                part(mc->name, chansvs.nick);
                                mychan_delete(mc->name);
			}
                }
        }
}

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;

	db_load = &flatfile_db_load;
	db_save = &flatfile_db_save;

	/* KLINES */
	kline_add = &flatfile_kline_add;
	kline_delete = &flatfile_kline_delete;
	kline_find = &flatfile_kline_find;
	kline_find_num = &flatfile_kline_find_num;
	kline_expire = &flatfile_kline_expire;

	/* MYUSER */
	myuser_add = &flatfile_myuser_add;
	myuser_delete = &flatfile_myuser_delete;
	myuser_find = &flatfile_myuser_find;

	/* MYCHAN */
	mychan_add = &flatfile_mychan_add;
	mychan_delete = &flatfile_mychan_delete;
	mychan_find = &flatfile_mychan_find;

	/* CHANACS */
	chanacs_add = &flatfile_chanacs_add;
	chanacs_add_host = &flatfile_chanacs_add_host;
	chanacs_delete = &flatfile_chanacs_delete;
	chanacs_delete_host = &flatfile_chanacs_delete_host;
	chanacs_find = &flatfile_chanacs_find;
	chanacs_find_host = &flatfile_chanacs_find_host;
	chanacs_find_host_literal = &flatfile_chanacs_find_host_literal;

	/* METADATA */
	metadata_add = &flatfile_metadata_add;
	metadata_delete = &flatfile_metadata_delete;
	metadata_find = &flatfile_metadata_find;

	/* Other. */
	expire_check = &flatfile_expire_check;

	/* Ok, now that symbol relocation is completed,
	 * We want to set up the block allocator.
	 */
	metadata_heap = BlockHeapCreate(sizeof(metadata_t), HEAP_CHANUSER);
	kline_heap = BlockHeapCreate(sizeof(kline_t), 16);
	myuser_heap = BlockHeapCreate(sizeof(myuser_t), HEAP_USER);
	mychan_heap = BlockHeapCreate(sizeof(mychan_t), HEAP_CHANNEL);
	chanacs_heap = BlockHeapCreate(sizeof(chanacs_t), HEAP_CHANUSER);

	if (!metadata_heap || !kline_heap || !myuser_heap || !mychan_heap || !chanacs_heap)
	{
		slog(LG_INFO, "atheme: Block allocator failed.");
		exit(EXIT_FAILURE);
	}

	backend_loaded = TRUE;
}
