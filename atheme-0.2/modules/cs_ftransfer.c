/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FTRANSFER function.
 *
 * $Id: cs_register.c 753 2005-07-14 01:14:30Z alambert $
 */

#include "atheme.h"

static void cs_cmd_ftransfer(char *origin);

command_t cs_ftransfer = { "FTRANSFER", "Forces foundership transfer of a channel.",
                           AC_IRCOP, cs_cmd_ftransfer };
                                                                                   
extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_ftransfer, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_ftransfer, &cs_cmdtree);
}

static void cs_cmd_ftransfer(char *origin)
{
	myuser_t *tmu;
	mychan_t *tmc;
	char *name = strtok(NULL, " ");
	char *newfndr = strtok(NULL, " ");
	/* in the future, we could generate a random password and SENDPASS it */
	char *pass = strtok(NULL, " ");

	if (!name || !pass || !newfndr)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2FTRANSFER\2.");
		notice(chansvs.nick, origin, "Syntax: FTRANSFER <#channel> <newfounder> <newpassword>");
		return;
	}

	if (strlen(pass) > 32)
	{
		notice(chansvs.nick, origin, "Invalid parameters specified for \2REGISTER\2.");
		return;
	}

	if (!(tmu = myuser_find(newfndr)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", newfndr);
		return;
	}

	if (!(tmc = mychan_find(name)))
	{
		notice(chansvs.nick, origin, "\2%s\2 is not registered.", name);
		return;
	}

	if (tmu == tmc->founder)
	{
		notice(chansvs.nick, origin, "\2%s\2 is already the founder of \2%s\2.", newfndr, name);
		return;
	}

	/* no maxchans check (intentional -- this is an oper command) */

	snoop("FTRANSFER: %s transferred %s from %s to %s", origin, name, tmc->founder->name, newfndr);
	wallops("%s transferred foundership of %s from %s to %s", origin, name, tmc->founder->name, newfndr);
	notice(chansvs.nick, origin, "Foundership of \2%s\2 has been transferred from \2%s\2 to \2%s\2.",
		name, tmc->founder->name, newfndr);

	/* from cs_set_founder */
	chanacs_delete(tmc, tmc->founder, CA_FOUNDER);
	chanacs_delete(tmc, tmu, CA_VOP);
	chanacs_delete(tmc, tmu, CA_HOP);
	chanacs_delete(tmc, tmu, CA_AOP);
	chanacs_delete(tmc, tmu, CA_SOP);
	tmc->founder = tmu;
	tmc->used = CURRTIME;
	free(tmc->pass);
	tmc->pass = sstrdup(pass);
	chanacs_add(tmc, tmu, CA_FOUNDER);
}
