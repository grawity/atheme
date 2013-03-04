/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains misc routines.
 *
 * $Id: function.c 163 2005-05-29 07:23:53Z nenolod $
 */

#include "atheme.h"

FILE *log_file;

#if !HAVE_STRLCAT
/* These functions are taken from Linux. */
size_t strlcat(char *dest, const char *src, size_t count)
{
	size_t dsize = strlen(dest);
	size_t len = strlen(src);
	size_t res = dsize + len;

	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count - 1;
	memcpy(dest, src, len);
	dest[len] = 0;
	return res;
}
#endif

#if !HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size)
	{
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}
#endif

#if HAVE_GETTIMEOFDAY
/* starts a timer */
void s_time(struct timeval *sttime)
{
	gettimeofday(sttime, NULL);
}
#endif

#if HAVE_GETTIMEOFDAY
/* ends a timer */
void e_time(struct timeval sttime, struct timeval *ttime)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	timersub(&now, &sttime, ttime);
}
#endif

#if HAVE_GETTIMEOFDAY
/* translates microseconds into miliseconds */
int32_t tv2ms(struct timeval *tv)
{
	return (tv->tv_sec * 1000) + (int32_t) (tv->tv_usec / 1000);
}
#endif

/* replaces tabs with a single ASCII 32 */
void tb2sp(char *line)
{
	char *c;

	while ((c = strchr(line, '\t')))
		*c = ' ';
}

/* copy at most len-1 characters from a string to a buffer, NULL terminate */
char *strscpy(char *d, const char *s, size_t len)
{
	char *d_orig = d;

	if (!len)
		return d;
	while (--len && (*d++ = *s++));
	*d = 0;
	return d_orig;
}

/* does malloc()'s job and dies if malloc() fails */
void *smalloc(long size)
{
	void *buf;

	buf = malloc(size);
	if (!buf)
		raise(SIGUSR1);
	return buf;
}

/* does calloc()'s job and dies if calloc() fails */
void *scalloc(long elsize, long els)
{
	void *buf = calloc(elsize, els);

	if (!buf)
		raise(SIGUSR1);
	return buf;
}

/* does realloc()'s job and dies if realloc() fails */
void *srealloc(void *oldptr, long newsize)
{
	void *buf = realloc(oldptr, newsize);

	if (!buf)
		raise(SIGUSR1);
	return buf;
}

/* does strdup()'s job, only with the above memory functions */
char *sstrdup(const char *s)
{
	char *t = smalloc(strlen(s) + 1);

	strcpy(t, s);
	return t;
}

/* removes unwanted chars from a line */
void strip(char *line)
{
	char *c;

	if (line)
	{
		if ((c = strchr(line, '\n')))
			*c = '\0';
		if ((c = strchr(line, '\r')))
			*c = '\0';
		if ((c = strchr(line, '\1')))
			*c = '\0';
	}
}

/* opens shrike.log */
void log_open(void)
{
	if (log_file)
		return;

	if (!(log_file = fopen("var/shrike.log", "a")))
		exit(EXIT_FAILURE);
}

/* logs something to shrike.log */
void slog(uint32_t level, const char *fmt, ...)
{
	va_list args;
	time_t t;
	struct tm tm;
	char buf[32];

	if (level > me.loglevel)
		return;

	va_start(args, fmt);

	time(&t);
	tm = *localtime(&t);
	strftime(buf, sizeof(buf) - 1, "[%d/%m %H:%M:%S] ", &tm);

	if (!log_file)
		log_open();

	if (log_file)
	{
		fputs(buf, log_file);

		vfprintf(log_file, fmt, args);

		fputc('\n', log_file);

		fflush(log_file);
	}

	if ((runflags & (RF_LIVE | RF_STARTING)))
	{
		vfprintf(stderr, fmt, args);

		fputc('\n', stderr);
	}
}

/* return the current time in milliseconds */
uint32_t time_msec(void)
{
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#else
	return CURRTIME * 1000;
#endif
}

/* performs a regex match */
uint8_t regex_match(regex_t * preg, char *pattern, char *string, size_t nmatch, regmatch_t pmatch[], int eflags)
{
	static char errmsg[256];
	int errnum;

	/* compile regex */
	preg = (regex_t *) malloc(sizeof(*preg));

	errnum = regcomp(preg, pattern, REG_ICASE | REG_EXTENDED);
	if (errnum != 0)
	{
		regerror(errnum, preg, errmsg, 256);
		slog(LG_ERROR, "regex_match(): %s\n", errmsg);
		free(preg);
		preg = NULL;
		return 1;
	}

	/* match it */
	if (regexec(preg, string, 5, pmatch, 0) != 0)
		return 1;

	else
		return 0;
}

/* generates a hash value */
uint32_t shash(const unsigned char *text)
{
	unsigned long h = 0, g;

	while (*text)
	{
		h = (h << 4) + tolower(*text++);

		if ((g = (h & 0xF0000000)))
			h ^= g >> 24;

		h &= ~g;
	}

	return (h % HASHSIZE);
}

/* replace all occurances of 'old' with 'new' */
char *replace(char *s, int32_t size, const char *old, const char *new)
{
	char *ptr = s;
	int32_t left = strlen(s);
	int32_t avail = size - (left + 1);
	int32_t oldlen = strlen(old);
	int32_t newlen = strlen(new);
	int32_t diff = newlen - oldlen;

	while (left >= oldlen)
	{
		if (strncmp(ptr, old, oldlen))
		{
			left--;
			ptr++;
			continue;
		}

		if (diff > avail)
			break;

		if (diff != 0)
			memmove(ptr + oldlen + diff, ptr + oldlen, left + 1);

		strncpy(ptr, new, newlen);
		ptr += newlen;
		left -= oldlen;
	}

	return s;
}

/* reverse of atoi() */
char *itoa(int num)
{
	static char ret[32];
	sprintf(ret, "%d", num);
	return ret;
}

/* channel modes we support */
struct flag
{
	char mode;
	int32_t flag;
	char prefix;
};

/* convert mode flags to a text mode string */
char *flags_to_string(int32_t flags)
{
	static char buf[32];
	char *s = buf;
	int i;

	for (i = 0; mode_list[i].mode != 0; i++)
		if (flags & mode_list[i].value)
			*s++ = mode_list[i].mode;

	*s = 0;

	return buf;
}

/* convert a mode character to a flag. */
int32_t mode_to_flag(char c)
{
	int i;

	for (i = 0; mode_list[i].mode != 0 && mode_list[i].mode != c; i++);

	return mode_list[i].value;
}

/* return the time elapsed since an event */
char *time_ago(time_t event)
{
	static char ret[128];
	int years, weeks, days, hours, minutes, seconds;

	event = CURRTIME - event;
	years = weeks = days = hours = minutes = seconds = 0;

	while (event >= 60 * 60 * 24 * 365)
	{
		event -= 60 * 60 * 24 * 365;
		years++;
	}
	while (event >= 60 * 60 * 24 * 7)
	{
		event -= 60 * 60 * 24 * 7;
		weeks++;
	}
	while (event >= 60 * 60 * 24)
	{
		event -= 60 * 60 * 24;
		days++;
	}
	while (event >= 60 * 60)
	{
		event -= 60 * 60;
		hours++;
	}
	while (event >= 60)
	{
		event -= 60;
		minutes++;
	}

	seconds = event;

	if (years)
		snprintf(ret, sizeof(ret),
			 "%d year%s, %d week%s, %d day%s, %02d:%02d:%02d", years, years == 1 ? "" : "s", weeks, weeks == 1 ? "" : "s", days, days == 1 ? "" : "s", hours, minutes, seconds);
	else if (weeks)
		snprintf(ret, sizeof(ret), "%d week%s, %d day%s, %02d:%02d:%02d", weeks, weeks == 1 ? "" : "s", days, days == 1 ? "" : "s", hours, minutes, seconds);
	else if (days)
		snprintf(ret, sizeof(ret), "%d day%s, %02d:%02d:%02d", days, days == 1 ? "" : "s", hours, minutes, seconds);
	else if (hours)
		snprintf(ret, sizeof(ret), "%d hour%s, %d minute%s, %d second%s", hours, hours == 1 ? "" : "s", minutes, minutes == 1 ? "" : "s", seconds, seconds == 1 ? "" : "s");
	else if (minutes)
		snprintf(ret, sizeof(ret), "%d minute%s, %d second%s", minutes, minutes == 1 ? "" : "s", seconds, seconds == 1 ? "" : "s");
	else
		snprintf(ret, sizeof(ret), "%d second%s", seconds, seconds == 1 ? "" : "s");

	return ret;
}

char *timediff(time_t seconds)
{
	static char buf[BUFSIZE];
	int days, hours, minutes;

	days = (int)(seconds / 86400);
	seconds %= 86400;
	hours = (int)(seconds / 3600);
	hours %= 3600;
	minutes = (int)(seconds / 60);
	minutes %= 60;
	seconds %= 60;

	snprintf(buf, sizeof(buf), "%d day%s, %d:%02d:%02lu", days, (days == 1) ? "" : "s", hours, minutes, (unsigned long)seconds);

	return buf;
}

/* generate a random number, for use as a key */
unsigned long makekey(void)
{
	unsigned long i, j, k;

	i = rand() % (CURRTIME / cnt.user + 1);
	j = rand() % (me.start * cnt.chan + 1);

	if (i > j)
		k = (i - j) + strlen(chansvs.user);
	else
		k = (j - i) + strlen(chansvs.host);

	/* shorten or pad it to 9 digits */
	if (k > 1000000000)
		k = k - 1000000000;
	if (k < 100000000)
		k = k + 100000000;

	return k;
}

int validemail(char *email)
{
	int i, valid = 1, chars = 0;

	/* make sure it has @ and . */
	if (!strchr(email, '@') || !strchr(email, '.'))
		valid = 0;

	/* check for other bad things */
	if (strchr(email, '$') || strchr(email, '/') || strchr(email, ';') || strchr(email, '<') || strchr(email, '>') || strchr(email, '&'))
		valid = 0;

	/* make sure there are at least 6 characters besides the above
	 * mentioned @ and .
	 */
	for (i = strlen(email) - 1; i > 0; i--)
		if (!(email[i] == '@' || email[i] == '.'))
			chars++;

	if (chars < 6)
		valid = 0;

	return valid;
}

boolean_t validhostmask(char *host)
{
	/* make sure it has ! and @ */
	if (!strchr(host, '!') || !strchr(host, '@'))
		return FALSE;

	return TRUE;
}

/* send the specified type of email.
 *
 * what is what we're sending to, a username.
 *
 * param is an extra parameter; email, key, etc.
 *
 * If type ==:
 *   1 - username REGISTER
 *   2 - username SENDPASS
 *   3 - username SET:EMAIL
 *   4 - channel  SENDPASS
 */
void sendemail(char *what, const char *param, int type)
{
	myuser_t *mu;
	mychan_t *mc;
	char *email, *date, *pass = NULL;
	char cmdbuf[512], timebuf[256], to[128], from[128], subject[128];
	FILE *out;
	time_t t;
	struct tm tm;

	if (type == 1 || type == 2 || type == 3)
	{
		if (!(mu = myuser_find(what)))
			return;

		if (type == 1 || type == 3)
			pass = itoa(mu->key);
		else
			pass = mu->pass;

		if (type == 3)
			email = mu->temp;
		else
			email = mu->email;
	}
	else
	{
		if (!(mc = mychan_find(what)))
			return;

		mu = mc->founder;

		pass = mc->pass;

		email = mu->email;
	}

	/* set up the email headers */
	time(&t);
	tm = *gmtime(&t);
	strftime(timebuf, sizeof(timebuf) - 1, "%a, %d %b %Y %H:%M:%S", &tm);

	date = timebuf;

	sprintf(from, "%s <%s@%s>", chansvs.nick, chansvs.user, chansvs.host);
	sprintf(to, "%s <%s>", mu->name, email);

	if (type == 1)
		strlcpy(subject, "Username Registration", 128);
	else if (type == 2 || type == 4)
		strlcpy(subject, "Password Retrieval", 128);
	else if (type == 3)
		strlcpy(subject, "Change Email Confirmation", 128);

	/* now set up the email */
	sprintf(cmdbuf, "%s %s", me.mta, email);
	out = popen(cmdbuf, "w");

	fprintf(out, "From: %s\n", from);
	fprintf(out, "To: %s\n", to);
	fprintf(out, "Subject: %s\n", subject);
	fprintf(out, "Date: %s\n\n", date);

	fprintf(out, "%s,\n\n", mu->name);
	fprintf(out, "Thank you for your interest in the %s IRC network.\n\n", me.netname);

	if (type == 1)
	{
		fprintf(out, "In order to complete your registration, you must send " "the following command on IRC:\n");
		fprintf(out, "/MSG %s REGISTER %s KEY %s\n\n", chansvs.nick, what, pass);
		fprintf(out, "Thank you for registering your username on the %s IRC " "network!\n", me.netname);
	}
	else if (type == 2 || type == 4)
	{
		fprintf(out, "Someone has requested the password for %s be sent to the " "corresponding email address. If you did not request this action " "please let us know.\n\n", what);
		fprintf(out, "The password for %s is %s. Please write this down for " "future reference.\n", what, pass);
	}
	else if (type == 3)
	{
		fprintf(out, "In order to complete your email change, you must send " "the following command on IRC:\n");
		fprintf(out, "/MSG %s SET %s EMAIL %s\n\n", chansvs.nick, what, pass);
	}

	fprintf(out, ".\n");
	pclose(out);
}

/* various access level checkers */
boolean_t is_founder(mychan_t *mychan, myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (mychan->founder == myuser)
		return TRUE;

	if ((chanacs_find(mychan, myuser, CA_FOUNDER)))
		return TRUE;

	return FALSE;
}

boolean_t is_successor(mychan_t *mychan, myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (mychan->successor == myuser)
		return TRUE;

	if ((chanacs_find(mychan, myuser, CA_SUCCESSOR)))
		return TRUE;

	return FALSE;
}

boolean_t is_xop(mychan_t *mychan, myuser_t *myuser, uint32_t level)
{
	chanacs_t *ca;

	if (!myuser)
		return FALSE;

	if ((ca = chanacs_find(mychan, myuser, level)))
		return TRUE;

	return FALSE;
}

boolean_t is_on_mychan(mychan_t *mychan, myuser_t *myuser)
{
	chanuser_t *cu;

	if ((cu = chanuser_find(mychan->chan, myuser->user)))
		return TRUE;

	return FALSE;
}

boolean_t should_op(mychan_t *mychan, myuser_t *myuser)
{
	chanuser_t *cu;

	if (MC_NEVEROP & mychan->flags)
		return FALSE;

	if (MU_NEVEROP & myuser->flags)
		return FALSE;

	cu = chanuser_find(mychan->chan, myuser->user);
	if (!cu)
		return FALSE;

	if (CMODE_OP & cu->modes)
		return FALSE;

	if (is_xop(mychan, myuser, (CA_AUTOOP)))
		return TRUE;

	return FALSE;
}

boolean_t should_op_host(mychan_t *mychan, char *host)
{
	chanacs_t *ca;
	char hostbuf[BUFSIZE];

	hostbuf[0] = '\0';

	strlcat(hostbuf, chansvs.nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, chansvs.user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, chansvs.host, BUFSIZE);

	if (!match(host, hostbuf))
		return FALSE;

	if ((ca = chanacs_find_host(mychan, host, (CA_AUTOOP))))
		return TRUE;

	return FALSE;
}

boolean_t should_kick(mychan_t *mychan, myuser_t *myuser)
{
	chanuser_t *cu;

	cu = chanuser_find(mychan->chan, myuser->user);
	if (!cu)
		return FALSE;

	if (is_xop(mychan, myuser, (CA_AKICK)))
		return TRUE;

	return FALSE;
}

boolean_t should_kick_host(mychan_t *mychan, char *host)
{
	chanacs_t *ca;
	char hostbuf[BUFSIZE];

	hostbuf[0] = '\0';

	strlcat(hostbuf, chansvs.nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, chansvs.user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, chansvs.host, BUFSIZE);

	if (!match(host, hostbuf))
		return FALSE;

	if ((ca = chanacs_find_host(mychan, host, (CA_AKICK))))
		return TRUE;

	return FALSE;
}

boolean_t should_voice(mychan_t *mychan, myuser_t *myuser)
{
	chanuser_t *cu;

	if (MC_NEVEROP & mychan->flags)
		return FALSE;

	if (MU_NEVEROP & myuser->flags)
		return FALSE;

	cu = chanuser_find(mychan->chan, myuser->user);
	if (!cu)
		return FALSE;

	if (CMODE_VOICE & cu->modes)
		return FALSE;

	if (is_xop(mychan, myuser, CA_AUTOVOICE))
		return TRUE;

	return FALSE;
}

boolean_t should_voice_host(mychan_t *mychan, char *host)
{
	chanacs_t *ca;
	char hostbuf[BUFSIZE];

	hostbuf[0] = '\0';

	strlcat(hostbuf, chansvs.nick, BUFSIZE);
	strlcat(hostbuf, "!", BUFSIZE);
	strlcat(hostbuf, chansvs.user, BUFSIZE);
	strlcat(hostbuf, "@", BUFSIZE);
	strlcat(hostbuf, chansvs.host, BUFSIZE);

	if (!match(host, hostbuf))
		return FALSE;

	if ((ca = chanacs_find_host(mychan, host, CA_AUTOVOICE)))
		return TRUE;

	return FALSE;
}

boolean_t is_sra(myuser_t *myuser)
{
	if (!myuser)
		return FALSE;

	if (myuser->sra)
		return TRUE;

	return FALSE;
}

boolean_t is_ircop(user_t *user)
{
	if (UF_IRCOP & user->flags)
		return TRUE;

	return FALSE;
}

boolean_t is_admin(user_t *user)
{
	if (UF_ADMIN & user->flags)
		return TRUE;

	return FALSE;
}

/* stolen from Sentinel */
int token_to_value(struct Token token_table[], char *token)
{
	int i;

	if ((token_table != NULL) && (token != NULL))
	{
		for (i = 0; token_table[i].text != NULL; i++)
		{
			if (strcasecmp(token_table[i].text, token) == 0)
			{
				return token_table[i].value;
			}
		}
		/* If no match... */
		return TOKEN_UNMATCHED;
	}

	/* Otherwise... */
	return TOKEN_ERROR;
}

char *sbytes(float x)
{
	if (x > 1073741824.0)
		return "GB";

	else if (x > 1048576.0)
		return "MB";

	else if (x > 1024.0)
		return "KB";

	return "B";
}

float bytes(float x)
{
	if (x > 1073741824.0)
		return (x / 1073741824.0);

	if (x > 1048576.0)
		return (x / 1048576.0);

	if (x > 1024.0)
		return (x / 1024.0);

	return x;
}
