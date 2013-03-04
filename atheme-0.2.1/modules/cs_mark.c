/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for nicknames.
 *
 * $Id: cs_mark.c 882 2005-07-16 17:51:47Z alambert $
 */

#include "atheme.h"

static void cs_cmd_mark(char *origin);

command_t cs_mark = { "MARK", "Adds a note to a user.",
			AC_IRCOP, cs_cmd_mark };

extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
	command_add(&cs_mark, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_mark, &cs_cmdtree);
}

static void cs_cmd_mark(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *info = strtok(NULL, "");
	myuser_t *mu;
	mychan_t *mc;

	if (!target || !action)
	{
		notice(chansvs.nick, origin, "Insufficient parameters for \2MARK\2.");
		notice(chansvs.nick, origin, "Usage: MARK <target> <ON|OFF> [note]");
		return;
	}

	if (target[0] == '#')
	{
		if (!(mc = mychan_find(target)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", target);
			return;
		}

		if (!strcasecmp(action, "ON"))
		{
			if (!info)
			{
				notice(chansvs.nick, origin, "Insufficient parameters for \2MARK\2.");
				notice(chansvs.nick, origin, "Usage: MARK <target> ON <note>");
				return;
			}


			if (metadata_find(mc, METADATA_CHANNEL, "private:mark:setter"))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already marked.", target);
				return;
			}

			metadata_add(mc, METADATA_CHANNEL, "private:mark:setter", origin);
			metadata_add(mc, METADATA_CHANNEL, "private:mark:reason", info);
			metadata_add(mc, METADATA_CHANNEL, "private:mark:timestamp", itoa(CURRTIME));

			wallops("%s marked the channel \2%s\2.", origin, target);
			notice(chansvs.nick, origin, "\2%s\2 is now marked.", target);
		}
		else if (!strcasecmp(action, "OFF"))
		{
			if (!metadata_find(mc, METADATA_CHANNEL, "private:mark:setter"))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not marked.", target);
				return;
			}

			metadata_delete(mc, METADATA_CHANNEL, "private:mark:setter");
			metadata_delete(mc, METADATA_CHANNEL, "private:mark:reason");
			metadata_delete(mc, METADATA_CHANNEL, "private:mark:timestamp");

			wallops("%s unmarked the channel \2%s\2.", origin, target);
			notice(chansvs.nick, origin, "\2%s\2 is now unmarked.", target);
		}
		else
		{
			notice(chansvs.nick, origin, "Insufficient parameters for \2MARK\2.");
			notice(chansvs.nick, origin, "Usage: MARK <target> <ON|OFF> [note]");
		}
	}
	else
	{
		if (!(mu = myuser_find(target)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not registered.", target);
			return;
		}

		if (!strcasecmp(action, "ON"))
		{
			if (metadata_find(mu, METADATA_USER, "private:mark:setter"))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already marked.", target);
				return;
			}

			if (!info)
			{
				notice(chansvs.nick, origin, "Insufficient parameters for \2MARK\2.");
				notice(chansvs.nick, origin, "Usage: MARK <target> ON <note>");
				return;
			}

			metadata_add(mu, METADATA_USER, "private:mark:setter", origin);
			metadata_add(mu, METADATA_USER, "private:mark:reason", info);
			metadata_add(mu, METADATA_USER, "private:mark:timestamp", itoa(time(NULL)));

			wallops("%s marked the user \2%s\2.", origin, target);
			notice(chansvs.nick, origin, "\2%s\2 is now marked.", target);
		}
		else if (!strcasecmp(action, "OFF"))
		{
			if (!metadata_find(mu, METADATA_USER, "private:mark:setter"))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not marked.", target);
				return;
			}

			metadata_delete(mu, METADATA_USER, "private:mark:setter");
			metadata_delete(mu, METADATA_USER, "private:mark:reason");
			metadata_delete(mu, METADATA_USER, "private:mark:timestamp");

			wallops("%s unmarked the user \2%s\2.", origin, target);
			notice(chansvs.nick, origin, "\2%s\2 is now unmarked.", target);
		}
		else
		{
			notice(chansvs.nick, origin, "Insufficient parameters for \2MARK\2.");
			notice(chansvs.nick, origin, "Usage: MARK <target> <ON|OFF> [note]");
		}
	}
}
