/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService TOPIC functions.
 *
 * $Id: topic.c 235 2005-05-30 03:13:49Z nenolod $
 */

#include "atheme.h"

void cs_topic(char *origin)
{
	char *chan = strtok(NULL, " ");
	char *topic = strtok(NULL, "");
	mychan_t *mc;
	channel_t *c;
	user_t *u;
	char hostbuf[BUFSIZE];

	if (!chan || !topic)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2TOPIC\2.");
		notice(chansvs.nick, origin, "Syntax: TOPIC <#channel> <topic>");
		return;
	}

	c = channel_find(chan);
	if (!c)
	{
                notice(chansvs.nick, origin, "No such channel: \2%s\2.", chan);
                return;
        }

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", chan);
		return;
	}

	u = user_find(origin);
	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	if ((u->myuser && !chanacs_find(mc, u->myuser, CA_TOPIC)) && (!chanacs_find_host(mc, hostbuf, CA_TOPIC)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	if (c->topic)
	{
		free(c->topic);
		free(c->topic_setter);
	}

	c->topic = sstrdup(topic);
	c->topic_setter = sstrdup(origin);

	topic_sts(chan, origin, topic);
	notice(chansvs.nick, origin, "Topic set to \2%s\2 on \2%s\2.", topic, chan);
}

void cs_topicappend(char *origin)
{
        char *chan = strtok(NULL, " ");
        char *topic = strtok(NULL, "");
        mychan_t *mc;
        user_t *u;
        char hostbuf[BUFSIZE];
	char topicbuf[BUFSIZE];
	channel_t *c;

        if (!chan || !topic)
        {
                notice(chansvs.nick, origin, "Insufficient parameters specified for \2TOPICAPPEND\2.");
                notice(chansvs.nick, origin, "Syntax: TOPICAPPEND <#channel> <topic>");
                return;
        }

	c = channel_find(chan);
	if (!c)
	{
                notice(chansvs.nick, origin, "No such channel: \2%s\2.", chan);
                return;
        }

        mc = mychan_find(chan);
        if (!mc)
        {
                notice(chansvs.nick, origin, "Channel \2%s\2 is not registered.", chan);
                return;
        }

        u = user_find(origin);
        strlcpy(hostbuf, u->nick, BUFSIZE);
        strlcat(hostbuf, "!", BUFSIZE);
        strlcat(hostbuf, u->user, BUFSIZE);
        strlcat(hostbuf, "@", BUFSIZE);
        strlcat(hostbuf, u->host, BUFSIZE);

        if ((u->myuser && !chanacs_find(mc, u->myuser, CA_TOPIC)) && (!chanacs_find_host(mc, hostbuf, CA_TOPIC)))
        {
                notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
                return;
        }

	topicbuf[0] = '\0';

	if (c->topic)
	{
		strlcpy(topicbuf, c->topic, BUFSIZE);
		strlcat(topicbuf, " | ", BUFSIZE);
		strlcat(topicbuf, topic, BUFSIZE);
		c->topic = sstrdup(topicbuf);
		c->topic_setter = sstrdup(origin);
	}
	else
	{
		strlcpy(topicbuf, topic, BUFSIZE);
		c->topic = sstrdup(topicbuf);
		c->topic_setter = sstrdup(origin);
	}

	topic_sts(chan, origin, topicbuf);
        notice(chansvs.nick, origin, "Topic set to \2%s\2 on \2%s\2.", c->topic, chan);
}
