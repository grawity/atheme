/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Headers for service selection tree.
 *
 * $Id: servtree.h 607 2005-06-27 03:58:48Z nenolod $
 */

#ifndef SERVTREE_H
#define SERVTREE_H

struct service_ {
	char *name;
	char *user;
	char *host;
	char *real;
	char *disp;

	uint32_t hash;
	node_t *node;
	user_t *me;

	void (*handler)(char *, uint8_t, char **);
};

typedef struct service_ service_t;

extern void servtree_init(void);
extern service_t *add_service(char *name, char *user, char *host, char *real,
        void (*handler)(char *origin, uint8_t parc, char *parv[]));
extern void del_service(service_t *sptr);
extern service_t *find_service(char *name);
extern char *service_name(char *name);

#endif
