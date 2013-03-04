/*
 * Copyright (c) 2005-2006 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 *
 * $Id: account.h 8367 2007-06-02 22:26:48Z jilles $
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

typedef struct chanacs_ chanacs_t;
typedef struct kline_ kline_t;
typedef struct mymemo_ mymemo_t;
typedef struct svsignore_ svsignore_t;

/* kline list struct */
struct kline_ {
  char *user;
  char *host;
  char *reason;
  char *setby;

  unsigned int number;
  long duration;
  time_t settime;
  time_t expires;
};

/* services ignore struct */
struct svsignore_ {
  svsignore_t *svsignore;

  char *mask;
  time_t settime;
  char *setby;
  char *reason;
};

/* services accounts */
struct myuser_
{
  object_t parent;

  char name[NICKLEN];
  char pass[NICKLEN];
  char email[EMAILLEN];

  list_t logins; /* user_t's currently logged in to this */
  time_t registered;
  time_t lastlogin;

  list_t chanacs;
  soper_t *soper;

  list_t metadata;

  unsigned int flags;
  
  list_t memos; /* store memos */
  unsigned int memoct_new;
  unsigned int memo_ratelimit_num; /* memos sent recently */
  time_t memo_ratelimit_time; /* last time a memo was sent */
  list_t memo_ignores;

  /* openservices patch */
  list_t access_list;

  list_t nicks; /* registered nicks, must include mu->name if nonempty */
};

#define MU_HOLD        0x00000001
#define MU_NEVEROP     0x00000002
#define MU_NOOP        0x00000004
#define MU_WAITAUTH    0x00000008
#define MU_HIDEMAIL    0x00000010
#define MU_OLD_ALIAS   0x00000020 /* obsolete */
#define MU_NOMEMO      0x00000040
#define MU_EMAILMEMOS  0x00000080
#define MU_CRYPTPASS   0x00000100
#define MU_OLD_SASL    0x00000200 /* obsolete */
#define MU_NOBURSTLOGIN 0x00000400

/* memoserv rate limiting parameters */
#define MEMO_MAX_NUM   5
#define MEMO_MAX_TIME  180

/* registered nick */
struct mynick_
{
  object_t parent;

  char nick[NICKLEN];

  myuser_t *owner;

  time_t registered;
  time_t lastseen;

  node_t node; /* for myuser_t.nicks */
};

struct mychan_
{
  object_t parent;

  char name[CHANNELLEN];

  myuser_t *founder;
  channel_t *chan;
  list_t chanacs;
  time_t registered;
  time_t used;

  int mlock_on;
  int mlock_off;
  unsigned int mlock_limit;
  char *mlock_key;

  unsigned int flags;

  list_t metadata;
};

#define MC_HOLD        0x00000001
#define MC_NOOP        0x00000002
#define MC_RESTRICTED  0x00000004
#define MC_SECURE      0x00000008
#define MC_VERBOSE     0x00000010
#define MC_STAFFONLY   0x00000020
#define MC_KEEPTOPIC   0x00000040
#define MC_VERBOSE_OPS 0x00000080
#define MC_TOPICLOCK   0x00000100
#define MC_GUARD       0x00000200

/* The following are temporary state */
#define MC_INHABIT     0x80000000 /* we're on channel to enforce akick/staffonly/close */
#define MC_MLOCK_CHECK 0x40000000 /* we need to check mode locks */
#define MC_FORCEVERBOSE 0x20000000 /* fantasy cmd in progress, be verbose */

#define MC_VERBOSE_MASK (MC_VERBOSE | MC_VERBOSE_OPS)

/* struct for channel access list */
struct chanacs_
{
	object_t parent;

	myuser_t *myuser;
	mychan_t *mychan;
	char      host[HOSTLEN];
	unsigned int  level;
	list_t	  metadata;
	time_t    ts;
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
#define CA_HALFOP	 0x00000400 /* Ability to use /msg X halfop */
#define CA_AUTOHALFOP	 0x00000800 /* Gain halfops automatically upon entry. */
#define CA_ACLVIEW	 0x00001000 /* Can view access lists */

/*#define CA_SUSPENDED	 0x40000000 * Suspended access entry (not yet implemented) */
#define CA_AKICK         0x80000000 /* Automatic kick */

#define CA_NONE          0x0

/* xOP defaults, compatible with Atheme 0.3 */
#define CA_VOP_DEF       (CA_VOICE | CA_AUTOVOICE | CA_ACLVIEW)
#define CA_HOP_DEF	 (CA_VOICE | CA_HALFOP | CA_AUTOHALFOP | CA_TOPIC | CA_ACLVIEW)
#define CA_AOP_DEF       (CA_VOICE | CA_HALFOP | CA_OP | CA_AUTOOP | CA_TOPIC | CA_ACLVIEW)
#define CA_SOP_DEF       (CA_AOP_DEF | CA_SET | CA_REMOVE | CA_INVITE)

/* special values for founder/successor -- jilles */
/* used in shrike flatfile conversion: */
#define CA_SUCCESSOR_0   (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_ACLVIEW)
/* granted to new founder on transfer etc: */
#define CA_FOUNDER_0     (CA_SUCCESSOR_0 | CA_FLAGS)
/* granted to founder on new channel: */
#define CA_INITIAL       (CA_FOUNDER_0 | CA_AUTOOP)

/* joining with one of these flags updates used time */
#define CA_USEDUPDATE    (CA_VOICE | CA_OP | CA_AUTOOP | CA_SET | CA_REMOVE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP)
#define CA_ALLPRIVS      (CA_VOICE | CA_AUTOVOICE | CA_OP | CA_AUTOOP | CA_TOPIC | CA_SET | CA_REMOVE | CA_INVITE | CA_RECOVER | CA_FLAGS | CA_HALFOP | CA_AUTOHALFOP | CA_ACLVIEW)
#define CA_ALL_ALL       (CA_ALLPRIVS | CA_AKICK)

/* old CA_ flags */
#define OLD_CA_AOP           (CA_VOICE | CA_OP | CA_AUTOOP | CA_TOPIC)

/* shrike CA_ flags */
#define SHRIKE_CA_VOP           0x00000002
#define SHRIKE_CA_AOP           0x00000004
#define SHRIKE_CA_SOP           0x00000008
#define SHRIKE_CA_FOUNDER       0x00000010
#define SHRIKE_CA_SUCCESSOR     0x00000020

/* struct for account memos */
struct mymemo_ {
	char	 sender[NICKLEN];
	char 	 text[MEMOLEN];
	time_t	 sent;
	unsigned int status;
	list_t	 metadata;
};

/* memo status flags */
#define MEMO_READ          0x00000001
#define MEMO_CHANNEL       0x00000002

/* account related hooks */
typedef struct {
	user_t *u;
	mychan_t *mc;
	sourceinfo_t *si;
} hook_channel_req_t;

typedef struct {
	sourceinfo_t *si;
	myuser_t *mu;
	mynick_t *mn;
} hook_user_req_t;

typedef struct {
	sourceinfo_t *si;
	const char *account;
	const char *email;
	int approved; /* Write non-zero here to disallow the registration */
} hook_user_register_check_t;

typedef struct {
	user_t *u;
	mynick_t *mn;
} hook_nick_enforce_t;

/* pmodule.c XXX */
E boolean_t backend_loaded;

/* dbhandler.c */
E void (*db_save)(void *arg);
E void (*db_load)(void);

/* function.c */
E boolean_t is_founder(mychan_t *mychan, myuser_t *myuser);
E boolean_t should_owner(mychan_t *mychan, myuser_t *myuser);
E boolean_t should_protect(mychan_t *mychan, myuser_t *myuser);

E void set_password(myuser_t *mu, char *newpassword);
E boolean_t verify_password(myuser_t *mu, char *password);

/* node.c */
E list_t klnlist;

E kline_t *kline_add(char *user, char *host, char *reason, long duration);
E void kline_delete(const char *user, const char *host);
E kline_t *kline_find(const char *user, const char *host);
E kline_t *kline_find_num(unsigned int number);
E kline_t *kline_find_user(user_t *u);
E void kline_expire(void *arg);

/* account.c */
E dictionary_tree_t *mulist;
E dictionary_tree_t *nicklist;
E dictionary_tree_t *mclist;

E void init_accounts(void);

E myuser_t *myuser_add(char *name, char *pass, char *email, unsigned int flags);
E void myuser_delete(myuser_t *mu);
E myuser_t *myuser_find(const char *name);
E myuser_t *myuser_find_ext(const char *name);
E void myuser_notice(char *from, myuser_t *target, char *fmt, ...);

E boolean_t myuser_access_verify(user_t *u, myuser_t *mu);
E boolean_t myuser_access_add(myuser_t *mu, char *mask);
E char *myuser_access_find(myuser_t *mu, char *mask);
E void myuser_access_delete(myuser_t *mu, char *mask);

E mynick_t *mynick_add(myuser_t *mu, const char *name);
E void mynick_delete(mynick_t *mn);
E mynick_t *mynick_find(const char *name);

E mychan_t *mychan_add(char *name);
E mychan_t *mychan_find(const char *name);
E boolean_t mychan_isused(mychan_t *mc);
E myuser_t *mychan_pick_candidate(mychan_t *mc, unsigned int minlevel, int maxtime);
E myuser_t *mychan_pick_successor(mychan_t *mc);

E chanacs_t *chanacs_add(mychan_t *mychan, myuser_t *myuser, unsigned int level, time_t ts);
E chanacs_t *chanacs_add_host(mychan_t *mychan, const char *host, unsigned int level, time_t ts);

E chanacs_t *chanacs_find(mychan_t *mychan, myuser_t *myuser, unsigned int level);
E chanacs_t *chanacs_find_host(mychan_t *mychan, const char *host, unsigned int level);
E unsigned int chanacs_host_flags(mychan_t *mychan, const char *host);
E chanacs_t *chanacs_find_host_literal(mychan_t *mychan, const char *host, unsigned int level);
E chanacs_t *chanacs_find_host_by_user(mychan_t *mychan, user_t *u, unsigned int level);
E unsigned int chanacs_host_flags_by_user(mychan_t *mychan, user_t *u);
E chanacs_t *chanacs_find_by_mask(mychan_t *mychan, const char *mask, unsigned int level);
E boolean_t chanacs_user_has_flag(mychan_t *mychan, user_t *u, unsigned int level);
E unsigned int chanacs_user_flags(mychan_t *mychan, user_t *u);
E boolean_t chanacs_source_has_flag(mychan_t *mychan, sourceinfo_t *si, unsigned int level);
E unsigned int chanacs_source_flags(mychan_t *mychan, sourceinfo_t *si);

chanacs_t *chanacs_open(mychan_t *mychan, myuser_t *mu, const char *hostmask, boolean_t create);
void chanacs_close(chanacs_t *ca);
boolean_t chanacs_modify(chanacs_t *ca, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags);
boolean_t chanacs_modify_simple(chanacs_t *ca, unsigned int addflags, unsigned int removeflags);

E boolean_t chanacs_change(mychan_t *mychan, myuser_t *mu, const char *hostmask, unsigned int *addflags, unsigned int *removeflags, unsigned int restrictflags);
E boolean_t chanacs_change_simple(mychan_t *mychan, myuser_t *mu, const char *hostmask, unsigned int addflags, unsigned int removeflags);

E void expire_check(void *arg);
/* Check the database for (version) problems common to all backends */
E void db_check(void);

/* svsignore.c */
E list_t svs_ignore_list;

E svsignore_t *svsignore_find(user_t *user);
E svsignore_t *svsignore_add(char *mask, char *reason);
E void svsignore_delete(svsignore_t *svsignore);

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
