/*
 * Copyright (c) 2005 William Pitcock et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the header which includes all of the system stuff.
 *
 * $Id: stdinc.h 448 2005-06-10 08:05:10Z nenolod $
 */

#ifndef STDINC_H
#define STDINC_H

/* I N C L U D E S */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <ctype.h>
#include <regex.h>
#include <fcntl.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

/* socket stuff */
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/socket.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <libgen.h>
#include <dirent.h>

typedef enum { ERROR = -1, FALSE, TRUE } l_boolean_t;
#undef boolean_t
#define boolean_t l_boolean_t

#endif
