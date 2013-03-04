/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: os_modunload.c 952 2005-07-17 22:57:38Z alambert $
 */

#include "atheme.h"

static void os_cmd_modunload(char *origin);

command_t os_modunload = { "MODUNLOAD", "Unloads a module.",
			 AC_SRA, os_cmd_modunload };

extern list_t os_cmdtree;
extern list_t modules;

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;
	command_add(&os_modunload, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_modunload, &os_cmdtree);
}

static void os_cmd_modunload(char *origin)
{
	char *module;

	while((module = strtok(NULL, " ")))
	{
		module_t *m = module_find(module);

		if (!m)
			continue;

		if (m->mflags & MODTYPE_CORE)
		{
			slog(LG_INFO, "%s tried to unload a core module",
				origin);
			continue;
		}

		module_unload(m);

		notice(opersvs.nick, origin, "Module \2%s\2 unloaded.", module);
	}
}
