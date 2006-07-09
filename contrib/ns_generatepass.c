/*
 * Copyright (c) 2005 Greg Feigenson
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generates a new password, either n digits long (w/ nickserv arg), or 7 digits
 *
 * $Id: ns_generatepass.c 5804 2006-07-09 14:37:47Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/generatepass", FALSE, _modinit, _moddeinit,
	"$Id: ns_generatepass.c 5804 2006-07-09 14:37:47Z jilles $",
	"Epiphanic Networks <http://www.epiphanic.org>"
);

static void ns_cmd_generatepass(char *origin);

command_t ns_generatepass = { "GENERATEPASS", "Generates a random password.",
                        AC_NONE, ns_cmd_generatepass };
                                                                                   
list_t *ns_cmdtree;
list_t *ns_helptree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(ns_cmdtree, "nickserv/main", "ns_cmdtree");
	MODULE_USE_SYMBOL(ns_helptree, "nickserv/main", "ns_helptree");

	command_add(&ns_generatepass, ns_cmdtree);
	help_addentry(ns_helptree, "GENERATEPASS", "help/nickserv/generatepass", NULL);
	
	/* You'll need to create a helpfile and put in help/nickserv */
}

void _moddeinit()
{
	command_delete(&ns_generatepass, ns_cmdtree);
	help_delentry(ns_helptree, "GENERATEPASS");
}

static void ns_cmd_generatepass(char *origin)
{
	int n = 0;
	char *arg = strtok(NULL, " ");
	char *newpass;
   
	if (arg)
		n = atoi(arg);

	if (n <= 0 || n > 127)
		n = 7;

	newpass = gen_pw(n);

	notice(nicksvs.nick, origin, "Randomly generated password: %s",newpass);
	free(newpass);
}
