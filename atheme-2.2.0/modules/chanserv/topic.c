/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService TOPIC functions.
 *
 * $Id: topic.c 8019 2007-04-02 09:27:15Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/topic", FALSE, _modinit, _moddeinit,
	"$Id: topic.c 8019 2007-04-02 09:27:15Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_topic(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_topicappend(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_topicprepend(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_topic = { "TOPIC", N_("Sets a topic on a channel."),
                        AC_NONE, 2, cs_cmd_topic };
command_t cs_topicappend = { "TOPICAPPEND", N_("Appends a topic on a channel."),
                        AC_NONE, 2, cs_cmd_topicappend };
command_t cs_topicprepend = { "TOPICPREPEND", N_("Prepends a topic on a channel."),
                        AC_NONE, 2, cs_cmd_topicprepend };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_topic, cs_cmdtree);
        command_add(&cs_topicappend, cs_cmdtree);
        command_add(&cs_topicprepend, cs_cmdtree);

	help_addentry(cs_helptree, "TOPIC", "help/cservice/topic", NULL);
	help_addentry(cs_helptree, "TOPICAPPEND", "help/cservice/topicappend", NULL);
	help_addentry(cs_helptree, "TOPICPREPEND", "help/cservice/topicprepend", NULL);
}

void _moddeinit()
{
	command_delete(&cs_topic, cs_cmdtree);
	command_delete(&cs_topicappend, cs_cmdtree);
	command_delete(&cs_topicprepend, cs_cmdtree);

	help_delentry(cs_helptree, "TOPIC");
	help_delentry(cs_helptree, "TOPICAPPEND");
	help_delentry(cs_helptree, "TOPICPREPEND");
}

static void cs_cmd_topic(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan = parv[0];
	char *topic = parv[1];
	mychan_t *mc;
	channel_t *c;
	char *topicsetter;
	time_t prevtopicts;

	if (!chan || !topic)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TOPIC");
		command_fail(si, fault_needmoreparams, _("Syntax: TOPIC <#channel> <topic>"));
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), chan);
		return;
	}
	
	c = channel_find(chan);
	if (!c)
	{
                command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), chan);
                return;
        }

	if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_TOPIC))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if (si->su != NULL)
		topicsetter = si->su->nick;
	else if (si->smu != NULL)
		topicsetter = si->smu->name;
	else
		topicsetter = "unknown";
	prevtopicts = c->topicts;
	handle_topic(c, topicsetter, CURRTIME, topic);
	topic_sts(c, topicsetter, CURRTIME, prevtopicts, topic);

	logcommand(si, CMDLOG_SET, "%s TOPIC", mc->name);
	if (!chanuser_find(c, si->su))
		command_success_nodata(si, _("Topic set to \2%s\2 on \2%s\2."), topic, chan);
}

static void cs_cmd_topicappend(sourceinfo_t *si, int parc, char *parv[])
{
        char *chan = parv[0];
        char *topic = parv[1];
        mychan_t *mc;
	char topicbuf[BUFSIZE];
	channel_t *c;
	char *topicsetter;
	time_t prevtopicts;

        if (!chan || !topic)
        {
                command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TOPICAPPEND");
                command_fail(si, fault_needmoreparams, _("Syntax: TOPICAPPEND <#channel> <topic>"));
                return;
        }

        mc = mychan_find(chan);
        if (!mc)
        {
                command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), chan);
                return;
        }

	c = channel_find(chan);
	if (!c)
	{
                command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), chan);
                return;
        }

        if (!chanacs_source_has_flag(mc, si, CA_TOPIC))
        {
                command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
                return;
        }
        
        if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
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
	prevtopicts = c->topicts;
	handle_topic(c, topicsetter, CURRTIME, topicbuf);
	topic_sts(c, topicsetter, CURRTIME, prevtopicts, topicbuf);

	logcommand(si, CMDLOG_SET, "%s TOPICAPPEND", mc->name);
	if (!chanuser_find(c, si->su))
        	command_success_nodata(si, _("Topic set to \2%s\2 on \2%s\2."), c->topic, chan);
}

static void cs_cmd_topicprepend(sourceinfo_t *si, int parc, char *parv[])
{
        char *chan = parv[0];
        char *topic = parv[1];
        mychan_t *mc;
	char topicbuf[BUFSIZE];
	channel_t *c;
	char *topicsetter;
	time_t prevtopicts;

        if (!chan || !topic)
        {
                command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TOPICPREPEND");
                command_fail(si, fault_needmoreparams, _("Syntax: TOPICPREPEND <#channel> <topic>"));
                return;
        }

        mc = mychan_find(chan);
        if (!mc)
        {
                command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), chan);
                return;
        }

	c = channel_find(chan);
	if (!c)
	{
                command_fail(si, fault_nosuch_target, _("\2%s\2 is currently empty."), chan);
                return;
        }

        if (!chanacs_source_has_flag(mc, si, CA_TOPIC))
        {
                command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
                return;
        }
        
        if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	topicbuf[0] = '\0';

	if (c->topic)
	{
		strlcpy(topicbuf, topic, BUFSIZE);
		strlcat(topicbuf, " | ", BUFSIZE);
		strlcat(topicbuf, c->topic, BUFSIZE);
	}
	else
		strlcpy(topicbuf, topic, BUFSIZE);

	if (si->su != NULL)
		topicsetter = si->su->nick;
	else if (si->smu != NULL)
		topicsetter = si->smu->name;
	else
		topicsetter = "unknown";
	prevtopicts = c->topicts;
	handle_topic(c, topicsetter, CURRTIME, topicbuf);
	topic_sts(c, topicsetter, CURRTIME, prevtopicts, topicbuf);

	logcommand(si, CMDLOG_SET, "%s TOPICPREPEND", mc->name);
	if (!chanuser_find(c, si->su))
        	command_success_nodata(si, _("Topic set to \2%s\2 on \2%s\2."), c->topic, chan);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
