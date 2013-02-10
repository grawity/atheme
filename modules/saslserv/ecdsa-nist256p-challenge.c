/*
 * Copyright (c) 2013 William Pitcock <nenolod@dereferenced.org>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ECDSA-NIST256P-CHALLENGE mechanism provider.
 */

#include "atheme.h"

#ifdef HAVE_OPENSSL

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#define CHALLENGE_LENGTH	SHA256_DIGEST_LENGTH
#define CURVE_IDENTIFIER	NID_X9_62_prime256v1

DECLARE_MODULE_V1
(
	"saslserv/ecdsa-nist256p-challenge", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

mowgli_list_t *mechanisms;
mowgli_node_t *mnode;
static int mech_start(sasl_session_t *p, char **out, int *out_len);
static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len);
static void mech_finish(sasl_session_t *p);
sasl_mechanism_t mech = {"ECDSA-NIST256P-CHALLENGE", &mech_start, &mech_step, &mech_finish};

typedef struct {
	EC_KEY *pubkey;
	unsigned char challenge[CHALLENGE_LENGTH];
} ecdsa_session_t;

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, mechanisms, "saslserv/main", "sasl_mechanisms");
	mnode = mowgli_node_create();
	mowgli_node_add(&mech, mnode, mechanisms);
}

void _moddeinit(module_unload_intent_t intent)
{
	mowgli_node_delete(mnode, mechanisms);
}

static int mech_start(sasl_session_t *p, char **out, int *out_len)
{
	ecdsa_session_t *s = mowgli_alloc(sizeof(ecdsa_session_t));
	p->mechdata = s;

	s->pubkey = EC_KEY_new_by_curve_name(CURVE_IDENTIFIER);

	EC_KEY_set_conv_form(s->pubkey, POINT_CONVERSION_COMPRESSED);

#ifndef DEBUG_STATIC_CHALLENGE_VECTOR
	RAND_pseudo_bytes(s->challenge, CHALLENGE_LENGTH);
#else
	memset(s->challenge, 'A', CHALLENGE_LENGTH);
#endif

	*out = malloc(400);
	memcpy(*out, s->challenge, CHALLENGE_LENGTH);
	*out_len = CHALLENGE_LENGTH;

	return ASASL_MORE;
}

static int mech_step(sasl_session_t *p, char *message, int len, char **out, int *out_len)
{
	ecdsa_session_t *s = p->mechdata;
	char username[256];
	char *response;
	char *end;
	int response_len;
	myuser_t *mu;
	unsigned char pubkey_raw[BUFSIZE];
	const unsigned char *pubkey_raw_p;
	metadata_t *md;
	int ret;

	memset(pubkey_raw, '\0', sizeof pubkey_raw);

	/* Skip the authzid entirely */
	end = memchr(message, '\0', len);
	if (end == NULL)
		return ASASL_FAIL;
	len -= end - message + 1;
	if (len <= 0)
		return ASASL_FAIL;
	message = end + 1;

	/* Copy the authcid */
	end = memchr(message, '\0', len);
	if (end == NULL)
		return ASASL_FAIL;
	if (end - message > 255)
		return ASASL_FAIL;
	len -= end - message + 1;
	if (len <= 0)
		return ASASL_FAIL;
	memcpy(username, message, end - message + 1);
	message = end + 1;

	p->username = sstrdup(username);

	mu = myuser_find_by_nick(p->username);
	if (mu == NULL)
		return ASASL_FAIL;

	md = metadata_find(mu, "pubkey");
	if (md == NULL)
		return ASASL_FAIL;

	ret = base64_decode(md->value, pubkey_raw, BUFSIZE);
	if (ret == -1)
		return ASASL_FAIL;

	pubkey_raw_p = pubkey_raw;
	o2i_ECPublicKey(&s->pubkey, &pubkey_raw_p, ret);

	if (!ECDSA_verify(0, s->challenge, CHALLENGE_LENGTH, message, len, s->pubkey))
		return ASASL_FAIL;

	return ASASL_DONE;
}

static void mech_finish(sasl_session_t *p)
{
	ecdsa_session_t *s = p->mechdata;

	if (s->pubkey != NULL)
		EC_KEY_free(s->pubkey);

	mowgli_free(s);
}

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
