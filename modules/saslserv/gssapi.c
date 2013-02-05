#include "atheme.h"
#include <gsasl.h>

DECLARE_MODULE_V1
(
	"saslserv/gssapi", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Mantas Mikulėnas <grawity@gmail.com>"
);

static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
static int callback(Gsasl *ctx, Gsasl_session *sctx, Gsasl_property prop);
static char * get_fqdn(void);

Gsasl *ctx;

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;

sasl_mechanism_t mech = {"GSSAPI", &mech_start, &mech_step, &mech_finish};
#if 0
sasl_mechanism_t mech[] = {
	{"CRAM-MD5",    &mech_start, &mech_step, &mech_finish},
	{"DIGEST-MD5",  &mech_start, &mech_step, &mech_finish},
	{"GS2-KRB5",    &mech_start, &mech_step, &mech_finish};
	{"GSSAPI",      &mech_start, &mech_step, &mech_finish};
	{"SCRAM-SHA-1", &mech_start, &mech_step, &mech_finish},
	{},
};
#endif

void _modinit(module_t *m)
{
	int rc;

	MODULE_TRY_REQUEST_SYMBOL(m, mechanisms, "saslserv/main", "sasl_mechanisms");

	rc = gsasl_init(&ctx);
	if (rc != GSASL_OK)
	{
		slog(LG_ERROR, "gssapi: Failed to initialize GSASL: (%d) %s",
			rc, gsasl_strerror(rc));
		return;
	}

	gsasl_callback_set(ctx, callback);

	mnode = mowgli_node_create();
	mowgli_node_add(&mech, mnode, mechanisms);
}

void _moddeinit(module_unload_intent_t intent)
{
	if (ctx)
		gsasl_done(ctx);

	mowgli_node_delete(mnode, mechanisms);
	mowgli_node_free(mnode);
}

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	char *mech = p->mechptr->name;
	Gsasl_session *sctx;
	int rc;

	if (!ctx)
		return ASASL_FAIL;

	rc = gsasl_server_start(ctx, mech, &sctx);
	if (rc != GSASL_OK)
	{
		slog(LG_ERROR, "gssapi: Failed to start a GSASL '%s' session: (%d) %s",
			mech, rc, gsasl_strerror(rc));
		return ASASL_FAIL;
	}

	p->mechdata = sctx;
	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	Gsasl_session *sctx = p->mechdata;
	int rc = gsasl_step(sctx, message, (size_t) len, out, (size_t *) out_len);

	if (rc == GSASL_NEEDS_MORE)
	{
		return ASASL_MORE;
	}
	else if (rc == GSASL_OK)
	{
		const char *authid = gsasl_property_get(sctx, GSASL_AUTHID);
		if (!authid)
			return ASASL_FAIL;
		if (!myuser_find(authid))
			return ASASL_FAIL;
		p->username = strdup(authid);
		return ASASL_DONE;
	}
	else
	{
		slog(LG_ERROR, "gssapi: Failed to perform GSASL step: (%d) %s",
			rc, gsasl_strerror(rc));
		return ASASL_FAIL;
	}
}

static void mech_finish(sasl_session_t *p)
{
	Gsasl_session *sctx = p->mechdata;
	if (sctx)
		gsasl_finish(sctx);
}

static int callback(Gsasl *ctx, Gsasl_session *sctx, Gsasl_property prop)
{
	switch (prop)
	{
		case GSASL_SERVICE:
		/* called to obtain GSSAPI service name */
			gsasl_property_set(sctx, prop, "irc");
			return GSASL_OK;

		case GSASL_HOSTNAME:
		/* called to obtain GSSAPI host name */
		{
			char *fqdn = get_fqdn();

			if (!fqdn)
			{
				slog(LG_ERROR, "gssapi: could not obtain my FQDN");
				return GSASL_NO_HOSTNAME;
			}

			slog(LG_ERROR, "gssapi: (debug) using '%s' as my FQDN", fqdn);
			gsasl_property_set(sctx, prop, fqdn);
			return GSASL_OK;
		}

		case GSASL_VALIDATE_GSSAPI:
		/* called to determine whether the principal is authorized */
		{
			const char *princ = gsasl_property_get(sctx, GSASL_GSSAPI_DISPLAY_NAME);
			const char *authzid = gsasl_property_get(sctx, GSASL_AUTHZID);

			mycertfp_t *mcfp;
			char buf[256];
			const char *authcid;

			snprintf(buf, sizeof buf, "GSSAPI:%s", princ);
			slog(LG_ERROR, "gssapi: (debug) trying to find '%s' in certfp list", buf);
			mcfp = mycertfp_find(buf);
			if (mcfp == NULL)
				return GSASL_AUTHENTICATION_ERROR;

			authcid = entity(mcfp->mu)->name;
			if (*authzid && strcmp(authcid, authzid))
				return GSASL_AUTHENTICATION_ERROR;

			gsasl_property_set(sctx, GSASL_AUTHID, authcid);
			return GSASL_OK;
		}

		default:
			return GSASL_NO_CALLBACK;
	}
}

static char *get_fqdn(void)
{
	/* TODO */
	char *host = getenv("FQDN");
	if (host)
		return host;
	else
		return "irc.cluenet.org";
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
