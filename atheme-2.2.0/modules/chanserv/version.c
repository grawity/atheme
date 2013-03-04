#include "atheme.h"

DECLARE_MODULE_V1
(
	"chanserv/version", FALSE, _modinit, _moddeinit,
	"$Id: version.c 7855 2007-03-06 00:43:08Z pippijn $",
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_version = { "VERSION", N_("Displays version information of the services."),
                        AC_NONE, 0, cs_cmd_version };

list_t *cs_cmdtree;

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(cs_cmdtree, "chanserv/main", "cs_cmdtree");

        command_add(&cs_version, cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_version, cs_cmdtree);
}

static void cs_cmd_version(sourceinfo_t *si, int parc, char *parv[])
{
        char buf[BUFSIZE];
        snprintf(buf, BUFSIZE, "Atheme %s [%s] #%s", version, revision, generation);
        command_success_string(si, buf, "%s", buf);
        return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
