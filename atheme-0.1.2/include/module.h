/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file contains data structures concerning modules.
 *
 * $Id: module.h 256 2005-05-31 13:50:18Z nenolod $
 */

#ifndef MODULE_H
#define MODULE_H

typedef struct module_ module_t;

struct module_ {
	char *name;
	void *address;
	void (*deinit)();
};

#endif
