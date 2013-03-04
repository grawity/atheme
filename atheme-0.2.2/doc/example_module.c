/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented in doc/LICENSE.
 *
 * This file is an example module used for testing the module loader with.
 *
 * $Id: example_module.c 476 2005-06-10 19:48:12Z nenolod $
 */

#include "atheme.h"

void _modinit() {
	slog(LG_INFO, "example.so loaded and we did not crash -- sweet.");
}

void _moddeinit() {
	slog(LG_INFO, "example.so unloaded and we did not crash -- kickin'.");
}

char *_version = "$Id: example_module.c 476 2005-06-10 19:48:12Z nenolod $";

