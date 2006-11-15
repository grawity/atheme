/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Message translation stuff.
 * 
 * $Id: culture.h 6803 2006-10-21 19:24:38Z nenolod $
 */

#ifndef CULTURE_H
#define CULTURE_H

struct translation_
{
	char *name;
	char *replacement;
};

typedef struct translation_ translation_t;

E const char *translation_get(const char *name);
E void itranslation_create(char *str, char *trans);
E void itranslation_destroy(char *str);
E void translation_create(char *str, char *trans);
E void translation_destroy(char *str);
E void translation_init(void);

#endif
