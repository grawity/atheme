/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Module management.
 *
 * $Id: module.c 908 2005-07-17 04:00:28Z w00t $
 */

#include "atheme.h"

list_t modules;
static BlockHeap *module_heap;

#ifdef __OpenBSD__
# define RTLD_NOW RTLD_LAZY
#endif

void modules_init(void)
{
	module_heap = BlockHeapCreate(sizeof(module_t), 256);

	if (!module_heap)
	{
		slog(LG_INFO, "modules_init(): block allocator failed.");
		exit(EXIT_FAILURE);
	}

	module_load_dir(PREFIX "/modules");
}

module_t *module_load(char *filespec)
{
	node_t *n;
	module_t *m;
	void *handle;
	void (*modinit)(void);
	char *modname = basename(filespec);

	if ((m = module_find(modname)))
	{
		slog(LG_INFO, "module_load(): module %s is already loaded at [0x%lx]",
				modname, m->address);
		return NULL;
	}

	handle = dlopen(filespec, RTLD_NOW);

	if (!handle)
	{
		char *errp = sstrdup(dlerror());
		slog(LG_INFO, "module_load(): error: %s", errp);
		wallops("Error while loading module %s: %s", modname, errp);
		return NULL;
	}

	modinit = (void (*)(void)) dlsym(handle, "_modinit");

	if (!modinit)
		return NULL;

	slog(LG_DEBUG, "module_load(): loaded %s at [0x%lx]", modname, handle);

	if (me.connected)
		wallops("Module %s loaded at [0x%lx]", modname, handle);

	m = BlockHeapAlloc(module_heap);

	m->name = sstrdup(modname);
	m->modpath = sstrdup(filespec);
	m->address = handle;
	m->mflags = MODTYPE_STANDARD;
	m->deinit = (void (*)(void)) dlsym(handle, "_moddeinit");

	n = node_create();

	node_add(m, n, &modules);

	modinit();

	return m;
}

void module_load_dir(char *dirspec)
{
	DIR *module_dir = NULL;
	struct dirent *ldirent = NULL;
	char module_filename[4096];

	module_dir = opendir(dirspec);

	if (!module_dir)
	{
		slog(LG_INFO, "module_load_dir(): %s: %s", dirspec, strerror(errno));
		return;
	}

	while ((ldirent = readdir(module_dir)) != NULL)
	{
		if (!strstr(ldirent->d_name, ".so"))
			continue;

		snprintf(module_filename, sizeof(module_filename), "%s/%s", dirspec, ldirent->d_name);
		module_load(module_filename);
	}

	closedir(module_dir);
}

void module_unload(module_t *m)
{
	node_t *n;

	if (!m)
		return;

	slog(LG_INFO, "module_unload(): unloaded %s", m->name);

	n = node_find(m, &modules);

	if (m->deinit)
		m->deinit();

	dlclose(m->address);
	free(m->name);

	node_del(n, &modules);

	BlockHeapFree(module_heap, m);
}

module_t *module_find(char *name)
{
	node_t *n;

	LIST_FOREACH(n, modules.head)
	{
		module_t *m = n->data;

		if (!strcasecmp(m->name, name))
			return m;
	}

	return NULL;
}
