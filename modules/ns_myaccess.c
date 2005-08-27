/*
 * Copyright (c) 2005 Alex Lambert
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ MYACCESS function.
 *
 * $Id: ns_register.c 753 2005-07-14 01:14:30Z alambert $
 */

#include "atheme.h"

static void ns_cmd_myaccess(char *origin);

command_t ns_myaccess = { "MYACCESS", "Lists channels that you have access to.", AC_NONE, ns_cmd_myaccess };

extern list_t ns_cmdtree;

void _modinit(module_t *m)
{
	command_add(&ns_myaccess, &ns_cmdtree);
}

void _moddeinit()
{
	command_delete(&ns_myaccess, &ns_cmdtree);
}

static void ns_cmd_myaccess(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu = myuser_find(origin);
	node_t *n;
	chanacs_t *ca;
	uint32_t i, matches = 0;

	if (!(mu = myuser_find(origin)))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not registered.", origin);
		return;
	}

	if (mu != u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	LIST_FOREACH(n, mu->chanacs.head)
	{
       	        ca = (chanacs_t *)n->data;

		switch (ca->level)
		{
			case CA_VOP:
				notice(nicksvs.nick, origin, "VOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_HOP:
				notice(nicksvs.nick, origin, "HOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_AOP:
				notice(nicksvs.nick, origin, "AOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_SOP:
				notice(nicksvs.nick, origin, "SOP in %s", ca->mychan->name);
				matches++;
				break;
			case CA_SUCCESSOR:
				notice(nicksvs.nick, origin, "Successor of %s", ca->mychan->name);
				matches++;
				break;
			case CA_FOUNDER:	/* equiv to is_founder() */
				notice(nicksvs.nick, origin, "Founder of %s", ca->mychan->name);
				matches++;
				break;
			default:
				/* a user may be AKICKed from a room he doesn't know about */
				if (!(ca->level & CA_AKICK))
				{
					notice(nicksvs.nick, origin, "%s in %s", bitmask_to_flags(ca->level, chanacs_flags), ca->mychan->name);
					matches++;
				}
		}
	}

	if (matches == 0)
		notice(nicksvs.nick, origin, "No channel access was found for the nickname \2%s\2.", origin);
	else
		notice(nicksvs.nick, origin, "\2%d\2 channel access match%s for the nickname \2%s\2",
							matches, (matches != 1) ? "es" : "", origin);
}
