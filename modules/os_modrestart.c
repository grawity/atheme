/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module listing.
 *
 * $Id: os_modrestart.c 952 2005-07-17 22:57:38Z alambert $
 */

#include "atheme.h"

static void os_cmd_modrestart(char *origin);

command_t os_modrestart = { "MODRESTART", "Restarts loaded modules.",
			    AC_SRA, os_cmd_modrestart };

extern list_t os_cmdtree;
extern list_t modules;

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;
	command_add(&os_modrestart, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_modrestart, &os_cmdtree);
}

static void os_cmd_modrestart(char *origin)
{
	node_t *n;
	uint32_t iter = 0;
	uint32_t reloaded = 0;

	snoop("MODRESTART: \2%s\2", origin);
	wallops("Restarting modules by request of \2%s\2", origin);

	LIST_FOREACH(n, modules.head)
	{
		module_t *m = n->data;

		/* don't touch core modules */
		if (strcasecmp(m->name, "os_modrestart.so") && !(m->mflags & MODTYPE_CORE))
		{
			char *modpath = sstrdup(m->modpath);
			module_unload(m);
			module_load(modpath);
			reloaded++;
		}

		/* Have we unloaded all the modules? */
		if (iter == (modules.count - 1))
			break;

		iter++;
	}

	module_load_dir(PREFIX "/modules");

	notice(opersvs.nick, origin, "Module restart: %d modules reloaded; %d modules now loaded", reloaded, modules.count);
}
