/*
 * Copyright (C) 2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for sourceinfo
 *
 * $Id: sourceinfo.h 6669 2006-10-06 00:13:15Z jilles $
 */

#ifndef SOURCEINFO_H
#define SOURCEINFO_H

struct sourceinfo_vtable
{
	const char *description;
	void (*cmd_fail)(sourceinfo_t *si, faultcode_t code, const char *message);
	void (*cmd_success_nodata)(sourceinfo_t *si, const char *message);
	void (*cmd_success_string)(sourceinfo_t *si, const char *result, const char *message);
};

/* structure describing data about a protocol message or service command */
struct sourceinfo_
{
	/* fields describing the source of the message */
	/* for protocol modules, the following applies to su and s:
	 * at most one of these two can be non-NULL
	 * before server registration, both are NULL, otherwise exactly
	 * one is NULL.
	 * for services commands, s is always NULL and su is non-NULL if
	 * and only if the command was received via IRC.
	 */
	user_t *su; /* source, if it's a user */
	server_t *s; /* source, if it's a server */

	connection_t *connection; /* physical connection cmd received from */
	const char *sourcedesc; /* additional information (e.g. IP address) */
	myuser_t *smu; /* login associated with source */

	service_t *service; /* destination service */

	channel_t *c; /* channel this command applies to (fantasy?) */

	struct sourceinfo_vtable *v; /* function pointers, could be NULL */
	void *callerdata; /* opaque data pointer for caller */
};

#endif
