/*
 * Copyright (c) 2005 Greg Feigenson
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Generates a new password, either n digits long (w/ nickserv arg), or 7 digits
 *
 * $Id: ns_generatepass.c 7785 2007-03-03 15:54:32Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/generatepass", FALSE, _modinit, _moddeinit,
	"$Id: ns_generatepass.c 7785 2007-03-03 15:54:32Z pippijn $",
	"Epiphanic Networks <http://www.epiphanic.org>"
);

static void ns_cmd_generatepass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_generatepass = { "GENERATEPASS", "Generates a random password.",
                        AC_NONE, 1, ns_cmd_generatepass };
                                                                                   
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

static void ns_cmd_generatepass(sourceinfo_t *si, int parc, char *parv[])
{
	int n = 0;
	char *newpass;
   
	if (parc >= 1)
		n = atoi(parv[0]);

	if (n <= 0 || n > 127)
		n = 7;

	newpass = gen_pw(n);

	command_success_string(si, newpass, "Randomly generated password: %s", newpass);
	free(newpass);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
