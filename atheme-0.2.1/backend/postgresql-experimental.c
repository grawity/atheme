/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains the implementation of the database
 * using PostgreSQL. It is as of yet, not realtime.
 *
 * $Id: postgresql-experimental.c 1036 2005-07-18 22:48:18Z alambert $
 */

#include "atheme.h"
#include <libpq-fe.h>

static BlockHeap *kline_heap;		/* 16 */
static BlockHeap *myuser_heap;		/* HEAP_USER */
static BlockHeap *mychan_heap;		/* HEAP_CHANNEL */
static BlockHeap *chanacs_heap; 	/* HEAP_CHANACS */
static BlockHeap *metadata_heap;	/* HEAP_CHANUSER */
PGconn *pq;

/*
 * Function to prevent SQL injection attacks, by properly escaping
 * the SQL command. This returns the postgresql result that is executed.
 * It also handles bounds checking.
 *
 * Returns a PGresult on success.
 * Returns NULL on failure, and logs the error if debug is enabled.
 */
static PGresult *safe_query(const char *string, ...)
{
	va_list args;
	char buf[BUFSIZE * 3];
	PGresult *res;

	va_start(args, string);
	vsnprintf(buf, BUFSIZE * 3, string, args);
	va_end(args);

	slog(LG_DEBUG, "executing: %s", buf);

	res = PQexec(pq, buf);

	if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		slog(LG_DEBUG, "There was an error executing the query:");
		slog(LG_DEBUG, "       %s", PQresultErrorMessage(res));

		wallops("\2DATABASE ERROR\2: %s", PQresultErrorMessage(res));

		return NULL;
	}

	return res;
}

/*
 * Writes a clean snapshot of Atheme's state to the SQL database.
 * This function is destructive, and will be called automagically
 * until rc6.
 *
 * So do not depend on your transactions staying in the database
 * if you are running Atheme 0.2rc5.
 */
static void postgresql_db_save(void *arg)
{
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	kline_t *k;
	node_t *n, *tn;
	uint32_t i, ii, iii;
	PGresult *res;

	slog(LG_DEBUG, "db_save(): saving myusers");

	ii = 0;

	/* clear everything out. */
	safe_query("DELETE FROM ACCOUNTS;");
	safe_query("DELETE FROM ACCOUNT_METADATA;");
	safe_query("DELETE FROM CHANNELS;");
	safe_query("DELETE FROM CHANNEL_METADATA;");
	safe_query("DELETE FROM CHANNEL_ACCESS;");
	safe_query("DELETE FROM KLINES;");

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mulist[i].head)
		{
			char user[BUFSIZE];
			char pass[BUFSIZE];
			char email[BUFSIZE];

			mu = (myuser_t *)n->data;

			PQescapeString(user, mu->name, BUFSIZE);
			PQescapeString(pass, mu->pass, BUFSIZE);
			PQescapeString(email, mu->email, BUFSIZE);

			res = safe_query("INSERT INTO ACCOUNTS(ID, USERNAME, PASSWORD, EMAIL, REGISTERED, LASTLOGIN, "
					"FAILNUM, LASTFAIL, LASTFAILON, FLAGS, KEY) VALUES (%d, '%s', '%s', '%s', %ld, %ld, %ld, "
					"'%s', %ld, %ld, %ld);", ii, user, pass, email, (long) mu->registered, (long) mu->lastlogin,
					(long) mu->failnum, mu->lastfail ? mu->lastfail : "", (long) mu->lastfailon, (long) mu->flags, (long) mu->key);

			LIST_FOREACH(tn, mu->metadata.head)
			{
				char key[BUFSIZE], keyval[BUFSIZE];
				metadata_t *md = (metadata_t *)tn->data;

				PQescapeString(key, md->name, BUFSIZE);
				PQescapeString(keyval, md->value, BUFSIZE);

				res = safe_query("INSERT INTO ACCOUNT_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES ("
						"DEFAULT, %d, '%s', '%s');", ii, key, keyval);
			}

			ii++;
		}
	}

	slog(LG_DEBUG, "db_save(): saving mychans");

	ii = 0;
	iii = 0;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, mclist[i].head)
		{
			mc = (mychan_t *)n->data;

			res = safe_query("INSERT INTO CHANNELS(ID, NAME, PASSWORD, FOUNDER, REGISTERED, LASTUSED, "
					"FLAGS, MLOCK_ON, MLOCK_OFF, MLOCK_LIMIT, MLOCK_KEY, URL, ENTRYMSG) VALUES ("
					"%d, '%s', '%s', '%s', %ld, %ld, %ld, %ld, %ld, %ld, '%s', '%s', '%s');", ii,
					mc->name, mc->pass, mc->founder->name, (long)mc->registered, (long)mc->used,
					(long)mc->flags, (long)mc->mlock_on, (long)mc->mlock_off, (long)mc->mlock_limit,
					mc->mlock_key ? mc->mlock_key : "", mc->url ? mc->url : "",
					mc->entrymsg ? mc->entrymsg : "");

			LIST_FOREACH(tn, mc->chanacs.head)
			{
				ca = (chanacs_t *)tn->data;

				res = safe_query("INSERT INTO CHANNEL_ACCESS(ID, PARENT, ACCOUNT, PERMISSIONS) VALUES ("
						"%d, %d, '%s', '%s');", iii, ii, (ca->host) ? ca->host : ca->myuser->name,
						bitmask_to_flags(ca->level, chanacs_flags));

				iii++;
			}

			LIST_FOREACH(tn, mc->metadata.head)
			{
				char key[BUFSIZE], keyval[BUFSIZE];
				metadata_t *md = (metadata_t *)tn->data;

				PQescapeString(key, md->name, BUFSIZE);
				PQescapeString(keyval, md->value, BUFSIZE);

				res = safe_query("INSERT INTO CHANNEL_METADATA(ID, PARENT, KEYNAME, VALUE) VALUES ("
						"DEFAULT, %d, '%s', '%s');", ii, key, keyval);
			}

			ii++;
		}
	}

	slog(LG_DEBUG, "db_save(): saving klines");

	ii = 0;

	LIST_FOREACH(n, klnlist.head)
	{
		k = (kline_t *)n->data;

		res = safe_query("INSERT INTO KLINES(ID, USERNAME, HOSTNAME, DURATION, SETTIME, SETTER, REASON) VALUES ("
				"%d, '%s', '%s', %ld, %ld, '%s', '%s');", ii, k->user, k->host, k->duration, (long) k->settime,
				k->setby, k->reason);

		ii++;
	}
}

/* loads atheme.db */
static void postgresql_db_load(void)
{
	myuser_t *mu;
	node_t *n;
	sra_t *sra;
	mychan_t *mc;
	kline_t *k;
	uint32_t i = 0, muin = 0, mcin = 0, kin = 0;
	char dbcredentials[BUFSIZE];
	PGresult *res;

	if (!database_options.port)
		snprintf(dbcredentials, BUFSIZE, "host=%s dbname=%s user=%s password=%s",
				database_options.host, database_options.database, database_options.user, database_options.pass);
	else
		snprintf(dbcredentials, BUFSIZE, "host=%s dbname=%s user=%s password=%s port=%d",
				database_options.host, database_options.database, database_options.user, database_options.pass,
				database_options.port);

	slog(LG_DEBUG, "Connecting to PostgreSQL provider using these credentials:");
	slog(LG_DEBUG, "      %s", dbcredentials); 

	/* Open the db connection up. */
	pq = PQconnectdb(dbcredentials);

	res = safe_query("SELECT * FROM ACCOUNTS;");
	muin = PQntuples(res);

	slog(LG_DEBUG, "db_load(): Got %d myusers from SQL.", muin);

	for (i = 0; i < muin; i++)
	{
		char *muname, *mupass, *muemail;
		uint32_t uid, umd, ii;
		PGresult *res2;

		uid = atoi(PQgetvalue(res, i, 0));
		muname = PQgetvalue(res, i, 1);
		mupass = PQgetvalue(res, i, 2);
		muemail = PQgetvalue(res, i, 3);

		mu = myuser_add(muname, mupass, muemail);

		mu->registered = atoi(PQgetvalue(res, i, 4));
		mu->lastlogin = atoi(PQgetvalue(res, i, 5));
		mu->failnum = atoi(PQgetvalue(res, i, 6));
		mu->lastfail = sstrdup(PQgetvalue(res, i, 7));
		mu->lastfailon = atol(PQgetvalue(res, i, 8));
		mu->flags = atoi(PQgetvalue(res, i, 9));

		if (atoi(PQgetvalue(res, i, 10)))
			mu->key = atoi(PQgetvalue(res, i, 10));

		res2 = safe_query("SELECT * FROM ACCOUNT_METADATA WHERE PARENT=%d;", uid);
		umd = PQntuples(res2);

		for (ii = 0; ii < umd; ii++)
			metadata_add(mu, METADATA_USER, PQgetvalue(res2, ii, 2), PQgetvalue(res2, ii, 3));

		PQclear(res2);
	}

	PQclear(res);
	res = safe_query("SELECT * FROM CHANNELS;");
	mcin = PQntuples(res);

	for (i = 0; i < mcin; i++)
	{
		char *mcname, *mcpass;
		uint32_t cain, mdin, ii;
		PGresult *res2;

		mcname = PQgetvalue(res, i, 1);
		mcpass = PQgetvalue(res, i, 2);

		mc = mychan_add(mcname, mcpass);

		mc->founder = myuser_find(PQgetvalue(res, i, 3));

		if (!mc->founder)
		{
			slog(LG_DEBUG, "db_load(): channel %s has no founder, dropping.", mc->name);
			mychan_delete(mc->name);
		}

		mc->registered = atoi(PQgetvalue(res, i, 4));
		mc->used = atoi(PQgetvalue(res, i, 5));
		mc->flags = atoi(PQgetvalue(res, i, 6));

		mc->mlock_on = atoi(PQgetvalue(res, i, 7));
		mc->mlock_off = atoi(PQgetvalue(res, i, 8));
		mc->mlock_limit = atoi(PQgetvalue(res, i, 9));
		mc->mlock_key = sstrdup(PQgetvalue(res, i, 10));

                mc->url = sstrdup(PQgetvalue(res, i, 11));
                mc->entrymsg = sstrdup(PQgetvalue(res, i, 12));

		/* SELECT * FROM CHANNEL_ACCESS WHERE PARENT=21 */
		res2 = safe_query("SELECT * FROM CHANNEL_ACCESS WHERE PARENT=%s;", PQgetvalue(res, i, 0));
		cain = PQntuples(res2);

		for (ii = 0; ii < cain; ii++)
		{
			char *username, *permissions;
			uint32_t fl;

			username = PQgetvalue(res2, ii, 2);
			permissions = PQgetvalue(res2, ii, 3);
			fl = flags_to_bitmask(permissions, chanacs_flags, 0x0);

			mu = myuser_find(username);

			if (!mu)
				chanacs_add_host(mc, username, fl);
			else
				chanacs_add(mc, mu, fl);
		}

		PQclear(res2);

		/* SELECT * FROM CHANNEL_METADATA WHERE PARENT=21 */
		res2 = safe_query("SELECT * FROM CHANNEL_METADATA WHERE PARENT=%s;", PQgetvalue(res, i, 0));
		mdin = PQntuples(res2);

		for (ii = 0; ii < mdin; ii++)
			metadata_add(mc, METADATA_CHANNEL, PQgetvalue(res2, ii, 2), PQgetvalue(res2, ii, 3));

		PQclear(res2);
	}

	PQclear(res);

	res = safe_query("SELECT * FROM KLINES;");
	kin = PQntuples(res);

	for (i = 0; i < kin; i++)
	{
		char *user, *host, *reason, *setby;
		uint32_t duration, settime;

		user = PQgetvalue(res, i, 1);
		host = PQgetvalue(res, i, 2);
		setby = PQgetvalue(res, i, 5);
		reason = PQgetvalue(res, i, 6);

		duration = atoi(PQgetvalue(res, i, 3));
		settime = atoi(PQgetvalue(res, i, 4));

		k = kline_add(user, host, reason, duration);
		k->settime = settime;
		k->setby = sstrdup(setby);
	}

	PQclear(res);

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
}

static kline_t *postgresql_kline_add(char *user, char *host, char *reason, long duration)
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

static void postgresql_kline_delete(char *user, char *host)
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

static kline_t *postgresql_kline_find(char *user, char *host)
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

static kline_t *postgresql_kline_find_num(uint32_t number)
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
 
static void postgresql_kline_expire(void *arg)
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

static myuser_t *postgresql_myuser_add(char *name, char *pass, char *email)
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

static void postgresql_myuser_delete(char *name)
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

static myuser_t *postgresql_myuser_find(char *name)
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

static mychan_t *postgresql_mychan_add(char *name, char *pass)
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

static void postgresql_mychan_delete(char *name)
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

static mychan_t *postgresql_mychan_find(char *name)
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

static chanacs_t *postgresql_chanacs_add(mychan_t *mychan, myuser_t *myuser, uint32_t level)
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

static chanacs_t *postgresql_chanacs_add_host(mychan_t *mychan, char *host, uint32_t level)
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

static void postgresql_chanacs_delete(mychan_t *mychan, myuser_t *myuser, uint32_t level)
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

static void postgresql_chanacs_delete_host(mychan_t *mychan, char *host, uint32_t level)
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

static chanacs_t *postgresql_chanacs_find(mychan_t *mychan, myuser_t *myuser, uint32_t level)
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

static chanacs_t *postgresql_chanacs_find_host(mychan_t *mychan, char *host, uint32_t level)
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
                        if ((ca->host) && (!match(ca->host, host)) && (ca->level & level))
                                return ca;
                }
                else if ((ca->host) && (!match(ca->host, host)))
                        return ca;
        }
         
        return NULL;
}

static chanacs_t *postgresql_chanacs_find_host_literal(mychan_t *mychan, char *host, uint32_t level)
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
                        if ((ca->host) && (!strcasecmp(ca->host, host)) && (ca->level & level))
                                return ca;
                }
                else if ((ca->host) && (!strcasecmp(ca->host, host)))
                        return ca;
        }
         
        return NULL;
}

static metadata_t *postgresql_metadata_add(void *target, int32_t type, char *name, char *value)
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
                metadata_delete(target, type, md);

        md = BlockHeapAlloc(metadata_heap);

        md->name = sstrdup(name);
        md->value = sstrdup(value);

        n = node_create();
 
        if (type == METADATA_USER)
                node_add(md, n, &mu->metadata);
        else
                node_add(md, n, &mc->metadata);

        return md;
}

static void postgresql_metadata_delete(void *target, int32_t type, metadata_t *md)
{
        node_t *n;
        myuser_t *mu;   
        mychan_t *mc;

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

static metadata_t *postgresql_metadata_find(void *target, int32_t type, char *name)
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

static void postgresql_expire_check(void *arg)
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

							if (config_options.chan && irccasecmp(mc->name, config_options.chan) || !config_options.chan)
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

	db_load = &postgresql_db_load;
	db_save = &postgresql_db_save;

	/* KLINES */
	kline_add = &postgresql_kline_add;
	kline_delete = &postgresql_kline_delete;
	kline_find = &postgresql_kline_find;
	kline_find_num = &postgresql_kline_find_num;
	kline_expire = &postgresql_kline_expire;

	/* MYUSER */
	myuser_add = &postgresql_myuser_add;
	myuser_delete = &postgresql_myuser_delete;
	myuser_find = &postgresql_myuser_find;

	/* MYCHAN */
	mychan_add = &postgresql_mychan_add;
	mychan_delete = &postgresql_mychan_delete;
	mychan_find = &postgresql_mychan_find;

	/* CHANACS */
	chanacs_add = &postgresql_chanacs_add;
	chanacs_add_host = &postgresql_chanacs_add_host;
	chanacs_delete = &postgresql_chanacs_delete;
	chanacs_delete_host = &postgresql_chanacs_delete_host;
	chanacs_find = &postgresql_chanacs_find;
	chanacs_find_host = &postgresql_chanacs_find_host;
	chanacs_find_host_literal = &postgresql_chanacs_find_host_literal;

	/* METADATA */
	metadata_add = &postgresql_metadata_add;
	metadata_delete = &postgresql_metadata_delete;
	metadata_find = &postgresql_metadata_find;

	/* Other. */
	expire_check = &postgresql_expire_check;

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
