/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains linked list functions.
 * Idea from ircd-hybrid.
 *
 * $Id: node.c 912 2005-07-17 06:10:36Z nenolod $
 */

#include "atheme.h"

list_t sralist;
list_t tldlist;
list_t uplinks;
list_t klnlist;
list_t sidlist[HASHSIZE];
list_t servlist[HASHSIZE];
list_t userlist[HASHSIZE];
list_t uidlist[HASHSIZE];
list_t chanlist[HASHSIZE];
list_t mclist[HASHSIZE];
list_t mulist[HASHSIZE];

list_t sendq;

static BlockHeap *node_heap;
static BlockHeap *sra_heap;
static BlockHeap *tld_heap;
static BlockHeap *serv_heap;
static BlockHeap *user_heap;
static BlockHeap *chan_heap;

static BlockHeap *chanuser_heap;
static BlockHeap *chanban_heap;
static BlockHeap *uplink_heap;

/*************
 * L I S T S *
 *************/

void init_nodes(void)
{
	node_heap = BlockHeapCreate(sizeof(node_t), HEAP_NODE);
	sra_heap = BlockHeapCreate(sizeof(sra_t), 2);
	tld_heap = BlockHeapCreate(sizeof(tld_t), 4);
	serv_heap = BlockHeapCreate(sizeof(server_t), HEAP_SERVER);
	user_heap = BlockHeapCreate(sizeof(user_t), HEAP_USER);
	chan_heap = BlockHeapCreate(sizeof(channel_t), HEAP_CHANNEL);
	chanuser_heap = BlockHeapCreate(sizeof(chanuser_t), HEAP_CHANUSER);
	chanban_heap = BlockHeapCreate(sizeof(chanban_t), HEAP_CHANUSER);
	uplink_heap = BlockHeapCreate(sizeof(uplink_t), 4);

	if (!node_heap || !tld_heap || !serv_heap || !user_heap || !chan_heap || !sra_heap || !chanuser_heap || !chanban_heap || !uplink_heap)
	{
		slog(LG_INFO, "init_nodes(): block allocator failed.");
		exit(EXIT_FAILURE);
	}
}

/* creates a new node */
node_t *node_create(void)
{
	node_t *n;

	/* allocate it */
	n = BlockHeapAlloc(node_heap);

	/* initialize */
	n->next = n->prev = n->data = NULL;

	/* up the count */
	cnt.node++;

	/* return a pointer to the new node */
	return n;
}

/* frees a node */
void node_free(node_t *n)
{
	/* free it */
	BlockHeapFree(node_heap, n);

	/* down the count */
	cnt.node--;
}

/* adds a node to the end of a list */
void node_add(void *data, node_t *n, list_t *l)
{
	node_t *tn;

	n->data = data;

	/* first node? */
	if (!l->head)
	{
		l->head = n;
		l->tail = NULL;
		l->count++;
		return;
	}

	/* find the last node */
	LIST_FOREACH_NEXT(tn, l->head);

	/* set the our `prev' to the last node */
	n->prev = tn;

	/* set the last node's `next' to us */
	n->prev->next = n;

	/* set the list's `tail' to us */
	l->tail = n;

	/* up the count */
	l->count++;
}

/* removes a node from a list */
void node_del(node_t *n, list_t *l)
{
	/* do we even have a node? */
	if (!n)
	{
		slog(LG_DEBUG, "node_del(): called with NULL node");
		return;
	}

	/* are we the head? */
	if (!n->prev)
		l->head = n->next;
	else
		n->prev->next = n->next;

	/* are we the tail? */
	if (!n->next)
		l->tail = n->prev;
	else
		n->next->prev = n->prev;

	/* down the count */
	l->count--;
}

/* finds a node by `data' */
node_t *node_find(void *data, list_t *l)
{
	node_t *n;

	LIST_FOREACH(n, l->head) if (n->data == data)
		return n;

	return NULL;
}

void node_move(node_t *m, list_t *oldlist, list_t *newlist)
{
	/* Assumption: If m->next == NULL, then list->tail == m
	 *      and:   If m->prev == NULL, then list->head == m
	 */
	if (m->next)
		m->next->prev = m->prev;
	else
		oldlist->tail = m->prev;

	if (m->prev)
		m->prev->next = m->next;
	else
		oldlist->head = m->next;

	m->prev = NULL;
	m->next = newlist->head;
	if (newlist->head != NULL)
		newlist->head->prev = m;
	else if (newlist->tail == NULL)
		newlist->tail = m;
	newlist->head = m;

	oldlist->count--;
	newlist->count++;
}

/*****************
 * U P L I N K S *
 *****************/

uplink_t *uplink_add(char *name, char *host, char *password, char *vhost, char *numeric, int port)
{
	uplink_t *u;
	node_t *n;

	slog(LG_DEBUG, "uplink_add(): %s -> %s:%d", me.name, name, port);

	if ((u = uplink_find(name)))
	{
		slog(LG_INFO, "Duplicate uplink %s.", name);
		return NULL;
	}

	u = BlockHeapAlloc(uplink_heap);
	u->name = sstrdup(name);
	u->host = sstrdup(host);
	u->pass = sstrdup(password);

	if (vhost)
		u->vhost = sstrdup(vhost);
	else
		u->vhost = sstrdup("0.0.0.0");

	if (numeric)
		u->numeric = sstrdup(numeric);
	else if (me.numeric)
		u->numeric = sstrdup(me.numeric);

	u->port = port;

	n = node_create();

	u->node = n;

	node_add(u, n, &uplinks);

	cnt.uplink++;

	return u;
}

void uplink_delete(uplink_t *u)
{
	node_t *n = node_find(u, &uplinks);

	free(u->name);
	free(u->host);
	free(u->pass);
	free(u->vhost);
	free(u->numeric);

	node_del(n, &uplinks);

	BlockHeapFree(uplink_heap, u);
}

uplink_t *uplink_find(char *name)
{
	node_t *n;

	LIST_FOREACH(n, uplinks.head)
	{
		uplink_t *u = n->data;

		if (!strcasecmp(u->name, name))
			return u;
	}

	return NULL;
}

/***********
 * S R A S *
 ***********/

sra_t *sra_add(char *name)
{
	sra_t *sra;
	myuser_t *mu = myuser_find(name);
	node_t *n = node_create();

	slog(LG_DEBUG, "sra_add(): %s", (mu) ? mu->name : name);

	sra = BlockHeapAlloc(sra_heap);

	node_add(sra, n, &sralist);

	if (mu)
	{
		sra->myuser = mu;
		mu->sra = sra;
	}
	else
		sra->name = sstrdup(name);

	cnt.sra++;

	return sra;
}

void sra_delete(myuser_t *myuser)
{
	sra_t *sra = sra_find(myuser);
	node_t *n;

	if (!sra)
	{
		slog(LG_DEBUG, "sra_delete(): called for nonexistant sra: %s", myuser->name);

		return;
	}

	slog(LG_DEBUG, "sra_delete(): %s", (sra->myuser) ? sra->myuser->name : sra->name);

	n = node_find(sra, &sralist);
	node_del(n, &sralist);
	node_free(n);

	if (sra->myuser)
		sra->myuser->sra = NULL;

	if (sra->name)
		free(sra->name);

	BlockHeapFree(sra_heap, sra);

	cnt.sra--;
}

sra_t *sra_find(myuser_t *myuser)
{
	sra_t *sra;
	node_t *n;

	LIST_FOREACH(n, sralist.head)
	{
		sra = (sra_t *)n->data;

		if (sra->myuser == myuser)
			return sra;
	}

	return NULL;
}

/***********
 * T L D S *
 ***********/

tld_t *tld_add(char *name)
{
	tld_t *tld;
	node_t *n = node_create();

	slog(LG_DEBUG, "tld_add(): %s", name);

	tld = BlockHeapAlloc(tld_heap);

	node_add(tld, n, &tldlist);

	tld->name = sstrdup(name);

	cnt.tld++;

	return tld;
}

void tld_delete(char *name)
{
	tld_t *tld = tld_find(name);
	node_t *n;

	if (!tld)
	{
		slog(LG_DEBUG, "tld_delete(): called for nonexistant tld: %s", name);

		return;
	}

	slog(LG_DEBUG, "tld_delete(): %s", tld->name);

	n = node_find(tld, &tldlist);
	node_del(n, &tldlist);
	node_free(n);

	free(tld->name);
	BlockHeapFree(tld_heap, tld);

	cnt.tld--;
}

tld_t *tld_find(char *name)
{
	tld_t *tld;
	node_t *n;

	LIST_FOREACH(n, tldlist.head)
	{
		tld = (tld_t *)n->data;

		if (!strcasecmp(name, tld->name))
			return tld;
	}

	return NULL;
}

/*****************
 * S E R V E R S *
 *****************/

server_t *server_add(char *name, uint8_t hops, char *id, char *desc)
{
	server_t *s;
	node_t *n = node_create();
	char *tld;

	slog(LG_DEBUG, "server_add(): %s", name);

	s = BlockHeapAlloc(serv_heap);

	s->hash = SHASH((unsigned char *)name);

	if (ircd->uses_uid == TRUE)
	{
		s->sid = sstrdup(id);
		s->shash = SHASH((unsigned char *)id);
		node_add(s, n, &sidlist[s->shash]);
	}

	node_add(s, n, &servlist[s->hash]);

	s->name = sstrdup(name);
	s->desc = sstrdup(desc);
	s->hops = hops;
	s->connected_since = CURRTIME;

	/* check to see if it's hidden */
	if ((desc[0] == '(') && (desc[1] == 'H') && (desc[2] == ')'))
		s->flags |= SF_HIDE;

	/* tld list for global noticer */
	tld = strrchr(name, '.');

	if (!tld_find(tld))
		tld_add(tld);

	cnt.server++;

	return s;
}

void server_delete(char *name)
{
	server_t *s = server_find(name);
	user_t *u;
	node_t *n, *tn;
	uint32_t i;

	if (!s)
	{
		slog(LG_DEBUG, "server_delete(): called for nonexistant server: %s", name);

		return;
	}

	slog(LG_DEBUG, "server_delete(): %s", s->name);

	/* first go through it's users and kill all of them */
	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH_SAFE(n, tn, userlist[i].head)
		{
			u = (user_t *)n->data;

			if (!strcasecmp(s->name, u->server->name))
				user_delete(u->nick);
		}
	}

	/* now remove the server */
	n = node_find(s, &servlist[s->hash]);
	node_del(n, &servlist[s->hash]);
	node_free(n);

	free(s->name);
	free(s->desc);
	BlockHeapFree(serv_heap, s);

	cnt.server--;
}

server_t *server_find(char *name)
{
	server_t *s;
	node_t *n;

	if (ircd->uses_uid == TRUE)
	{
		LIST_FOREACH(n, sidlist[SHASH((unsigned char *)name)].head)
		{
			s = (server_t *)n->data;

			if (!strcasecmp(name, s->sid))
				return s;
		}
	}

	LIST_FOREACH(n, servlist[SHASH((unsigned char *)name)].head)
	{
		s = (server_t *)n->data;

		if (!strcasecmp(name, s->name))
			return s;
	}

	return NULL;
}

/*************
 * U S E R S *
 *************/

user_t *user_add(char *nick, char *user, char *host, char *vhost, char *uid, server_t *server)
{
	user_t *u;
	node_t *n = node_create();

	slog(LG_DEBUG, "user_add(): %s (%s@%s) -> %s", nick, user, host, server->name);

	u = BlockHeapAlloc(user_heap);

	u->hash  = UHASH((unsigned char *)nick);

	if (ircd->uses_uid == TRUE)
	{
		u->uid  = sstrdup(uid);
		u->uhash = UHASH((unsigned char *)uid);
		node_add(u, n, &uidlist[u->uhash]);
	}

	node_add(u, n, &userlist[u->hash]);

	u->nick = sstrdup(nick);
	u->user = sstrdup(user);
	u->host = sstrdup(host);

	if (vhost)
		u->vhost = sstrdup(vhost);
	else
		u->vhost = sstrdup(host);

	u->server = server;

	u->server->users++;

	cnt.user++;

	hook_call_event("user_add", u);

	return u;
}

void user_delete(char *nick)
{
	user_t *u = user_find(nick);
	node_t *n, *tn;
	chanuser_t *cu;

	if (!u)
	{
		slog(LG_DEBUG, "user_delete(): called for nonexistant user: %s", nick);
		return;
	}

	slog(LG_DEBUG, "user_delete(): removing user: %s -> %s", u->nick, u->server->name);

	hook_call_event("user_delete", u);

	u->server->users--;

	/* remove the user from each channel */
	LIST_FOREACH_SAFE(n, tn, u->channels.head)
	{
		cu = (chanuser_t *)n->data;

		chanuser_delete(cu->chan, u);
	}

	/* set the user's myuser `identified' to FALSE if it exists */
	if (u->myuser)
		if (u->myuser->identified)
			u->myuser->identified = FALSE;

	n = node_find(u, &userlist[u->hash]);
	node_del(n, &userlist[u->hash]);
	node_free(n);

	if (u->myuser)
	{
		u->myuser->user = NULL;
		u->myuser = NULL;
	}

	if (u->nick)
		free(u->nick);

	if (u->user)
		free(u->user);

	if (u->host)
		free(u->host);

	BlockHeapFree(user_heap, u);

	cnt.user--;
}

user_t *user_find(char *nick)
{
	user_t *u;
	node_t *n;

	if (ircd->uses_uid == TRUE)
	{
		LIST_FOREACH(n, uidlist[SHASH((unsigned char *)nick)].head)
		{
			u = (user_t *)n->data;

			if (!irccasecmp(nick, u->uid))
				return u;
		}
	}

	LIST_FOREACH(n, userlist[SHASH((unsigned char *)nick)].head)
	{
		u = (user_t *)n->data;

		if (!irccasecmp(nick, u->nick))
			return u;
	}

	return NULL;
}

/*******************
 * C H A N N E L S *
 *******************/
channel_t *channel_add(char *name, uint32_t ts)
{
	channel_t *c;
	mychan_t *mc;
	node_t *n;

	if (*name != '#')
	{
		slog(LG_DEBUG, "channel_add(): got non #channel: %s", name);
		return NULL;
	}

	c = channel_find(name);

	if (c)
	{
		slog(LG_DEBUG, "channel_add(): channel already exists: %s", name);
		return c;
	}

	slog(LG_DEBUG, "channel_add(): %s", name);

	n = node_create();
	c = BlockHeapAlloc(chan_heap);

	c->name = sstrdup(name);
	c->ts = ts;
	c->hash = CHASH((unsigned char *)name);

	c->topic = NULL;
	c->topic_setter = NULL;

	c->bans.head = NULL;
	c->bans.tail = NULL;
	c->bans.count = 0;

	if ((mc = mychan_find(c->name)))
		mc->chan = c;

	node_add(c, n, &chanlist[c->hash]);

	cnt.chan++;

	if (!irccasecmp(config_options.chan, c->name))
		join(config_options.chan, chansvs.nick);

	hook_call_event("channel_add", c);

	return c;
}

void channel_delete(char *name)
{
	channel_t *c = channel_find(name);
	mychan_t *mc;
	node_t *n;

	if (!c)
	{
		slog(LG_DEBUG, "channel_delete(): called for nonexistant channel: %s", name);
		return;
	}

	slog(LG_DEBUG, "channel_delete(): %s", c->name);

	hook_call_event("channel_delete", c);

	/* we assume all lists should be null */

	n = node_find(c, &chanlist[c->hash]);
	node_del(n, &chanlist[c->hash]);
	node_free(n);

	if ((mc = mychan_find(c->name)))
		mc->chan = NULL;

	free(c->name);
	BlockHeapFree(chan_heap, c);

	cnt.chan--;
}

channel_t *channel_find(char *name)
{
	channel_t *c;
	node_t *n;
	uint32_t i;

	for (i = 0; i < HASHSIZE; i++)
	{
		LIST_FOREACH(n, chanlist[i].head)
		{
			c = (channel_t *)n->data;

			if (!irccasecmp(name, c->name))
				return c;
		}
	}

	return NULL;
}

/********************
 * C H A N  B A N S *
 ********************/

chanban_t *chanban_add(channel_t *chan, char *mask)
{
	chanban_t *c;
	node_t *n;

	c = chanban_find(chan, mask);

	if (c)
	{
		slog(LG_DEBUG, "chanban_add(): channel ban %s:%s already exists",
			chan->name, c->mask);
		return NULL;
	}

	slog(LG_DEBUG, "chanban_add(): %s:%s", chan->name, mask);

	n = node_create();
	c = BlockHeapAlloc(chanban_heap);

	c->chan = chan;
	c->mask = sstrdup(mask);

	node_add(c, n, &chan->bans);

	return c;
}

void chanban_delete(chanban_t *c)
{
	node_t *n;

	if (!c)
	{
		slog(LG_DEBUG, "chanban_delete(): called for nonexistant ban");
		return;
	}

	n = node_find(c, &c->chan->bans);
	node_del(n, &c->chan->bans);
	node_free(n);

	BlockHeapFree(chanban_heap, c);
}

chanban_t *chanban_find(channel_t *chan, char *mask)
{
	chanban_t *c;
	node_t *n;

	LIST_FOREACH(n, chan->bans.head)
	{
		c = n->data;

		if (!irccasecmp(c->mask, mask))
			return c;
	}

	return NULL;
}

/**********************
 * C H A N  U S E R S *
 **********************/

/*
 * Rewritten 06/23/05 by nenolod:
 *
 * Iterate through the list of prefix characters we know about.
 * Continue to do so until all prefixes are covered. Then add the
 * nick to the channel, with the privs he has acquired thus far.
 *
 * Once, and only once we have done that do we start in on checking
 * privileges. Otherwise we have a very inefficient way of doing
 * things. It worked fine for shrike, but the old code was restricted
 * to handling only @, @+ and + as prefixes.
 */
chanuser_t *chanuser_add(channel_t *chan, char *nick)
{
	user_t *u;
	node_t *n1;
	node_t *n2;
	chanuser_t *cu, *tcu;
	mychan_t *mc;
	uint32_t flags = 0;
	char hostbuf[BUFSIZE];
	int i = 0;

	if (*chan->name != '#')
	{
		slog(LG_DEBUG, "chanuser_add(): got non #channel: %s", chan->name);
		return NULL;
	}

	while (!(u = user_find(nick)) && *nick != '\0')
	{
		for (i = 0; prefix_mode_list[i].mode; i++)
			if (*nick == prefix_mode_list[i].mode)
				flags |= prefix_mode_list[i].value;
		nick++;
	}

	if (u == NULL)
	{
		slog(LG_DEBUG, "chanuser_add(): nonexist user: %s", nick);
		return NULL;
	}

	tcu = chanuser_find(chan, u);
	if (tcu != NULL)
	{
		slog(LG_DEBUG, "chanuser_add(): user is already present: %s -> %s", chan->name, u->nick);

		/* could be an OPME or other desyncher... */
		tcu->modes |= flags;

		return tcu;
	}

	slog(LG_DEBUG, "chanuser_add(): %s -> %s", chan->name, u->nick);

	n1 = node_create();
	n2 = node_create();

	cu = BlockHeapAlloc(chanuser_heap);

	cu->chan = chan;
	cu->user = u;
	cu->modes |= flags;

	chan->nummembers++;

	if ((chan->nummembers == 1) && (irccasecmp(config_options.chan, chan->name)))
	{
		if ((mc = mychan_find(chan->name)) && (config_options.join_chans))
		{
			join(chan->name, chansvs.nick);
			mc->used = CURRTIME;
		}
	}

	node_add(cu, n1, &chan->members);
	node_add(cu, n2, &u->channels);

	cnt.chanuser++;

	/* If they are attached to me, we are done here. */
	if (u->server == me.me || u->server == NULL)
		return cu;

	/* auto stuff */
	if (((mc = mychan_find(chan->name))) && (u->myuser))
	{
		if ((mc->flags & MC_STAFFONLY) && !is_ircop(u) && !is_sra(u->myuser))
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "You are not authorized to be on this channel");
		}

		if (should_kick(mc, u->myuser))
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "User is banned from this channel");
		}

		if (should_owner(mc, u->myuser))
		{
			if (ircd->uses_owner)
			{
				cmode(chansvs.nick, chan->name, ircd->owner_mchar, CLIENT_NAME(u));
				cu->modes |= ircd->owner_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->owner_mode) && ircd->uses_owner)
		{
			char *mbuf = sstrdup(ircd->owner_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->owner_mode;

			free(mbuf);
		}

		if (should_protect(mc, u->myuser))
		{
			if (ircd->uses_protect)
			{
				cmode(chansvs.nick, chan->name, ircd->protect_mchar, CLIENT_NAME(u));
				cu->modes |= ircd->protect_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->protect_mode) && ircd->uses_protect)
		{
			char *mbuf = sstrdup(ircd->protect_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->protect_mode;

			free(mbuf);
		}

		if (should_op(mc, u->myuser))
		{
			cmode(chansvs.nick, chan->name, "+o", CLIENT_NAME(u));
			cu->modes |= CMODE_OP;
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & CMODE_OP))
		{
			cmode(chansvs.nick, chan->name, "-o", CLIENT_NAME(u));
			cu->modes &= ~CMODE_OP;
		}

		if (should_halfop(mc, u->myuser))
		{
			if (ircd->uses_halfops)
			{
				cmode(chansvs.nick, chan->name, "+h", CLIENT_NAME(u));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && (cu->modes & ircd->halfops_mode) && ircd->uses_halfops)
		{
			cmode(chansvs.nick, chan->name, "-h", CLIENT_NAME(u));
			cu->modes &= ~ircd->halfops_mode;
		}

		if (should_voice(mc, u->myuser))
		{
			cmode(chansvs.nick, chan->name, "+v", CLIENT_NAME(u));
			cu->modes |= CMODE_VOICE;
		}
		else if (cu->modes & CMODE_VOICE)
		{
			cmode(chansvs.nick, chan->name, "-v", CLIENT_NAME(u));
			cu->modes &= ~CMODE_VOICE;
		}
	}

	if (mc)
	{
		hostbuf[0] = '\0';

		strlcat(hostbuf, u->nick, BUFSIZE);
		strlcat(hostbuf, "!", BUFSIZE);
		strlcat(hostbuf, u->user, BUFSIZE);
		strlcat(hostbuf, "@", BUFSIZE);
		strlcat(hostbuf, u->host, BUFSIZE);

		if ((mc->flags & MC_STAFFONLY) && !is_ircop(u))
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "You are not authorized to be on this channel");
		}


		if (should_kick_host(mc, hostbuf))
		{
			ban(chansvs.nick, chan->name, u);
			kick(chansvs.nick, chan->name, u->nick, "User is banned from this channel");
		}

		if (!should_owner(mc, u->myuser) && (cu->modes & ircd->owner_mode))
		{
			char *mbuf = sstrdup(ircd->owner_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->owner_mode;

			free(mbuf);
		}

		if (!should_protect(mc, u->myuser) && (cu->modes & ircd->protect_mode))
		{
			char *mbuf = sstrdup(ircd->protect_mchar);
			*mbuf = '-';

			cmode(chansvs.nick, chan->name, mbuf, CLIENT_NAME(u));
			cu->modes &= ~ircd->protect_mode;

			free(mbuf);
		}

		if (should_op_host(mc, hostbuf))
		{
			if (!(cu->modes & CMODE_OP))
			{
				cmode(chansvs.nick, chan->name, "+o", CLIENT_NAME(u));
				cu->modes |= CMODE_OP;
			}
		}
		else if ((mc->flags & MC_SECURE) && !should_op(mc, u->myuser) && (cu->modes & CMODE_OP))
		{
			cmode(chansvs.nick, chan->name, "-o", CLIENT_NAME(u));
			cu->modes &= ~CMODE_OP;
		}

		if (should_halfop_host(mc, hostbuf))
		{
			if (ircd->uses_halfops && !(cu->modes & ircd->halfops_mode))
			{
				cmode(chansvs.nick, chan->name, "+h", CLIENT_NAME(u));
				cu->modes |= ircd->halfops_mode;
			}
		}
		else if ((mc->flags & MC_SECURE) && ircd->uses_halfops && !should_halfop(mc, u->myuser) && (cu->modes & ircd->halfops_mode))
		{
			cmode(chansvs.nick, chan->name, "-h", CLIENT_NAME(u));
			cu->modes &= ~ircd->halfops_mode;
		}

		if (should_voice_host(mc, hostbuf))
		{
			if (!(cu->modes & CMODE_VOICE))
			{
				cmode(chansvs.nick, chan->name, "+v", CLIENT_NAME(u));
				cu->modes |= CMODE_VOICE;
			}
		}

		if (mc->url)
			numeric_sts(me.name, 328, u->nick, "%s :%s", mc->name, mc->url);

		if (mc->entrymsg)
			notice(chansvs.nick, u->nick, "[%s] %s", mc->name, mc->entrymsg);
	}

	hook_call_event("channel_join", cu);

	return cu;
}

void chanuser_delete(channel_t *chan, user_t *user)
{
	chanuser_t *cu;
	node_t *n, *tn, *n2;

	LIST_FOREACH_SAFE(n, tn, chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == user)
		{
			slog(LG_DEBUG, "chanuser_delete(): %s -> %s (%d)", cu->chan->name, cu->user->nick, cu->chan->nummembers - 1);
			node_del(n, &chan->members);
			node_free(n);

			n2 = node_find(cu, &user->channels);
			node_del(n2, &user->channels);
			node_free(n2);

			chan->nummembers--;
			cnt.chanuser--;

			if (chan->nummembers == 1)
			{
				if (config_options.leave_chans)
					part(chan->name, chansvs.nick);
			}

			else if (chan->nummembers == 0)
			{
				/* empty channels die */
				slog(LG_DEBUG, "chanuser_delete(): `%s' is empty, removing", chan->name);

				channel_delete(chan->name);
			}

			return;
		}
	}
}

chanuser_t *chanuser_find(channel_t *chan, user_t *user)
{
	node_t *n;
	chanuser_t *cu;

	if ((!chan) || (!user))
		return NULL;

	LIST_FOREACH(n, chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if (cu->user == user)
			return cu;
	}

	return NULL;
}

/*********
 * SENDQ *
 *********/

void sendq_add(char *buf, int len, int pos)
{
	node_t *n = node_create();
	struct sendq *sq = smalloc(sizeof(struct sendq));

	slog(LG_DEBUG, "sendq_add(): triggered");

	sq->buf = sstrdup(buf);
	sq->len = len - pos;
	sq->pos = pos;
	node_add(sq, n, &sendq);
}

int sendq_flush(void)
{
	node_t *n, *tn;
	struct sendq *sq;
	int l;

	LIST_FOREACH_SAFE(n, tn, sendq.head)
	{
		sq = (struct sendq *)n->data;

		if ((l = write(servsock, sq->buf + sq->pos, sq->len)) == -1)
		{
			if (errno != EAGAIN)
				return -1;

			return 0;
		}

		if (l == sq->len)
		{
			node_del(n, &sendq);
			free(sq->buf);
			free(sq);
			node_free(n);
		}
		else
		{
			sq->pos += l;
			sq->len -= l;
			return 0;
		}
	}

	return 1;
}
