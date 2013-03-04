/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures related to services psuedo-clients.
 *
 * $Id: services.h 320 2005-06-02 20:31:57Z nenolod $
 */

#ifndef SERVICES_H
#define SERVICES_H

/* core services */
struct chansvs
{
  char *nick;                   /* the IRC client's nickname  */
  char *user;                   /* the IRC client's username  */
  char *host;                   /* the IRC client's hostname  */
  char *real;                   /* the IRC client's realname  */

  user_t *me;                   /* our user_t struct          */
} chansvs;

struct globsvs
{
  char *nick;
  char *user;
  char *host;
  char *real;
   
  user_t *me;
} globsvs;

struct opersvs
{
  char *nick;
  char *user;
  char *host;
  char *real;
   
  user_t *me;
} opersvs;

/* optional services */
struct nicksvs
{
  boolean_t  enable;

  char   *nick;
  char   *user;
  char   *host;
  char   *real;

  user_t *me;
} nicksvs;

#endif
