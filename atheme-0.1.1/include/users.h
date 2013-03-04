/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for connected clients.
 *
 * $Id: users.h 158 2005-05-29 07:04:02Z nenolod $
 */

#ifndef USERS_H
#define USERS_H

struct user_
{
  char *nick;
  char *user;
  char *host;

  list_t channels;

  server_t *server;
  myuser_t *myuser;

  uint8_t offenses;
  uint8_t msgs;
  time_t lastmsg;

  uint32_t flags;
  int32_t hash;
};

#define UF_ISOPER      0x00000001
#define UF_ISAWAY      0x00000002
#define UF_INVIS       0x00000004
#define UF_LOGGEDIN    0x00000008
#define UF_IRCOP       0x00000010
#define UF_ADMIN       0x00000020
#define UF_SRA         0x00000040

#endif
