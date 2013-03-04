/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the main header file, usually the only one #include'd
 *
 * $Id: atheme.h 162 2005-05-29 07:15:46Z nenolod $
 */

#ifndef SHRIKE_H
#define SHRIKE_H

/* *INDENT-OFF* */

#include "sysconf.h"
#include "stdinc.h"
#include "node.h"
#include "balloc.h"
#include "servers.h"
#include "channels.h"
#include "account.h"
#include "users.h"
#include "common.h"

#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif

/* hashing macros */
#define SHASH(server) shash(server) % HASHSIZE
#define UHASH(nick) shash(nick) % HASHSIZE
#define CHASH(chan) shash(chan) % HASHSIZE
#define MUHASH(myuser) shash(myuser) % HASHSIZE
#define MCHASH(mychan) shash(mychan) % HASHSIZE

/* T Y P E D E F S */

typedef struct tld_ tld_t;
typedef struct kline_ kline_t;
typedef void EVH(void *);

/* S T R U C T U R E S */
struct me
{
  char *name;                   /* server's name on IRC               */
  char *desc;                   /* server's description               */
  char *uplink;                 /* the server we connect to           */
  char *actual;                 /* the reported name of the uplink    */
  uint16_t port;                /* port we connect to our uplink on   */
  char *pass;                   /* password we use for linking        */
  char *vhost;                  /* IP we bind outgoing stuff to       */
  uint16_t recontime;           /* time between reconnection attempts */
  uint16_t restarttime;         /* time before restarting             */
  uint32_t expire;              /* time before registrations expire   */
  char *netname;                /* IRC network name                   */
  char *adminname;              /* SRA's name (for ADMIN)             */
  char *adminemail;             /* SRA's email (for ADMIN             */
  char *mta;                    /* path to mta program           */

  uint8_t loglevel;             /* logging level                      */
  uint32_t maxfd;               /* how many fds do we have?           */
  time_t start;                 /* starting time                      */
  server_t *me;                 /* pointer to our server struct       */
  boolean_t connected;          /* are we connected?                  */
  boolean_t bursting;           /* are we bursting?                   */

  uint16_t maxusers;            /* maximum usernames from one email   */
  uint16_t maxchans;            /* maximum chans from one username    */
  uint8_t auth;                 /* registration auth type             */

  time_t uplinkpong;            /* when the uplink last sent a PONG   */
} me;

#define AUTH_NONE  0
#define AUTH_EMAIL 1

struct ConfOption
{
  char *chan;                   /* channel we join/msg        */

  uint16_t flood_msgs;          /* messages determining flood */
  uint16_t flood_time;          /* time determining flood     */
  uint32_t kline_time;          /* default expire for klines  */

  boolean_t silent;             /* stop sending WALLOPS?      */
  boolean_t join_chans;         /* join registered channels?  */
  boolean_t leave_chans;        /* leave channels when empty? */

  uint16_t defuflags;           /* default username flags     */
  uint16_t defcflags;           /* default channel flags      */

  boolean_t raw;                /* enable raw/inject?         */

  char *global;                 /* nick for global noticer    */
} config_options;

/* keep track of how many of what we have */
struct cnt
{
  uint32_t event;
  uint32_t sra;
  uint32_t tld;
  uint32_t kline;
  uint32_t server;
  uint32_t user;
  uint32_t chan;
  uint32_t chanuser;
  uint32_t myuser;
  uint32_t mychan;
  uint32_t chanacs;
  uint32_t node;
  uint32_t bin;
  uint32_t bout;
} cnt;

#define MTYPE_NUL 0
#define MTYPE_ADD 1
#define MTYPE_DEL 2

struct cmode_
{
        char mode;
        uint32_t value;
};

/* event list struct */
struct ev_entry
{
  EVH *func;
  void *arg;
  const char *name;
  time_t frequency;
  time_t when;
  boolean_t active;
};

/* tld list struct */
struct tld_ {
  char *name;
};

/* kline list struct */
struct kline_ {
  char *user;
  char *host;
  char *reason;
  char *setby;

  uint16_t number;
  long duration;
  time_t settime;
  time_t expires;
};

/* global list struct */
struct global_ {
  char *text;
};

/* sendq struct */
struct sendq {
  char *buf;
  int len;
  int pos;
};

/* database versions */
#define DB_SHRIKE	1
#define DB_ATHEME	2

/* struct for irc message hash table */
struct message_
{
  const char *name;
  void (*func) (char *origin, uint8_t parc, char *parv[]);
};

/* struct for command hash table */
struct command_
{
  const char *name;
  uint8_t access;
  void (*func) (char *nick);
};

/* struct for set command hash table */
struct set_command_
{
  const char *name;
  uint8_t access;
  void (*func) (char *origin, char *name, char *params);
};

/* struct for help command hash table */
struct help_command_
{
  const char *name;
  uint8_t access;
  char *file;
};

#define AC_NONE  0
#define AC_IRCOP 1
#define AC_SRA   2

/* run flags */
int runflags;

#define RF_LIVE         0x00000001      /* don't fork  */
#define RF_SHUTDOWN     0x00000002      /* shut down   */
#define RF_STARTING     0x00000004      /* starting up */
#define RF_RESTART      0x00000008      /* restart     */
#define RF_REHASHING    0x00000010      /* rehashing   */

/* log levels */
#define LG_NONE         0x00000001      /* don't log                */
#define LG_INFO         0x00000002      /* log general info         */
#define LG_ERROR        0x00000004      /* log real important stuff */
#define LG_DEBUG        0x00000008      /* log debugging stuff      */

/* bursting timer */
#if HAVE_GETTIMEOFDAY
struct timeval burstime;
#endif

/* *INDENT-OFF* */

/* down here so stuff it uses in here works */
#include "confparse.h"
#include "flags.h"
#include "extern.h"
#include "services.h"

/* *INDENT-ON* */

#endif /* SHRIKE_H */
