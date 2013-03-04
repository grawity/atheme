/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for flags to bitmask processing routines.
 *
 * $Id: confparse.h 155 2005-05-29 06:45:10Z nenolod $
 */

#ifndef CONFPARSE_H
#define CONFPARSE_H

typedef struct _configfile CONFIGFILE;
typedef struct _configentry CONFIGENTRY;

struct _configfile
{
  char *cf_filename;
  CONFIGENTRY *cf_entries;
  CONFIGFILE *cf_next;
};

struct _configentry
{
  CONFIGFILE    *ce_fileptr;

  int ce_varlinenum;
  char *ce_varname;
  char *ce_vardata;
  int ce_vardatanum;
  int ce_fileposstart;
  int ce_fileposend;

  int ce_sectlinenum;
  CONFIGENTRY *ce_entries;

  CONFIGENTRY *ce_prevlevel;

  CONFIGENTRY *ce_next;
};

struct Token
{
  const char *text;
  int value;
};

#endif
