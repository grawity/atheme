/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This code contains the channel mode definitions for ratbox ircd.
 *
 * $Id: inspircd.h 265 2005-05-31 16:07:22Z nenolod $
 */

#ifndef RATBOX_H
#define RATBOX_H

#define CMODE_BAN       0x00000000      /* IGNORE */
#define CMODE_EXEMPT    0x00000000      /* IGNORE */ 
#define CMODE_INVEX     0x00000000      /* IGNORE */

/* Extended channel modes will eventually go here. */
#define CMODE_NOCOLOR	0x00001000	/* bahamut +c */
#define CMODE_MODREG	0x00002000	/* bahamut +M */
#define CMODE_REGONLY	0x00004000	/* bahamut +R */
#define CMODE_OPERONLY  0x00008000      /* bahamut +O */
#define CMODE_ADMONLY	0x00010000	/* unreal +A */
#define CMODE_PEACE	0x00020000	/* unreal +Q */
#define CMODE_STRIP	0x00040000	/* unreal +S */
#define CMODE_NOKNOCK	0x00080000	/* unreal +K */
#define CMODE_NOINVITE	0x00100000	/* unreal +V */
#define CMODE_NOCTCP	0x00200000	/* unreal +C */
#define CMODE_HIDING	0x00400000	/* unreal +u */
#define CMODE_SSLONLY	0x00800000	/* unreal +z */
#define CMODE_STICKY	0x01000000	/* unreal +N */

#endif
