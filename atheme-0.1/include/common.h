/*
 * Copyright (C) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Data structures for account information.
 *
 * $Id: common.h 162 2005-05-29 07:15:46Z nenolod $
 */

#ifndef COMMON_H
#define COMMON_H

/* D E F I N E S */
#define BUFSIZE  1024            /* maximum size of a buffer */
#define HASHSIZE 1021            /* hash table size          */
#define MODESTACK_WAIT 500
#define MAXMODES 4
#define MAXPARAMSLEN (510-32-64-34-(7+MAXMODES))
#define MAX_EVENTS 25
#define TOKEN_UNMATCHED -1
#define TOKEN_ERROR -2

#undef DEBUG_BALLOC

#ifdef DEBUG_BALLOC
#define BALLOC_MAGIC 0x3d3a3c3d
#endif

#ifdef LARGE_NETWORK
#define HEAP_CHANNEL    1024
#define HEAP_CHANUSER   1024
#define HEAP_USER       1024
#define HEAP_SERVER     16
#define HEAP_NODE       1024  
#define HEAP_CHANACS    1024
#else
#define HEAP_CHANNEL    64
#define HEAP_CHANUSER   128
#define HEAP_USER       128
#define HEAP_SERVER     8
#define HEAP_NODE       128
#define HEAP_CHANACS    128
#endif

#define CACHEFILE_HEAP_SIZE     32
#define CACHELINE_HEAP_SIZE     64

#ifndef uint8_t
#define uint8_t u_int8_t
#define uint16_t u_int16_t
#define uint32_t u_int32_t
#define uint64_t u_int64_t
#endif

#endif
