/*
 * Copyright (c) 2005 William Pitcock
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Marking for nicknames.
 *
 * $Id: ns_mark.c 882 2005-07-16 17:51:47Z alambert $
 */

#include "atheme.h"

static void ns_cmd_mark(char *origin);

command_t ns_mark = { "MARK", "Adds a note to a user.",
			AC_IRCOP, ns_cmd_mark };

extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
	command_add(&ns_mark, &ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_mark, &ns_cmdtree);
}

static void ns_cmd_mark(char *origin)
{
	char *target = strtok(NULL, " ");
	char *action = strtok(NULL, " ");
	char *info = strtok(NULL, "");
	myuser_t *mu;

	if (!target || !action)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2MARK\2.");
		notice(nicksvs.nick, origin, "Usage: MARK <target> <ON|OFF> [note]");
		return;
	}

	if (!(mu = myuser_find(target)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", target);
		return;
	}

	if (!strcasecmp(action, "ON"))
	{
		if (!info)
		{
			notice(nicksvs.nick, origin, "Insufficient parameters for \2MARK\2.");
			notice(nicksvs.nick, origin, "Usage: MARK <target> ON <note>");
			return;
		}

		if (metadata_find(mu, METADATA_USER, "private:mark:setter"))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is already marked.", target);
			return;
		}

		metadata_add(mu, METADATA_USER, "private:mark:setter", origin);
		metadata_add(mu, METADATA_USER, "private:mark:reason", info);
		metadata_add(mu, METADATA_USER, "private:mark:timestamp", itoa(time(NULL)));

		wallops("%s marked the nickname \2%s\2.", origin, target);
		notice(nicksvs.nick, origin, "\2%s\2 is now marked.", target);
	}
	else if (!strcasecmp(action, "OFF"))
	{
		if (!metadata_find(mu, METADATA_USER, "private:mark:setter"))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not marked.", target);
			return;
		}

		metadata_delete(mu, METADATA_USER, "private:mark:setter");
		metadata_delete(mu, METADATA_USER, "private:mark:reason");
		metadata_delete(mu, METADATA_USER, "private:mark:timestamp");

		wallops("%s unmarked the nickname \2%s\2.", origin, target);
		notice(nicksvs.nick, origin, "\2%s\2 is now unmarked.", target);
	}
	else
	{
		notice(nicksvs.nick, origin, "Insufficient parameters for \2MARK\2.");
		notice(nicksvs.nick, origin, "Usage: MARK <target> <ON|OFF> [note]");
	}
}
