/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Undernet base64 routine.
 *
 * $Id: ubase64.c 4621 2006-01-20 01:44:46Z jilles $
 */

#include "atheme.h"

/*
 * Rewritten 07/17/05 by nenolod, due to legal concerns
 * of using GPL'ed ircu code in the main tree, raised
 * by Zoot (over at srvx, who have also rewritten their
 * base64 implementation for srvx version 2).
 *
 * Run may be a GPL zealot, that's really ok, because
 * our implementation is faster and more secure...
 */
/*
 * base64touint() written 01/20/06 by jilles, for getting IP addresses.
 */

static const char ub64_alphabet[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]";
#define _ '\377'
static const char ub64_lookuptab[256] = {
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* 0-15 */
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* 16-31 */
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, /* 32-47 */
52, 53, 54, 55, 56, 57, 58, 59, 60, 61, _, _, _, _, _, _, /* 48-63 */
_, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, /* 64-79 */
5, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 62, _, 63, _, _, /* 80-95 */
_, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 96-111 */
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, _, _, _, _, _, /* 112-127 */
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
_, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _ };
#undef _

const char *uinttobase64(char *buf, uint64_t v, int64_t count)
{
	buf[count] = '\0';

	do
	{
		buf[--count] = ub64_alphabet[v & 63];
	}
	while (v >>= 6 && count >= 0);

	return buf;
}

uint32_t base64touint(char *buf)
{
	int bits;
	uint32_t v = 0;
	int count = 6;

	while (--count >= 0 && (bits = ub64_lookuptab[255 & (int)*buf++]) != '\377')
		v = v << 6 | bits;
	return v;
}
