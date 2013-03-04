/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains a generic help system implementation.
 *
 * $Id: help.c 74 2005-05-23 02:33:37Z nenolod $
 */

#include "atheme.h"

struct help_command_ *help_cmd_find(char *svs, char *origin, char *command, struct help_command_ table[])
{
	user_t *u = user_find(origin);
	struct help_command_ *c;

	for (c = table; c->name; c++)
	{
		if (!strcasecmp(command, c->name))
		{
			/* no special access required, so go ahead... */
			if (c->access == AC_NONE)
				return c;

			/* sra? */
			if ((c->access == AC_SRA) && (is_sra(u->myuser)))
				return c;

			/* ircop? */
			if ((c->access == AC_IRCOP) && (is_sra(u->myuser) || (is_ircop(u))))
				return c;

			/* otherwise... */
			else
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(svs, origin, "No help available for \2%s\2.", command);
	return NULL;
}
