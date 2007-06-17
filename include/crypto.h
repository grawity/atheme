/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented under doc/LICENSE.
 *
 * Cryptographic module support.
 *
 * $Id: crypto.h 7779 2007-03-03 13:55:42Z pippijn $
 */

#ifndef CRYPTO_H
#define CRYPTO_H

E char *(*crypt_string)(char *str, char *salt);
E char *generic_crypt_string(char *str, char *salt);
E boolean_t crypt_verify_password(char *uinput, char *pass);
E char *gen_salt(void);
E boolean_t crypto_module_loaded;

#endif

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
