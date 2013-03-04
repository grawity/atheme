/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 6895 2006-10-22 21:07:24Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 6895 2006-10-22 21:07:24Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_help_set(sourceinfo_t *si);
static void cs_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

static void cs_cmd_set_email(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_url(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_entrymsg(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_founder(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_mlock(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_keeptopic(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_topiclock(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_secure(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_verbose(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_staffonly(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_set_property(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_set = { "SET", "Sets various control flags.",
                        AC_NONE, 3, cs_cmd_set };

command_t cs_set_founder   = { "FOUNDER",   "Transfers foundership of a channel.",                          AC_NONE, 2, cs_cmd_set_founder    };
command_t cs_set_mlock     = { "MLOCK",     "Sets channel mode lock.",                                      AC_NONE, 2, cs_cmd_set_mlock      };
command_t cs_set_secure    = { "SECURE",    "Prevents unauthorized users from gaining operator status.",    AC_NONE, 2, cs_cmd_set_secure     };
command_t cs_set_verbose   = { "VERBOSE",   "Notifies channel about access list modifications.",            AC_NONE, 2, cs_cmd_set_verbose    };
command_t cs_set_url       = { "URL",       "Sets the channel URL.",                                        AC_NONE, 2, cs_cmd_set_url        };
command_t cs_set_entrymsg  = { "ENTRYMSG",  "Sets the channel's entry message.",                            AC_NONE, 2, cs_cmd_set_entrymsg   };
command_t cs_set_property  = { "PROPERTY",  "Manipulates channel metadata.",                                AC_NONE, 2, cs_cmd_set_property   };
command_t cs_set_email     = { "EMAIL",     "Sets the channel e-mail address.",                             AC_NONE, 2, cs_cmd_set_email      };
command_t cs_set_keeptopic = { "KEEPTOPIC", "Enables topic retention.",                                     AC_NONE, 2, cs_cmd_set_keeptopic  };
command_t cs_set_topiclock = { "TOPICLOCK", "Restricts who can change the topic.",                          AC_NONE, 2, cs_cmd_set_topiclock  };
command_t cs_set_fantasy   = { "FANTASY",   "Allows or disallows in-channel commands.",                     AC_NONE, 2, cs_cmd_set_fantasy    };
command_t cs_set_staffonly = { "STAFFONLY", "Sets the channel as staff-only. (Non staff is kickbanned.)",   PRIV_CHAN_ADMIN, 2, cs_cmd_set_staffonly  };

command_t *cs_set_commands[] = {
	&cs_set_founder,
	&cs_set_mlock,
	&cs_set_secure,
	&cs_set_verbose,
	&cs_set_url,
	&cs_set_entrymsg,
	&cs_set_property,
	&cs_set_email,
	&cs_set_keeptopic,
	&cs_set_topiclock,
	&cs_set_fantasy,
	&cs_set_staffonly,
	NULL
};

list_t *cs_cmdtree;
list_t *cs_helptree;
list_t cs_set_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_set, cs_cmdtree);
	command_add_many(cs_set_commands, &cs_set_cmdtree);

	help_addentry(cs_helptree, "SET", NULL, cs_help_set);
	help_addentry(cs_helptree, "SET FOUNDER", "help/cservice/set_founder", NULL);
	help_addentry(cs_helptree, "SET MLOCK", "help/cservice/set_mlock", NULL);
	help_addentry(cs_helptree, "SET SECURE", "help/cservice/set_secure", NULL);
	help_addentry(cs_helptree, "SET VERBOSE", "help/cservice/set_verbose", NULL);
	help_addentry(cs_helptree, "SET URL", "help/cservice/set_url", NULL);
	help_addentry(cs_helptree, "SET EMAIL", "help/cservice/set_email", NULL);
	help_addentry(cs_helptree, "SET ENTRYMSG", "help/cservice/set_entrymsg", NULL);
	help_addentry(cs_helptree, "SET PROPERTY", "help/cservice/set_property", NULL);
	help_addentry(cs_helptree, "SET STAFFONLY", "help/cservice/set_staffonly", NULL);
	help_addentry(cs_helptree, "SET KEEPTOPIC", "help/cservice/set_keeptopic", NULL);
	help_addentry(cs_helptree, "SET TOPICLOCK", "help/cservice/set_topiclock", NULL);
	help_addentry(cs_helptree, "SET FANTASY", "help/cservice/set_fantasy", NULL);
}

void _moddeinit()
{
	command_delete(&cs_set, cs_cmdtree);
	command_delete_many(cs_set_commands, &cs_set_cmdtree);

	help_delentry(cs_helptree, "SET");
	help_delentry(cs_helptree, "SET FOUNDER");
	help_delentry(cs_helptree, "SET MLOCK");
	help_delentry(cs_helptree, "SET SECURE");
	help_delentry(cs_helptree, "SET VERBOSE");
	help_delentry(cs_helptree, "SET URL");
	help_delentry(cs_helptree, "SET EMAIL");
	help_delentry(cs_helptree, "SET ENTRYMSG");
	help_delentry(cs_helptree, "SET PROPERTY");
	help_delentry(cs_helptree, "SET STAFFONLY");
	help_delentry(cs_helptree, "SET KEEPTOPIC");
	help_delentry(cs_helptree, "SET TOPICLOCK");
	help_delentry(cs_helptree, "SET FANTASY");
}

static void cs_help_set(sourceinfo_t *si)
{
	command_success_nodata(si, "Help for \2SET\2:");
	command_success_nodata(si, " ");
	command_success_nodata(si, "SET allows you to set various control flags");
	command_success_nodata(si, "for channels that change the way certain");
	command_success_nodata(si, "operations are performed on them.");
	command_success_nodata(si, " ");
	command_help(si, &cs_set_cmdtree);
	command_success_nodata(si, " ");
	command_success_nodata(si, "For more specific help use \2/msg %s HELP SET \37command\37\2.", si->service->disp);
}

/* SET <#channel> <setting> <parameters> */
static void cs_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;

	if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, "Syntax: SET <#channel> <setting> <parameters>");
		return;
	}

	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET");
		command_fail(si, fault_badparams, "Syntax: SET <#channel> <setting> <parameters>");
		return;
	}

	c = command_find(&cs_set_cmdtree, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, "Invalid command. Use \2/%s%s help\2 for a command listing.", (ircd->uses_rcommand == FALSE) ? "msg " : "", si->service->disp);
		return;
	}

	parv[1] = chan;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static void cs_cmd_set_email(sourceinfo_t *si, int parc, char *parv[])
{
        mychan_t *mc;
        char *mail = parv[1];

        if (!(mc = mychan_find(parv[0])))
        {
                command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
                return;
        }

        if (!chanacs_source_has_flag(mc, si, CA_SET))
        {
                command_fail(si, fault_noprivs, "You are not authorized to execute this command.");
                return;
        }

        if (!mail || !strcasecmp(mail, "NONE") || !strcasecmp(mail, "OFF"))
        {
                if (metadata_find(mc, METADATA_CHANNEL, "email"))
                {
                        metadata_delete(mc, METADATA_CHANNEL, "email");
                        command_success_nodata(si, "The e-mail address for \2%s\2 was deleted.", mc->name);
			logcommand(si, CMDLOG_SET, "%s SET EMAIL NONE", mc->name);
                        return;
                }

                command_fail(si, fault_nochange, "The e-mail address for \2%s\2 was not set.", mc->name);
                return;
        }

        if (strlen(mail) >= EMAILLEN)
        {
                command_fail(si, fault_badparams, STR_INVALID_PARAMS, "EMAIL");
                return;
        }

        if (!validemail(mail))
        {
                command_fail(si, fault_badparams, "\2%s\2 is not a valid e-mail address.", mail);
                return;
        }

        /* we'll overwrite any existing metadata */
        metadata_add(mc, METADATA_CHANNEL, "email", mail);

	logcommand(si, CMDLOG_SET, "%s SET EMAIL %s", mc->name, mail);
        command_success_nodata(si, "The e-mail address for \2%s\2 has been set to \2%s\2.", parv[0], mail);
}

static void cs_cmd_set_url(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char *url = parv[1];

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to execute this command.");
		return;
	}

	/* XXX: I'd like to be able to use /CS SET #channel URL to clear but CS SET won't let me... */
	if (!url || !strcasecmp("OFF", url) || !strcasecmp("NONE", url))
	{
		/* not in a namespace to allow more natural use of SET PROPERTY.
		 * they may be able to introduce spaces, though. c'est la vie.
		 */
		if (metadata_find(mc, METADATA_CHANNEL, "url"))
		{
			metadata_delete(mc, METADATA_CHANNEL, "url");
			logcommand(si, CMDLOG_SET, "%s SET URL NONE", mc->name);
			command_success_nodata(si, "The URL for \2%s\2 has been cleared.", parv[0]);
			return;
		}

		command_fail(si, fault_nochange, "The URL for \2%s\2 was not set.", parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mc, METADATA_CHANNEL, "url", url);

	logcommand(si, CMDLOG_SET, "%s SET URL %s", mc->name, url);
	command_success_nodata(si, "The URL of \2%s\2 has been set to \2%s\2.", parv[0], url);
}

static void cs_cmd_set_entrymsg(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to execute this command.");
		return;
	}

	/* XXX: I'd like to be able to use /CS SET #channel ENTRYMSG to clear but CS SET won't let me... */
	if (!parv[1] || !strcasecmp("OFF", parv[1]) || !strcasecmp("NONE", parv[1]))
	{
		/* entrymsg is private because users won't see it if they're AKICKED,
		 * if the channel is +i, or if the channel is STAFFONLY
		 */
		if (metadata_find(mc, METADATA_CHANNEL, "private:entrymsg"))
		{
			metadata_delete(mc, METADATA_CHANNEL, "private:entrymsg");
			logcommand(si, CMDLOG_SET, "%s SET ENTRYMSG NONE", mc->name, parv[1]);
			command_success_nodata(si, "The entry message for \2%s\2 has been cleared.", parv[0]);
			return;
		}

		command_fail(si, fault_nochange, "The entry message for \2%s\2 was not set.", parv[0]);
		return;
	}

	/* we'll overwrite any existing metadata */
	metadata_add(mc, METADATA_CHANNEL, "private:entrymsg", parv[1]);

	logcommand(si, CMDLOG_SET, "%s SET ENTRYMSG %s", mc->name, parv[1]);
	command_success_nodata(si, "The entry message for \2%s\2 has been set to \2%s\2", parv[0], parv[1]);
}

/*
 * This is how CS SET FOUNDER behaves in the absence of channel passwords:
 *
 * To transfer a channel, the original founder (OF) issues the command:
 *    /CS SET #chan FOUNDER NF
 * where NF is the new founder of the channel.
 *
 * Then, to complete the transfer, the NF must issue the command:
 *    /CS SET #chan FOUNDER NF
 *
 * To cancel the transfer before it completes, the OF can issue the command:
 *    /CS SET #chan FOUNDER OF
 *
 * The purpose of the confirmation step is to prevent users from giving away
 * undesirable channels (e.g. registering #kidsex and transferring to an
 * innocent user.) Originally, we used channel passwords for this purpose.
 */
static void cs_cmd_set_founder(sourceinfo_t *si, int parc, char *parv[])
{
	char *newfounder = parv[1];
	myuser_t *tmu;
	mychan_t *mc;

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, "You are not logged in.");
		return;
	}

	if (!(tmu = myuser_find_ext(newfounder)))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", newfounder);
		return;
	}

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!is_founder(mc, si->smu))
	{
		/* User is not currently the founder.
		 * Maybe he is trying to complete a transfer?
		 */
		metadata_t *md;

		/* XXX is it portable to compare times like that? */
		if ((si->smu == tmu)
			&& (md = metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder"))
			&& !irccasecmp(md->value, si->smu->name)
			&& (md = metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp"))
			&& (atol(md->value) >= si->smu->registered))
		{
			mychan_t *tmc;
			node_t *n;
			uint32_t tcnt;
			dictionary_iteration_state_t state;

			/* make sure they're within limits (from cs_cmd_register) */
			tcnt = 0;
			DICTIONARY_FOREACH(tmc, &state, mclist)
			{
				if (is_founder(tmc, tmu))
					tcnt++;
			}

			if ((tcnt >= me.maxchans) && !has_priv_myuser(tmu, PRIV_REG_NOLIMIT))
			{
				command_fail(si, fault_toomany, "\2%s\2 has too many channels registered.", tmu->name);
				return;
			}

			if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
			{
				command_fail(si, fault_noprivs, "\2%s\2 is closed; it cannot be transferred.", mc->name);
				return;
			}

			logcommand(si, CMDLOG_SET, "%s SET FOUNDER %s (completing transfer from %s)", mc->name, tmu->name, mc->founder->name);
			verbose(mc, "Foundership transferred from \2%s\2 to \2%s\2.", mc->founder->name, tmu->name);

			/* add target as founder... */
			mc->founder = tmu;
			chanacs_change_simple(mc, tmu, NULL, CA_FOUNDER_0, 0, CA_ALL);

			/* delete transfer metadata */
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");

			/* done! */
			snoop("SET:FOUNDER: \2%s\2 -> \2%s\2", mc->name, tmu->name);
			command_success_nodata(si, "Transfer complete: "
				"\2%s\2 has been set as founder for \2%s\2.", tmu->name, mc->name);

			return;
		}

		command_fail(si, fault_noprivs, "You are not the founder of \2%s\2.", mc->name);
		return;
	}

	if (is_founder(mc, tmu))
	{
		/* User is currently the founder and
		 * trying to transfer back to himself.
		 * Maybe he is trying to cancel a transfer?
		 */

		if (metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder"))
		{
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder");
			metadata_delete(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp");

			logcommand(si, CMDLOG_SET, "%s SET FOUNDER %s (cancelling transfer)", mc->name, tmu->name);
			command_success_nodata(si, "The transfer of \2%s\2 has been cancelled.", mc->name);

			return;
		}

		command_fail(si, fault_nochange, "\2%s\2 is already the founder of \2%s\2.", tmu->name, mc->name);
		return;
	}

	/* check for lazy cancellation of outstanding requests */
	if (metadata_find(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder"))
	{
		logcommand(si, CMDLOG_SET, "%s SET FOUNDER %s (cancelling old transfer and initializing transfer)", mc->name, tmu->name);
		command_success_nodata(si, "The previous transfer request for \2%s\2 has been cancelled.", mc->name);
	}
	else
		logcommand(si, CMDLOG_SET, "%s SET FOUNDER %s (initializing transfer)", mc->name, tmu->name);

	metadata_add(mc, METADATA_CHANNEL, "private:verify:founderchg:newfounder", tmu->name);
	metadata_add(mc, METADATA_CHANNEL, "private:verify:founderchg:timestamp", itoa(time(NULL)));

	command_success_nodata(si, "\2%s\2 can now take ownership of \2%s\2.", tmu->name, mc->name);
	command_success_nodata(si, "In order to complete the transfer, \2%s\2 must perform the following command:", tmu->name);
	command_success_nodata(si, "   \2/msg %s SET %s FOUNDER %s\2", chansvs.nick, mc->name, tmu->name);
	command_success_nodata(si, "After that command is issued, the channel will be transferred.", mc->name);
	command_success_nodata(si, "To cancel the transfer, use \2/msg %s SET %s FOUNDER %s\2", chansvs.nick, mc->name, mc->founder->name);
}

static void cs_cmd_set_mlock(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
	char modebuf[32], *end, c;
	int add = -1;
	int32_t newlock_on = 0, newlock_off = 0, newlock_limit = 0, flag = 0;
	int32_t mask;
	char newlock_key[KEYLEN];
	char newlock_ext[MAXEXTMODES][512];
	boolean_t newlock_ext_off[MAXEXTMODES];
	char newext[512];
	char ext_plus[MAXEXTMODES + 1], ext_minus[MAXEXTMODES + 1];
	int i;
	char *letters = strtok(parv[1], " ");
	char *arg;

	if (!letters)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "MLOCK");
		return;
	}

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this command.");
		return;
	}

	for (i = 0; i < MAXEXTMODES; i++)
	{
		newlock_ext[i][0] = '\0';
		newlock_ext_off[i] = FALSE;
	}
	ext_plus[0] = '\0';
	ext_minus[0] = '\0';
	newlock_key[0] = '\0';

	mask = has_priv(si, PRIV_CHAN_CMODES) ? 0 : ircd->oper_only_modes;

	while (*letters)
	{
		if (*letters != '+' && *letters != '-' && add < 0)
		{
			letters++;
			continue;
		}

		switch ((c = *letters++))
		{
		  case '+':
			  add = 1;
			  break;

		  case '-':
			  add = 0;
			  break;

		  case 'k':
			  if (add)
			  {
				  arg = strtok(NULL, " ");
				  if (!arg)
				  {
					  command_fail(si, fault_badparams, "You need to specify which key to MLOCK.");
					  return;
				  }
				  else if (strlen(arg) >= KEYLEN)
				  {
					  command_fail(si, fault_badparams, "MLOCK key is too long (%d > %d).", strlen(arg), KEYLEN - 1);
					  return;
				  }
				  else if (strchr(arg, ',') || arg[0] == ':')
				  {
					  command_fail(si, fault_badparams, "MLOCK key contains invalid characters.");
					  return;
				  }

				  strlcpy(newlock_key, arg, sizeof newlock_key);
				  newlock_off &= ~CMODE_KEY;
			  }
			  else
			  {
				  newlock_key[0] = '\0';
				  newlock_off |= CMODE_KEY;
			  }

			  break;

		  case 'l':
			  if (add)
			  {
				  arg = strtok(NULL, " ");
				  if(!arg)
				  {
					  command_fail(si, fault_badparams, "You need to specify what limit to MLOCK.");
					  return;
				  }

				  if (atol(arg) <= 0)
				  {
					  command_fail(si, fault_badparams, "You must specify a positive integer for limit.");
					  return;
				  }

				  newlock_limit = atol(arg);
				  newlock_off &= ~CMODE_LIMIT;
			  }
			  else
			  {
				  newlock_limit = 0;
				  newlock_off |= CMODE_LIMIT;
			  }

			  break;

		  default:
			  flag = mode_to_flag(c);

			  if (flag)
			  {
				  if (add)
					  newlock_on |= flag, newlock_off &= ~flag;
				  else
					  newlock_off |= flag, newlock_on &= ~flag;
				  break;
			  }

			  for (i = 0; i < MAXEXTMODES; i++)
			  {
				  if (c == ignore_mode_list[i].mode)
				  {
					  if (add)
					  {
						  arg = strtok(NULL, " ");
						  if(!arg)
						  {
							  command_fail(si, fault_badparams, "You need to specify a value for mode +%c.", c);
							  return;
						  }
						  if (strlen(arg) > 350)
						  {
							  command_fail(si, fault_badparams, "Invalid value \2%s\2 for mode +%c.", arg, c);
							  return;
						  }
						  if ((mc->chan == NULL || mc->chan->extmodes[i] == NULL || strcmp(mc->chan->extmodes[i], arg)) && !ignore_mode_list[i].check(arg, mc->chan, mc, si->su, si->smu))
						  {
							  command_fail(si, fault_badparams, "Invalid value \2%s\2 for mode +%c.", arg, c);
							  return;
						  }
						  strlcpy(newlock_ext[i], arg, sizeof newlock_ext[i]);
						  newlock_ext_off[i] = FALSE;
					  }
					  else
					  {
						  newlock_ext[i][0] = '\0';
						  newlock_ext_off[i] = TRUE;
					  }
				  }
			  }
		}
	}

	if (strlen(newext) > 450)
	{
		command_fail(si, fault_badparams, "Mode lock is too long.");
		return;
	}

	/* save it to mychan */
	/* leave the modes in mask unchanged -- jilles */
	mc->mlock_on = (newlock_on & ~mask) | (mc->mlock_on & mask);
	mc->mlock_off = (newlock_off & ~mask) | (mc->mlock_off & mask);
	mc->mlock_limit = newlock_limit;

	if (mc->mlock_key)
		free(mc->mlock_key);

	mc->mlock_key = *newlock_key != '\0' ? sstrdup(newlock_key) : NULL;

	newext[0] = '\0';
	for (i = 0; i < MAXEXTMODES; i++)
	{
		if (newlock_ext[i][0] != '\0' || newlock_ext_off[i])
		{
			if (*newext != '\0')
			{
				modebuf[0] = ' ';
				modebuf[1] = '\0';
				strlcat(newext, modebuf, sizeof newext);
			}
			modebuf[0] = ignore_mode_list[i].mode;
			modebuf[1] = '\0';
			strlcat(newext, modebuf, sizeof newext);
			strlcat(newlock_ext_off[i] ? ext_minus : ext_plus,
					modebuf, MAXEXTMODES + 1);
			if (!newlock_ext_off[i])
				strlcat(newext, newlock_ext[i], sizeof newext);
		}
	}
	if (newext[0] != '\0')
		metadata_add(mc, METADATA_CHANNEL, "private:mlockext", newext);
	else
		metadata_delete(mc, METADATA_CHANNEL, "private:mlockext");

	end = modebuf;
	*end = 0;

	if (mc->mlock_on || mc->mlock_key || mc->mlock_limit || *ext_plus)
		end += snprintf(end, sizeof(modebuf) - (end - modebuf), "+%s%s%s%s", flags_to_string(mc->mlock_on), mc->mlock_key ? "k" : "", mc->mlock_limit ? "l" : "", ext_plus);

	if (mc->mlock_off || *ext_minus)
		end += snprintf(end, sizeof(modebuf) - (end - modebuf), "-%s%s%s%s", flags_to_string(mc->mlock_off), mc->mlock_off & CMODE_KEY ? "k" : "", mc->mlock_off & CMODE_LIMIT ? "l" : "", ext_minus);

	if (*modebuf)
	{
		command_success_nodata(si, "The MLOCK for \2%s\2 has been set to \2%s\2.", mc->name, modebuf);
		logcommand(si, CMDLOG_SET, "%s SET MLOCK %s", mc->name, modebuf);
	}
	else
	{
		command_success_nodata(si, "The MLOCK for \2%s\2 has been removed.", mc->name);
		logcommand(si, CMDLOG_SET, "%s SET MLOCK NONE", mc->name);
	}

	check_modes(mc, TRUE);

	return;
}

static void cs_cmd_set_keeptopic(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_KEEPTOPIC & mc->flags)
                {
                        command_fail(si, fault_nochange, "The \2KEEPTOPIC\2 flag is already set for \2%s\2.", mc->name);
                        return;
                }

		logcommand(si, CMDLOG_SET, "%s SET KEEPTOPIC ON", mc->name);

                mc->flags |= MC_KEEPTOPIC;

                command_success_nodata(si, "The \2KEEPTOPIC\2 flag has been set for \2%s\2.", mc->name);

                return;
        }

        else if (!strcasecmp("OFF", parv[1]))
        {
                if (!(MC_KEEPTOPIC & mc->flags))
                {
                        command_fail(si, fault_nochange, "The \2KEEPTOPIC\2 flag is not set for \2%s\2.", mc->name);
                        return;
                }

		logcommand(si, CMDLOG_SET, "%s SET KEEPTOPIC OFF", mc->name);

                mc->flags &= ~(MC_KEEPTOPIC | MC_TOPICLOCK);

                command_success_nodata(si, "The \2KEEPTOPIC\2 flag has been removed for \2%s\2.", mc->name);

                return;
        }

        else
        {
                command_fail(si, fault_badparams, STR_INVALID_PARAMS, "KEEPTOPIC");
                return;
        }
}

static void cs_cmd_set_topiclock(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_TOPICLOCK & mc->flags)
                {
                        command_fail(si, fault_nochange, "The \2TOPICLOCK\2 flag is already set for \2%s\2.", mc->name);
                        return;
                }

		logcommand(si, CMDLOG_SET, "%s SET TOPICLOCK ON", mc->name);

                mc->flags |= MC_KEEPTOPIC | MC_TOPICLOCK;

                command_success_nodata(si, "The \2TOPICLOCK\2 flag has been set for \2%s\2.", mc->name);

                return;
        }

        else if (!strcasecmp("OFF", parv[1]))
        {
                if (!(MC_TOPICLOCK & mc->flags))
                {
                        command_fail(si, fault_nochange, "The \2TOPICLOCK\2 flag is not set for \2%s\2.", mc->name);
                        return;
                }

		logcommand(si, CMDLOG_SET, "%s SET TOPICLOCK OFF", mc->name);

                mc->flags &= ~MC_TOPICLOCK;

                command_success_nodata(si, "The \2TOPICLOCK\2 flag has been removed for \2%s\2.", mc->name);

                return;
        }

        else
        {
                command_fail(si, fault_badparams, STR_INVALID_PARAMS, "TOPICLOCK");
                return;
        }
}

static void cs_cmd_set_secure(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_SECURE & mc->flags)
		{
			command_fail(si, fault_nochange, "The \2SECURE\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "%s SET SECURE ON", mc->name);

		mc->flags |= MC_SECURE;

		command_success_nodata(si, "The \2SECURE\2 flag has been set for \2%s\2.", mc->name);

		return;
	}

	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_SECURE & mc->flags))
		{
			command_fail(si, fault_nochange, "The \2SECURE\2 flag is not set for \2%s\2.", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "%s SET SECURE OFF", mc->name);

		mc->flags &= ~MC_SECURE;

		command_success_nodata(si, "The \2SECURE\2 flag has been removed for \2%s\2.", mc->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SECURE");
		return;
	}
}

static void cs_cmd_set_verbose(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", parv[1]) || !strcasecmp("ALL", parv[1]))
	{
		if (MC_VERBOSE & mc->flags)
		{
			command_fail(si, fault_nochange, "The \2VERBOSE\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "%s SET VERBOSE ON", mc->name);

 		mc->flags &= ~MC_VERBOSE_OPS;
 		mc->flags |= MC_VERBOSE;

		verbose(mc, "\2%s\2 enabled the VERBOSE flag", get_source_name(si));
		command_success_nodata(si, "The \2VERBOSE\2 flag has been set for \2%s\2.", mc->name);

		return;
	}

	else if (!strcasecmp("OPS", parv[1]))
	{
		if (MC_VERBOSE_OPS & mc->flags)
		{
			command_fail(si, fault_nochange, "The \2VERBOSE_OPS\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "%s SET VERBOSE OPS", mc->name);

		if (mc->flags & MC_VERBOSE)
		{
			verbose(mc, "\2%s\2 restricted VERBOSE to chanops", get_source_name(si));
 			mc->flags &= ~MC_VERBOSE;
 			mc->flags |= MC_VERBOSE_OPS;
		}
		else
		{
 			mc->flags |= MC_VERBOSE_OPS;
			verbose(mc, "\2%s\2 enabled the VERBOSE_OPS flag", get_source_name(si));
		}

		command_success_nodata(si, "The \2VERBOSE_OPS\2 flag has been set for \2%s\2.", mc->name);

		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!((MC_VERBOSE | MC_VERBOSE_OPS) & mc->flags))
		{
			command_fail(si, fault_nochange, "The \2VERBOSE\2 flag is not set for \2%s\2.", mc->name);
			return;
		}

		logcommand(si, CMDLOG_SET, "%s SET VERBOSE OFF", mc->name);

		if (mc->flags & MC_VERBOSE)
			verbose(mc, "\2%s\2 disabled the VERBOSE flag", get_source_name(si));
		else
			verbose(mc, "\2%s\2 disabled the VERBOSE_OPS flag", get_source_name(si));
		mc->flags &= ~(MC_VERBOSE | MC_VERBOSE_OPS);

		command_success_nodata(si, "The \2VERBOSE\2 flag has been removed for \2%s\2.", mc->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "VERBOSE");
		return;
	}
}

static void cs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		metadata_t *md = metadata_find(mc, METADATA_CHANNEL, "disable_fantasy");

		if (!md)
		{
			command_fail(si, fault_nochange, "Fantasy is already enabled on \2%s\2.", mc->name);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, "disable_fantasy");

		logcommand(si, CMDLOG_SET, "%s SET FANTASY ON", mc->name);
		command_success_nodata(si, "The \2FANTASY\2 flag has been set for \2%s\2.", mc->name);
		return;
	}
	else if (!strcasecmp("OFF", parv[1]))
	{
		metadata_t *md = metadata_find(mc, METADATA_CHANNEL, "disable_fantasy");

		if (md)
		{
			command_fail(si, fault_nochange, "Fantasy is already disabled on \2%s\2.", mc->name);
			return;
		}

		metadata_add(mc, METADATA_CHANNEL, "disable_fantasy", "on");

		logcommand(si, CMDLOG_SET, "%s SET FANTASY OFF", mc->name);
		command_success_nodata(si, "The \2FANTASY\2 flag has been removed for \2%s\2.", mc->name);
		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FANTASY");
		return;
	}
}

static void cs_cmd_set_staffonly(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!strcasecmp("ON", parv[1]))
	{
		if (MC_STAFFONLY & mc->flags)
		{
			command_fail(si, fault_nochange, "The \2STAFFONLY\2 flag is already set for \2%s\2.", mc->name);
			return;
		}

		snoop("SET:STAFFONLY:ON: for \2%s\2 by \2%s\2", mc->name, get_oper_name(si));
		logcommand(si, CMDLOG_SET, "%s SET STAFFONLY ON", mc->name);

		mc->flags |= MC_STAFFONLY;

		command_success_nodata(si, "The \2STAFFONLY\2 flag has been set for \2%s\2.", mc->name);

		return;
	}

	else if (!strcasecmp("OFF", parv[1]))
	{
		if (!(MC_STAFFONLY & mc->flags))
		{
			command_fail(si, fault_nochange, "The \2STAFFONLY\2 flag is not set for \2%s\2.", mc->name);
			return;
		}

		snoop("SET:STAFFONLY:OFF: for \2%s\2 by \2%s\2", mc->name, get_oper_name(si));
		logcommand(si, CMDLOG_SET, "%s SET STAFFONLY OFF", mc->name);

		mc->flags &= ~MC_STAFFONLY;

		command_success_nodata(si, "The \2STAFFONLY\2 flag has been removed for \2%s\2.", mc->name);

		return;
	}

	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "STAFFONLY");
		return;
	}
}

static void cs_cmd_set_property(sourceinfo_t *si, int parc, char *parv[])
{
	mychan_t *mc;
        char *property = strtok(parv[1], " ");
        char *value = strtok(NULL, "");

        if (!property)
        {
                command_fail(si, fault_needmoreparams, "Syntax: SET <#channel> PROPERTY <property> [value]");
                return;
        }

	/* do we really need to allow this? -- jilles */
        if (strchr(property, ':') && !has_priv(si, PRIV_METADATA))
        {
                command_fail(si, fault_badparams, "Invalid property name.");
                return;
        }

	if (!(mc = mychan_find(parv[0])))
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", parv[0]);
		return;
	}

	if (!is_founder(mc, si->smu))
	{
		command_fail(si, fault_noprivs, "You are not authorized to perform this command.");
		return;
	}

	if (mc->metadata.count >= me.mdlimit)
	{
		command_fail(si, fault_toomany, "Cannot add \2%s\2 to \2%s\2 metadata table, it is full.",
						property, parv[0]);
		return;
	}

        if (strchr(property, ':'))
        	snoop("SET:PROPERTY: \2%s\2: \2%s\2/\2%s\2", mc->name, property, value);

	if (!value)
	{
		metadata_t *md = metadata_find(mc, METADATA_CHANNEL, property);

		if (!md)
		{
			command_fail(si, fault_nochange, "Metadata entry \2%s\2 was not set.", property);
			return;
		}

		metadata_delete(mc, METADATA_CHANNEL, property);
		logcommand(si, CMDLOG_SET, "%s SET PROPERTY %s (deleted)", mc->name, property);
		command_success_nodata(si, "Metadata entry \2%s\2 has been deleted.", property);
		return;
	}

	if (strlen(property) > 32 || strlen(value) > 300)
	{
		command_fail(si, fault_badparams, "Parameters are too long. Aborting.");
		return;
	}

	metadata_add(mc, METADATA_CHANNEL, property, value);
	logcommand(si, CMDLOG_SET, "%s SET PROPERTY %s to %s", mc->name, property, value);
	command_success_nodata(si, "Metadata entry \2%s\2 added.", property);
}
