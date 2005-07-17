/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService INFO functions.
 *
 * $Id: cs_info.c 948 2005-07-17 20:39:21Z nenolod $
 */

#include "atheme.h"

static void cs_cmd_info(char *origin);

command_t cs_info = { "INFO", "Displays information on registrations.",
                        AC_NONE, cs_cmd_info };

extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_info, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_info, &cs_cmdtree);
}

static void cs_cmd_info(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc;
	char *name = strtok(NULL, " ");
	char buf[BUFSIZE], strfbuf[32];
	struct tm tm;
	metadata_t *md;

	if (!name)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2INFO\2.");
		notice(chansvs.nick, origin, "Syntax: INFO <username|#channel>");
		return;
	}

	if (*name == '#')
	{
		if (!(mc = mychan_find(name)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
			return;
		}

		snoop("INFO: \2%s\2 by \2%s\2", name, origin);

		tm = *localtime(&mc->registered);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(chansvs.nick, origin, "Information on \2%s\2", mc->name);

		if (mc->founder->user)
			notice(chansvs.nick, origin, "Founder    : %s (logged in from \2%s\2)", mc->founder->name, mc->founder->user->nick);
		else
			notice(chansvs.nick, origin, "Founder    : %s (not logged in)", mc->founder->name);

		if (mc->successor)
		{
			if (mc->successor->user)
				notice(chansvs.nick, origin, "Successor  : %s (logged in from \2%s\2)", mc->successor->name, mc->successor->user->nick);
			else
				notice(chansvs.nick, origin, "Successor  : %s (not logged in)", mc->successor->name);
		}

		notice(chansvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mc->registered));

		if (mc->mlock_on || mc->mlock_off)
		{
			char params[BUFSIZE];

			*buf = 0;
			*params = 0;

			if (mc->mlock_on)
			{
				strcat(buf, "+");
				strcat(buf, flags_to_string(mc->mlock_on));

				/* add these in manually */
				if (mc->mlock_limit)
				{
					strcat(buf, "l");
					strcat(params, " ");
					strcat(params, itoa(mc->mlock_limit));
				}

				if (mc->mlock_key)
					strcat(buf, "k");
			}

			if (mc->mlock_off)
			{
				strcat(buf, "-");
				strcat(buf, flags_to_string(mc->mlock_off));
			}

			if (*buf)
				notice(chansvs.nick, origin, "Mode lock  : %s %s", buf, (params) ? params : "");
		}

		if (mc->url)
			notice(chansvs.nick, origin, "URL        : %s", mc->url);

		*buf = '\0';

		if (MC_HOLD & mc->flags)
			strcat(buf, "HOLD");

		if (MC_SECURE & mc->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "SECURE");
		}
		if (MC_VERBOSE & mc->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "VERBOSE");
		}
		if (MC_STAFFONLY & mc->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "STAFFONLY");
		}

                if ((is_ircop(u) || is_sra(u->myuser)) && (md = metadata_find(mc, METADATA_CHANNEL, "private:mark:setter")))
                {
                        char *setter = md->value;
                        char *reason;
                        time_t ts;

                        md = metadata_find(mc, METADATA_CHANNEL, "private:mark:reason");
                        reason = md->value;

                        md = metadata_find(mc, METADATA_CHANNEL, "private:mark:timestamp");
                        ts = atoi(md->value);

                        tm = *localtime(&ts);
                        strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

                        notice(chansvs.nick, origin, "%s was MARKED by %s on %s (%s)", mc->name, setter, strfbuf, reason);
                }

		if ((is_ircop(u) || is_sra(u->myuser)) && (md = metadata_find(mc, METADATA_CHANNEL, "private:close:closer")))
		{
			char *setter = md->value;
			char *reason;
			time_t ts;

			md = metadata_find(mc, METADATA_CHANNEL, "private:close:reason");
			reason = md->value;

			md = metadata_find(mc, METADATA_CHANNEL, "private:close:timestamp");
			ts = atoi(md->value);

			tm = *localtime(&ts);
			strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

			notice(chansvs.nick, origin, "%s was CLOSED by %s on %s (%s)", mc->name, setter, strfbuf, reason);
		}

		if (*buf)
			notice(chansvs.nick, origin, "Flags      : %s", buf);
	}
	else
	{
		if (!(mu = myuser_find(name)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
			return;
		}

		snoop("INFO: \2%s\2 by \2%s\2", name, origin);

		tm = *localtime(&mu->registered);
		strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

		notice(chansvs.nick, origin, "Information on \2%s\2", mu->name);

		notice(chansvs.nick, origin, "Registered : %s (%s ago)", strfbuf, time_ago(mu->registered));

		if ((!(mu->flags & MU_HIDEMAIL)) || (is_sra(u->myuser) || is_ircop(u) || u->myuser == mu))
			notice(chansvs.nick, origin, "Email      : %s%s", mu->email,
								(mu->flags & MU_HIDEMAIL) ? " (hidden)" : "");

		*buf = '\0';

		if (MU_HIDEMAIL & mu->flags)
			strcat(buf, "HIDEMAIL");

		if (MU_HOLD & mu->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "HOLD");
		}
		if (MU_NEVEROP & mu->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "NEVEROP");
		}
		if (MU_NOOP & mu->flags)
		{
			if (*buf)
				strcat(buf, " ");

			strcat(buf, "NOOP");
		}

	        if (is_ircop(u) && (md = metadata_find(mu, METADATA_USER, "private:mark:setter")))
	        {
	                char *setter = md->value;
	                char *reason;
	                time_t ts;

	                md = metadata_find(mu, METADATA_USER, "private:mark:reason");
	                reason = md->value;
                
	                md = metadata_find(mu, METADATA_USER, "private:mark:timestamp");
	                ts = atoi(md->value);

	                tm = *localtime(&ts);
	                strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);
         
	                notice(chansvs.nick, origin, "%s was MARKED by %s on %s (%s)", mu->name, setter, strfbuf, reason);
	        }

		if (*buf)
			notice(chansvs.nick, origin, "Flags      : %s", buf);
	}
}
