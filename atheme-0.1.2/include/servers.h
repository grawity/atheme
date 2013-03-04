/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to network servers.
 *
 * $Id: servers.h 158 2005-05-29 07:04:02Z nenolod $
 */

#ifndef SERVERS_H
#define SERVERS_H

typedef struct server_ server_t;

/* servers struct */
struct server_
{
  char *name;
  char *desc;

  uint8_t hops;
  uint32_t users;
  uint32_t invis;
  uint32_t opers;
  uint32_t away;

  time_t connected_since;

  uint32_t flags;
  int32_t hash;

  server_t *uplink;
};

#define SF_HIDE        0x00000001

#endif
