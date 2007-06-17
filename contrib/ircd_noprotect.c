/*
 * Copyright (c) 2006 Jilles Tjoelker
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module to disable protect (+a) mode.
 * This will stop Atheme setting this mode by itself, but it can still
 * be used via OperServ MODE etc.
 *
 * $Id: ircd_noprotect.c 7785 2007-03-03 15:54:32Z pippijn $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"ircd_noprotect", FALSE, _modinit, _moddeinit,
	"$Id: ircd_noprotect.c 7785 2007-03-03 15:54:32Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

boolean_t oldflag;

void _modinit(module_t *m)
{

	if (ircd == NULL)
	{
		slog(LG_ERROR, "Module %s must be loaded after a protocol module.", m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}
	oldflag = ircd->uses_protect;
	ircd->uses_protect = FALSE;
	update_chanacs_flags();
}

void _moddeinit()
{

	ircd->uses_protect = oldflag;
	update_chanacs_flags();
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
