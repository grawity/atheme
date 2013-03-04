/*
 * Copyright (c) 2005-2006 Jilles Tjoelker, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService TEMPLATE functions.
 *
 * $Id: template.c 7109 2006-11-08 15:18:19Z jilles $
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/template", FALSE, _modinit, _moddeinit,
	"$Id: template.c 7109 2006-11-08 15:18:19Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void list_generic_flags(sourceinfo_t *si);

static void cs_cmd_template(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_flags = { "TEMPLATE", "Manipulates predefined sets of flags.",
                        AC_NONE, 3, cs_cmd_template };

list_t *cs_cmdtree;
list_t *cs_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");
	MODULE_USE_SYMBOL(cs_helptree, "chanserv/main", "cs_helptree");

        command_add(&cs_flags, cs_cmdtree);
	help_addentry(cs_helptree, "TEMPLATE", "help/cservice/template", NULL);
}

void _moddeinit()
{
	command_delete(&cs_flags, cs_cmdtree);
	help_delentry(cs_helptree, "TEMPLATE");
}

static void list_generic_flags(sourceinfo_t *si)
{
	command_success_nodata(si, "%-20s %s", "Name", "Flags");
	command_success_nodata(si, "%-20s %s", "--------------------", "-----");
	command_success_nodata(si, "%-20s %s", "SOP", bitmask_to_flags(chansvs.ca_sop, chanacs_flags));
	command_success_nodata(si, "%-20s %s", "AOP", bitmask_to_flags(chansvs.ca_aop, chanacs_flags));
	command_success_nodata(si, "%-20s %s", "HOP", bitmask_to_flags(chansvs.ca_hop, chanacs_flags));
	command_success_nodata(si, "%-20s %s", "VOP", bitmask_to_flags(chansvs.ca_vop, chanacs_flags));
	command_success_nodata(si, "%-20s %s", "--------------------", "-----");
	command_success_nodata(si, "End of network wide template list.");
}

/* TEMPLATE [channel] [template] [flags] */
static void cs_cmd_template(sourceinfo_t *si, int parc, char *parv[])
{
	metadata_t *md;
	int operoverride = 0, changechanacs = 0, l;
	char *channel = parv[0];
	char *target = parv[1];
	mychan_t *mc = mychan_find(channel);
	uint32_t oldflags, newflags = 0, addflags, removeflags, restrictflags;
	char *p, *q, *r;
	char ss[40], newstr[400];
	boolean_t found, denied;

	if (!channel)
	{
		list_generic_flags(si);
		logcommand(si, CMDLOG_GET, "TEMPLATE");
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, "\2%s\2 is not registered.", channel);
		return;
	}

	if (!target)
	{
		uint8_t i = 1;

		if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
		{
			if (has_priv(si, PRIV_CHAN_AUSPEX))
				operoverride = 1;
			else
			{
				command_fail(si, fault_noprivs, "You are not authorized to perform this operation.");
				return;
			}
		}
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer") && !has_priv(si, PRIV_CHAN_AUSPEX))
		{
			command_fail(si, fault_noprivs, "\2%s\2 is closed.", channel);
			return;
		}

		md = metadata_find(mc, METADATA_CHANNEL, "private:templates");

		if (md != NULL)
		{
			command_success_nodata(si, "%-20s %s", "Name", "Flags");
			command_success_nodata(si, "%-20s %s", "--------------------", "-----");

			p = md->value;
			while (p != NULL)
			{
				while (*p == ' ')
					p++;
				q = strchr(p, '=');
				if (q == NULL)
					break;
				r = strchr(q, ' ');
				command_success_nodata(si, "%-20.*s %.*s", (q - p), p, r != NULL ? (r - q - 1) : strlen(q + 1), q + 1);
				i++;
				p = r;
			}

			command_success_nodata(si, "%-20s %s", "--------------------", "-----");
			command_success_nodata(si, "End of \2%s\2 TEMPLATE listing.", mc->name);
		}
		else
			command_success_nodata(si, "No templates set on channel \2%s\2.", mc->name);
		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "%s TEMPLATE (oper override)", mc->name);
		else
			logcommand(si, CMDLOG_GET, "%s TEMPLATE", mc->name);
	}
	else
	{
		char *flagstr = parv[2];

		if (!si->smu)
		{
			command_fail(si, fault_noprivs, "You are not logged in.");
			return;
		}

		/* probably no need to special-case founder here -- jilles */
#if 0
		if (is_founder(mc, si->smu))
			restrictflags = CA_ALL;
		else
#endif
		{
			restrictflags = chanacs_source_flags(mc, si);
			if (!(restrictflags & CA_FLAGS))
			{
				command_fail(si, fault_noprivs, "You are not authorized to execute this command.");
				return;
			}
			restrictflags = allow_flags(restrictflags);
		}
		
		if (metadata_find(mc, METADATA_CHANNEL, "private:close:closer"))
		{
			command_fail(si, fault_noprivs, "\2%s\2 is closed.", channel);
			return;
		}

		if (!target || !flagstr)
		{
			command_fail(si, fault_needmoreparams, "Usage: TEMPLATE %s [target flags]", channel);
			return;
		}

		if (*target == '+' || *target == '-' || *target == '=')
		{
			command_fail(si, fault_badparams, "Invalid template name \2%s\2.", target);
			return;
		}
		l = strlen(target);
		/* don't allow redefining xop
		 * redefinition of future per-network other templates
		 * could be ok though */
		if (l == 3 && (!strcasecmp(target, "SOP") ||
					!strcasecmp(target, "AOP") ||
					!strcasecmp(target, "HOP") ||
					!strcasecmp(target, "VOP")))
		{
			command_fail(si, fault_noprivs, "Cannot redefine built-in template \2%s\2.", target);
			return;
		}

		if (*flagstr == '!' && (flagstr[1] == '+' || flagstr[1] == '-' || flagstr[1] == '='))
		{
			changechanacs = 1;
			flagstr++;
		}

		if (*flagstr == '+' || *flagstr == '-' || *flagstr == '=')
		{
			flags_make_bitmasks(flagstr, chanacs_flags, &addflags, &removeflags);
			if (addflags == 0 && removeflags == 0)
			{
				command_fail(si, fault_badparams, "No valid flags given, use /%s%s HELP FLAGS for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp);
				return;
			}
		}
		else
		{
			/* allow copying templates as well */
			addflags = get_template_flags(mc, flagstr);
			if (addflags == 0)
			{
				command_fail(si, fault_nosuch_key, "Invalid template name given, use /%s%s TEMPLATE %s for a list", ircd->uses_rcommand ? "" : "msg ", chansvs.disp, mc->name);
				return;
			}
			removeflags = CA_ALL & ~addflags;
		}

		found = denied = FALSE;
		oldflags = 0;

		md = metadata_find(mc, METADATA_CHANNEL, "private:templates");
		if (md != NULL)
		{
			p = md->value;
			strlcpy(newstr, p, sizeof newstr);
			while (p != NULL)
			{
				while (*p == ' ')
					p++;
				q = strchr(p, '=');
				if (q == NULL)
					break;
				r = strchr(q, ' ');
				strlcpy(ss, q, sizeof ss);
				if (r != NULL && r - q < (sizeof ss - 1))
				{
					ss[r - q] = '\0';
				}
				if (q - p == l && !strncasecmp(target, p, q - p))
				{
					found = TRUE;
					oldflags = flags_to_bitmask(ss, chanacs_flags, 0);
					addflags &= ~oldflags;
					removeflags &= oldflags & ~addflags;
					/* no change? */
					if ((addflags | removeflags) == 0)
						break;
					/* attempting to add bad flag? */
					/* attempting to remove bad flag? */
					/* attempting to manipulate something with more privs? */
					if (~restrictflags & addflags ||
							~restrictflags & removeflags ||
							~restrictflags & oldflags)
					{
						denied = TRUE;
						break;
					}
					newflags = (oldflags | addflags) & ~removeflags;
					if (newflags == 0)
					{
						if (p == md->value)
							/* removing first entry,
							 * zap the space after it */
							strlcpy(newstr, r != NULL ? r + 1 : "", sizeof newstr);
						else
						{
							/* otherwise, zap the space before it */
							p--;
							strlcpy(newstr + (p - md->value), r != NULL ? r : "", sizeof newstr - (p - md->value));
						}
					}
					else
						snprintf(newstr + (p - md->value), sizeof newstr - (p - md->value), "%s=%s%s", target, bitmask_to_flags(newflags, chanacs_flags), r != NULL ? r : "");
					break;
				}
				p = r;
			}
		}
		if (!found)
		{
			removeflags = 0;
			newflags = addflags;
			if ((addflags | removeflags) == 0)
				;
			else if (~restrictflags & addflags)
				denied = TRUE;
			else if (md != NULL)
				snprintf(newstr + strlen(newstr), sizeof newstr - strlen(newstr), " %s=%s", target, bitmask_to_flags(addflags, chanacs_flags));
			else
				snprintf(newstr, sizeof newstr, "%s=%s", target, bitmask_to_flags(newflags, chanacs_flags));
		}
		if ((addflags | removeflags) == 0)
		{
			if (oldflags != 0)
				command_fail(si, fault_nochange, "Template \2%s\2 on \2%s\2 unchanged.", target, channel);
			else
				command_fail(si, fault_nosuch_key, "No such template \2%s\2 on \2%s\2.", target, channel);
			return;
		}
		if (denied)
		{
			command_fail(si, fault_noprivs, "You are not allowed to set \2%s\2 on template \2%s\2 in \2%s\2.", bitmask_to_flags2(addflags, removeflags, chanacs_flags), target, mc->name);
			return;
		}
		if (strlen(newstr) >= 300)
		{
			command_fail(si, fault_toomany, "Sorry, too many templates on \2%s\2.", channel);
			return;
		}
		if (newstr[0] == '\0')
			metadata_delete(mc, METADATA_CHANNEL, "private:templates");
		else
			metadata_add(mc, METADATA_CHANNEL, "private:templates", newstr);
		if (oldflags == 0)
			command_success_nodata(si, "Added template \2%s\2 with flags \2%s\2 in \2%s\2.", target, bitmask_to_flags(newflags, chanacs_flags), channel);
		else if (newflags == 0)
			command_success_nodata(si, "Removed template \2%s\2 from \2%s\2.", target, channel);
		else
			command_success_nodata(si, "Changed template \2%s\2 to \2%s\2 in \2%s\2.", target, bitmask_to_flags(newflags, chanacs_flags), channel);

		flagstr = bitmask_to_flags2(addflags, removeflags, chanacs_flags);
		if (changechanacs)
		{
			node_t *n, *tn;
			chanacs_t *ca;
			int changes = 0, founderskipped = 0;
			char flagstr2[128];

			LIST_FOREACH_SAFE(n, tn, mc->chanacs.head)
			{
				ca = n->data;
				if (ca->level != oldflags)
					continue;
				if (ca->myuser != NULL && is_founder(mc, ca->myuser) && !(newflags & CA_FLAGS))
				{
					founderskipped = 1;
					continue;
				}
				changes++;
				if (newflags == 0)
					if (ca->myuser != NULL)
						chanacs_delete(mc, ca->myuser, ca->level);
					else
						chanacs_delete_host(mc, ca->host, ca->level);
				else
					ca->level = newflags;
			}
			logcommand(si, CMDLOG_SET, "%s TEMPLATE %s !%s (%d changes)", mc->name, target, flagstr, changes);
			strlcpy(flagstr2, flagstr, sizeof flagstr2);
			if (changes > 0)
				verbose(mc, "\2%s\2 set \2%s\2 on %d access entries with flags \2%s\2.", get_source_name(si), flagstr2, changes, bitmask_to_flags(oldflags, chanacs_flags));
			command_success_nodata(si, "%d access entries updated accordingly.", changes);
			if (founderskipped)
				command_success_nodata(si, "The access entry for %s was not updated because they are channel founder.", mc->founder->name);
		}
		else
			logcommand(si, CMDLOG_SET, "%s TEMPLATE %s %s", mc->name, target, flagstr);
		/*verbose(mc, "Flags \2%s\2 were set on template \2%s\2 in \2%s\2.", flagstr, target, channel);*/
	}
}
