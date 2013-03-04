/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module listing.
 *
 * $Id: os_modlist.c 952 2005-07-17 22:57:38Z alambert $
 */

#include "atheme.h"

static void os_cmd_modlist(char *origin);

command_t os_modlist = { "MODLIST", "Lists loaded modules.",
			 AC_SRA, os_cmd_modlist };

extern list_t os_cmdtree;
extern list_t modules;

void _modinit(module_t *m)
{
	command_add(&os_modlist, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_modlist, &os_cmdtree);
}

static void os_cmd_modlist(char *origin)
{
	node_t *n;
	uint16_t i = 0;
	notice(opersvs.nick, origin, "Loaded modules:");

	LIST_FOREACH(n, modules.head)
	{
		module_t *m = n->data;

		notice(opersvs.nick, origin, "%-1d: %-20s [loaded at 0x%lx]", ++i, m->name, m->address);
	}

	notice(opersvs.nick, origin, "\2%d\2 modules loaded.", i);
}
