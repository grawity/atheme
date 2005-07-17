/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Loads a new module in.
 *
 * $Id: os_modload.c 952 2005-07-17 22:57:38Z alambert $
 */

#include "atheme.h"

static void os_cmd_modload(char *origin);

command_t os_modload = { "MODLOAD", "Loads a module.",
			 AC_SRA, os_cmd_modload };

extern list_t os_cmdtree;
extern list_t modules;

void _modinit(module_t *m)
{
	m->mflags = MODTYPE_CORE;
	command_add(&os_modload, &os_cmdtree);
}

void _moddeinit()
{
	command_delete(&os_modload, &os_cmdtree);
}

static void os_cmd_modload(char *origin)
{
	char *module;
	char pbuf[BUFSIZE + 1];

	while((module = strtok(NULL, " ")))
	{
		if (*module != '/')
		{
			snprintf(pbuf, BUFSIZE, "%s/%s", PREFIX "/modules",
				 module);
			module_load(pbuf);
		}
		else
			module_load(module);

		notice(opersvs.nick, origin, "Module \2%s\2 loaded.", module);
	}
}
