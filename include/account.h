/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 *
 * $Id: account.h 175 2005-05-29 08:00:14Z nenolod $
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

typedef struct sra_ sra_t;
typedef struct myuser_ myuser_t;
typedef struct mychan_ mychan_t;
typedef struct chanacs_ chanacs_t;

/* sra list struct */
struct sra_ {
  myuser_t *myuser;
  char *name;
};

struct myuser_
{
  char *name;
  char *pass;
  char *email;

  user_t *user;
  time_t registered;
  time_t lastlogin;

  boolean_t identified;
  uint16_t failnum;
  char *lastfail;
  time_t lastfailon;

  list_t chanacs;
  sra_t *sra;

  unsigned long key;
  char *temp;

  uint32_t flags;
  int32_t hash;
};

#define MU_HOLD        0x00000001
#define MU_NEVEROP     0x00000002
#define MU_NOOP        0x00000004
#define MU_WAITAUTH    0x00000008
#define MU_HIDEMAIL    0x00000010

#define MU_IRCOP       0x00001000
#define MU_SRA         0x00002000

struct mychan_
{
  char *name;
  char *pass;

  myuser_t *founder;
  myuser_t *successor;
  channel_t *chan;
  list_t chanacs;
  time_t registered;
  time_t used;

  int mlock_on;
  int mlock_off;
  int mlock_limit;
  char *mlock_key;

  uint32_t flags;
  int32_t hash;

  char *url;
  char *entrymsg;
};

#define MC_HOLD        0x00000001
#define MC_NEVEROP     0x00000002
#define MC_RESTRICTED  0x00000004
#define MC_SECURE      0x00000008
#define MC_VERBOSE     0x00000010
#define MC_STAFFONLY   0x00000020

/* struct for channel access list */
struct chanacs_
{
  myuser_t *myuser;
  mychan_t *mychan;
  char *host;
  int level;
};

/* the new atheme-style channel flags */
#define CA_VOICE         0x00000001 /* Ability to use voice/devoice command. */
#define CA_AUTOVOICE     0x00000002 /* Gain voice automatically upon entry. */
#define CA_OP            0x00000004 /* Ability to use op/deop command. */
#define CA_AUTOOP        0x00000008 /* Gain ops automatically upon entry. */
#define CA_TOPIC         0x00000010 /* Ability to use /msg X topic */
#define CA_SET           0x00000020 /* Ability to use /msg X set */
#define CA_REMOVE        0x00000040 /* Ability to use /msg X kick */
#define CA_INVITE        0x00000080 /* Ability to use /msg X invite */
#define CA_RECOVER       0x00000100 /* Ability to use /msg X recover */
#define CA_FLAGS         0x00000200 /* Ability to write to channel flags table */
#define CA_AKICK         0x00000400 /* Automatic kick */

/* compatibility shims for current shrike code. */
#define CA_NONE          0x0
#define CA_VOP           (CA_VOICE | CA_AUTOVOICE)
#define CA_AOP           (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC)
#define CA_SOP           (CA_AOP | CA_SET | CA_REMOVE | CA_INVITE)
#define CA_SUCCESSOR     (CA_SOP | CA_RECOVER)
#define CA_FOUNDER       (CA_SUCCESSOR | CA_FLAGS)

/* shrike CA_ flags */
#define SHRIKE_CA_VOP           0x00000002
#define SHRIKE_CA_AOP           0x00000004
#define SHRIKE_CA_SOP           0x00000008
#define SHRIKE_CA_FOUNDER       0x00000010
#define SHRIKE_CA_SUCCESSOR     0x00000020

#endif
