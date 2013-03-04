/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService RECOVER functions.
 *
 * $Id: cs_recover.c 826 2005-07-16 06:22:04Z w00t $
 */

#include "atheme.h"

static void cs_cmd_recover(char *origin);

command_t cs_recover = { "RECOVER", "Regain control of your channel.",
                        AC_NONE, cs_cmd_recover };

extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_recover, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_recover, &cs_cmdtree);
}

static void cs_cmd_recover(char *origin)
{
	user_t *u = user_find(origin);
	chanuser_t *cu;
	mychan_t *mc;
	node_t *n;
	char *name = strtok(NULL, " ");
	char hostbuf[BUFSIZE];

	if (!name)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2RECOVER\2.");
		notice(chansvs.nick, origin, "Syntax: RECOVER <#channel>");
		return;
	}

	if (!u->myuser)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	if (!(mc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if ((!is_founder(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_RECOVER)))
	{
		notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
		return;
	}

	verbose(mc, "\2%s\2 used RECOVER.", origin);

	/* deop everyone */
	LIST_FOREACH(n, mc->chan->members.head)
	{
		cu = (chanuser_t *)n->data;

		if ((CMODE_OP & cu->modes) && (irccasecmp(chansvs.nick, cu->user->nick)))
			cmode(chansvs.nick, mc->chan->name, "-o", cu->user->nick);
	}

	/* remove modes that keep people out */
	if (CMODE_LIMIT & mc->chan->modes)
		cmode(chansvs.nick, mc->chan->name, "-l", NULL);

	if (CMODE_INVITE & mc->chan->modes)
		cmode(chansvs.nick, mc->chan->name, "-i", NULL);

	if (CMODE_KEY & mc->chan->modes)
		cmode(chansvs.nick, mc->chan->name, "-k", mc->chan->key);

	/* set an exempt on the user calling this */
	hostbuf[0] = '\0';

	strlcat(hostbuf, u->nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, u->user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, u->host, BUFSIZE);

	sts(":%s MODE %s +e %s", chansvs.nick, mc->chan->name, hostbuf);

	/* invite them back. */
	sts(":%s INVITE %s %s", chansvs.nick, u->nick, mc->chan->name);

	notice(chansvs.nick, origin, "Recover complete for \2%s\2.", mc->chan->name);
}
