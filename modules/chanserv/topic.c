/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService TOPIC functions.
 *
 * $Id: topic.c 6727 2006-10-20 18:48:53Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/topic", FALSE, _modinit, _moddeinit,
	"$Id: topic.c 6727 2006-10-20 18:48:53Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_topic(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_topicappend(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_topic = { "TOPIC", "Sets a topic on a channel.",
                        AC_NONE, 2, cs_cmd_topic };
command_t cs_topicappend = { "TOPICAPPEND", "Appends a topic on a channel.",
                        AC_NONE, 2, cs_cmd_topicappend };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_topic, cs_cmdtree);
        command_add(&cs_topicappend, cs_cmdtree);

	help_addentry(cs_helptree, "TOPIC", "help/cservice/topic", NULL);
	help_addentry(cs_helptree, "TOPICAPPEND", "help/cservice/topicappend", NULL);
}

void _moddeinit()
{
	command_delete(&cs_topic, cs_cmdtree);
	command_delete(&cs_topicappend, cs_cmdtree);

	help_delentry(cs_helptree, "TOPIC");
	help_delentry(cs_helptree, "TOPICAPPEND");
}

static void cs_cmd_topic(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *topic = parv[1];
	mychan_t *mc;
	channel_t *c;
	char *topicsetter;

	if (!chan || !topic)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TOPIC");
		command_fail(si, fault_needmoreparams, "Syntax: TOPIC <#channel> <topic>");
		return;
	}

	c = channel_find(chan);
	if (!c)
	{
                command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
                return;
        }

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "Channel \2%s\2 is not registered.", chan);
		return;
	}
	
	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_TOPIC))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
		return;
	}

	if (si->su != NULL)
		topicsetter = si->su->nick;
	else if (si->smu != NULL)
		topicsetter = si->smu->name;
	else
		topicsetter = "unknown";
	handle_topic(c, topicsetter, CURRTIME, topic);
	topic_sts(chan, topicsetter, CURRTIME, topic);

	logcommand(si, CMDLOG_SET, "%s TOPIC", mc->name);
	if (!chanuser_find(c, si->su))
		command_success_nodata(si, "Topic set to \2%s\2 on \2%s\2.", topic, chan);
}

static void cs_cmd_topicappend(sourceinfo_t *si, int parc, char *parv[])
{
        char *chan = parv[0];
        char *topic = parv[1];
        mychan_t *mc;
	char topicbuf[BUFSIZE];
	channel_t *c;
	char *topicsetter;

        if (!chan || !topic)
        {
                command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TOPICAPPEND");
                command_fail(si, fault_needmoreparams, "Syntax: TOPICAPPEND <#channel> <topic>");
                return;
        }

	c = channel_find(chan);
	if (!c)
	{
                command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", chan);
                return;
        }

        mc = mychan_find(chan);
        if (!mc)
        {
                command_fail(si, fault_nosuch_target, "Channel \2%s\2 is not registered.", chan);
                return;
        }

        if (!chanacs_source_has_flag(mc, si, CA_TOPIC))
        {
                command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
                return;
        }
        
        if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, "\2%s\2 is closed.", chan);
		return;
	}

	topicbuf[0] = '\0';

	if (c->topic)
	{
		strlcpy(topicbuf, c->topic, BUFSIZE);
		strlcat(topicbuf, " | ", BUFSIZE);
		strlcat(topicbuf, topic, BUFSIZE);
	}
	else
		strlcpy(topicbuf, topic, BUFSIZE);

	if (si->su != NULL)
		topicsetter = si->su->nick;
	else if (si->smu != NULL)
		topicsetter = si->smu->name;
	else
		topicsetter = "unknown";
	handle_topic(c, topicsetter, CURRTIME, topicbuf);
	topic_sts(chan, topicsetter, CURRTIME, topicbuf);

	logcommand(si, CMDLOG_SET, "%s TOPICAPPEND", mc->name);
	if (!chanuser_find(c, si->su))
        	command_success_nodata(si, "Topic set to \2%s\2 on \2%s\2.", c->topic, chan);
}

