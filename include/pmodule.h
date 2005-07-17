/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Protocol module stuff.
 *
 * $Id: pmodule.h 698 2005-07-10 01:13:22Z nenolod $
 */

#ifndef PMODULE_H
#define PMODULE_H

typedef struct pcommand_ pcommand_t;

struct pcommand_ {
	char	*token;
	void	(*handler)(char *origin, uint8_t parc, char *parv[]);
};

extern list_t pcommands;

extern void pcommand_init(void);
extern void pcommand_add(char *token,
        void (*handler)(char *origin, uint8_t parc, char *parv[]));
extern void pcommand_delete(char *token);
extern pcommand_t *pcommand_find(char *token);

extern boolean_t pmodule_loaded;
extern boolean_t backend_loaded;

#endif
