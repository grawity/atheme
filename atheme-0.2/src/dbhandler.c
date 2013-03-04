/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Symbols which are relocated at runtime relating to database activities.
 *
 * $Id: dbhandler.c 908 2005-07-17 04:00:28Z w00t $
 */

#include "atheme.h"

void (*db_save)(void *arg) = NULL;
void (*db_load)(void) = NULL;

myuser_t *(*myuser_add)(char *name, char *pass, char *email) = NULL;
void (*myuser_delete)(char *name) = NULL;
myuser_t *(*myuser_find)(char *name) = NULL;

mychan_t *(*mychan_add)(char *name, char *pass) = NULL;
void (*mychan_delete)(char *name) = NULL;
mychan_t *(*mychan_find)(char *name) = NULL;

chanacs_t *(*chanacs_add)(mychan_t *mychan, myuser_t *myuser, uint32_t level) = NULL;
chanacs_t *(*chanacs_add_host)(mychan_t *mychan, char *host, uint32_t level) = NULL;
void (*chanacs_delete)(mychan_t *mychan, myuser_t *myuser, uint32_t level) = NULL;
void (*chanacs_delete_host)(mychan_t *mychan, char *host, uint32_t level) = NULL;
chanacs_t *(*chanacs_find)(mychan_t *mychan, myuser_t *myuser, uint32_t level) = NULL;
chanacs_t *(*chanacs_find_host)(mychan_t *mychan, char *host, uint32_t level) = NULL;
chanacs_t *(*chanacs_find_host_literal)(mychan_t *mychan, char *host, uint32_t level) = NULL;

kline_t *(*kline_add)(char *user, char *host, char *reason, long duration) = NULL;
void (*kline_delete)(char *user, char *host) = NULL;
kline_t *(*kline_find)(char *user, char *host) = NULL;
kline_t *(*kline_find_num)(uint32_t number) = NULL;
void (*kline_expire)(void *arg) = NULL;

metadata_t *(*metadata_add)(void *target, int32_t type, char *name, char *value) = NULL;
void (*metadata_delete)(void *target, int32_t type, char *name) = NULL;
metadata_t *(*metadata_find)(void *target, int32_t type, char *name) = NULL;

void (*expire_check)(void *arg) = NULL;
