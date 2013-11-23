/*
 * Copyright (c) 2009 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Dummy authentication, always returns failure.
 * This really only serves to have an auth module that always compiles.
 */

#include "atheme.h"

#include <curl/curl.h>

DECLARE_MODULE_V1
(
	"auth/http", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

struct authdata {
	myuser_t *mu,
	const char *pw
};

static size_t authpostdata(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct authdata *state = userdata;


}

static bool dummy_auth_user(myuser_t *mu, const char *password)
{
	CURL *ch = NULL;
	bool ok = false;
	struct authdata *state = NULL;

	state = malloc(sizeof *state);
	if (!state)
		goto fin;

	*state = (struct authdata) { .mu = mu, .pw = password };

	ch = curl_easy_init();
	if (!ch) goto fin;
	
	cr = curl_easy_setopt(ch, CURLOPT_VERBOSE, 1);
	if (cr) goto fin;

	cr = curl_easy_setopt(ch, CURLOPT_URL, "http://localhost/foo");
	if (cr) goto fin;

	cr = curl_easy_setopt(ch, CURLOPT_POST, 1);
	if (cr) goto fin;

	cr = curl_easy_setopt(ch, CURLOPT_TIMEOUT, 5);
	if (cr) goto fin;

	cr = curl_easy_setopt(ch, CURLOPT_READFUNCTION, authpostdata);

	cr = curl_easy_setopt(ch, CURLOPT_READDATA, state);
	if (cr) goto fin;

	cr = curl_easy_perform(ch);
	if (cr) goto fin;

fin:
	if (state) free(state);
	if (ch) curl_easy_cleanup(ch);
	return ok;
}

void _modinit(module_t *m)
{
	if (curl_global_init(CURL_GLOBAL_ALL) == 0) {
		auth_user_custom = &dummy_auth_user;
		auth_module_loaded = true;
	}
}

void _moddeinit(module_unload_intent_t intent)
{
	auth_user_custom = NULL;
	auth_module_loaded = false;

	curl_global_cleanup();
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
