/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file contains data structures concerning modules.
 *
 * $Id: module.h 685 2005-07-08 04:19:05Z nenolod $
 */

#ifndef MODULE_H
#define MODULE_H

typedef struct module_ module_t;

struct module_ {
	char *name;
	char *modpath;

	uint16_t mflags;

	void *address;
	void (*deinit)(void);
};

#define MODTYPE_STANDARD	0
#define MODTYPE_CORE		1 /* Can't be unloaded. */

extern void _modinit(module_t *m);
extern void _moddeinit(void);

extern void modules_init(void);
extern module_t *module_load(char *filespec);
extern void module_load_dir(char *dirspec);
extern void module_unload(module_t *m);
extern module_t *module_find(char *name);

#endif
