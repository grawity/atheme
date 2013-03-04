/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 *
 * $Id: set.c 324 2005-06-04 19:01:54Z nenolod $
 */

#include "atheme.h"

static struct set_command_ *ns_set_cmd_find(char *origin, char *command);

/* SET <setting> <parameters> */
void ns_set(char *origin)
{
	char *name = strtok(NULL, " ");
	char *setting = strtok(NULL, " ");
	char *params = strtok(NULL, "");
	struct set_command_ *c;

	if (!name || !setting || !params)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specificed for \2SET\2.");
		notice(nicksvs.nick, origin, "Syntax: SET <setting> <parameters>");
		return;
	}

	/* take the command through the hash table */
	if ((c = ns_set_cmd_find(origin, setting)))
	{
		if (c->func)
			c->func(origin, name, params);
		else
			notice(nicksvs.nick, origin, "Invalid setting.  Please use \2HELP\2 for help.");
	}
}

static void ns_set_email(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (mu->temp)
	{
		if (mu->key == atoi(params))
		{
			free(mu->email);
			mu->email = sstrdup(mu->temp);
			free(mu->temp);
			mu->temp = NULL;
			mu->key = 0;

			snoop("SET:EMAIL:VS: \2%s\2 by \2%s\2", mu->email, origin);

			notice(nicksvs.nick, origin, "\2%s\2 has now been verified.", mu->email);

			return;
		}

		snoop("SET:EMAIL:VF: \2%s\2 by \2%s\2", mu->temp, origin);

		notice(nicksvs.nick, origin, "Verification failed. Invalid key for \2%s\2.", mu->temp);

		return;
	}

	if (!validemail(params))
	{
		notice(nicksvs.nick, origin, "\2%s\2 is not a valid email address.", params);
		return;
	}

	if (!strcasecmp(mu->email, params))
	{
		notice(nicksvs.nick, origin, "The email address for \2%s\2 is already set to \2%s\2.", mu->name, mu->email);
		return;
	}

	snoop("SET:EMAIL: \2%s\2 (\2%s\2 -> \2%s\2)", mu->name, mu->email, params);

	if (me.auth == AUTH_EMAIL)
	{
		if (mu->temp)
			free(mu->temp);

		mu->temp = sstrdup(params);
		mu->key = makekey();

		notice(nicksvs.nick, origin, "An email containing email changing instructions " "has been sent to \2%s\2.", mu->temp);
		notice(nicksvs.nick, origin, "Your email address will not be changed until you follow " "these instructions.");

		sendemail(mu->name, itoa(mu->key), 3);

		return;
	}

	free(mu->email);
	mu->email = sstrdup(params);

	notice(nicksvs.nick, origin, "The email address for \2%s\2 has been changed to \2%s\2.", mu->name, mu->email);
}

static void ns_set_hidemail(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}


	if (!strcasecmp("ON", params))
	{
		if (MU_HIDEMAIL & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HIDEMAIL:ON: for \2%s\2", mu->name);

		mu->flags |= MU_HIDEMAIL;

		notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HIDEMAIL & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HIDEMAIL:OFF: for \2%s\2", mu->name);

		mu->flags &= ~MU_HIDEMAIL;

		notice(nicksvs.nick, origin, "The \2HIDEMAIL\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2HIDEMAIL\2.");
		return;
	}
}

static void ns_set_hold(char *origin, char *name, char *params)
{
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_HOLD & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2HOLD\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HOLD:ON: for \2%s\2 by \2%s\2", mu->name, origin);

		mu->flags |= MU_HOLD;

		notice(nicksvs.nick, origin, "The \2HOLD\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_HOLD & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2HOLD\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:HOLD:OFF: for \2%s\2 by \2%s\2", mu->name, origin);

		mu->flags &= ~MU_HOLD;

		notice(nicksvs.nick, origin, "The \2HOLD\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2HOLD\2.");
		return;
	}
}

static void ns_set_neverop(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	if (!strcasecmp("ON", params))
	{
		if (MU_NEVEROP & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NEVEROP:ON: for \2%s\2 by \2%s\2", mu->name, origin);

		mu->flags |= MU_NEVEROP;

		notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NEVEROP & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NEVEROP:OFF: for \2%s\2 by \2%s\2", mu->name, origin);

		mu->flags &= ~MU_NEVEROP;

		notice(nicksvs.nick, origin, "The \2NEVEROP\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2NEVEROP\2.");
		return;
	}
}

static void ns_set_noop(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	if (mu != u->myuser)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}


	if (!strcasecmp("ON", params))
	{
		if (MU_NOOP & mu->flags)
		{
			notice(nicksvs.nick, origin, "The \2NOOP\2 flag is already set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NOOP:ON: for \2%s\2", mu->name);

		mu->flags |= MU_NOOP;

		notice(nicksvs.nick, origin, "The \2NOOP\2 flag has been set for \2%s\2.", mu->name);

		return;
	}

	else if (!strcasecmp("OFF", params))
	{
		if (!(MU_NOOP & mu->flags))
		{
			notice(nicksvs.nick, origin, "The \2NOOP\2 flag is not set for \2%s\2.", mu->name);
			return;
		}

		snoop("SET:NOOP:OFF: for \2%s\2", mu->name);

		mu->flags &= ~MU_NOOP;

		notice(nicksvs.nick, origin, "The \2NOOP\2 flag has been removed for \2%s\2.", mu->name);

		return;
	}

	else
	{
		notice(nicksvs.nick, origin, "Invalid parameters specified for \2NOOP\2.");
		return;
	}
}

static void ns_set_password(char *origin, char *name, char *params)
{
	user_t *u = user_find(origin);
	myuser_t *mu;

	if (!(mu = myuser_find(name)))
	{
		notice(nicksvs.nick, origin, "No such username: \2%s\2.", name);
		return;
	}

	if (u->myuser != mu)
	{
		notice(nicksvs.nick, origin, "You are not authorized to perform this command.");
		return;
	}

	snoop("SET:PASSWORD: \2%s\2 as \2%s\2 for \2%s\2", u->nick, mu->name, mu->name);

	free(mu->pass);
	mu->pass = sstrdup(params);

	notice(nicksvs.nick, origin, "The password for \2%s\2 has been changed to \2%s\2. " "Please write this down for future reference.", mu->name, mu->pass);

	return;
}

/* *INDENT-OFF* */

/* commands we understand */
static struct set_command_ ns_set_commands[] = {
  { "EMAIL",      AC_NONE,  ns_set_email      },
  { "HIDEMAIL",   AC_NONE,  ns_set_hidemail   },
  { "HOLD",       AC_SRA,   ns_set_hold       },
  { "NEVEROP",    AC_NONE,  ns_set_neverop    },
  { "NOOP",       AC_NONE,  ns_set_noop       },
  { "PASSWORD",   AC_NONE,  ns_set_password   },
  { NULL }
};

/* *INDENT-ON* */

static struct set_command_ *ns_set_cmd_find(char *origin, char *command)
{
	user_t *u = user_find(origin);
	struct set_command_ *c;

	for (c = ns_set_commands; c->name; c++)
	{
		if (!strcasecmp(command, c->name))
		{
			/* no special access required, so go ahead... */
			if (c->access == AC_NONE)
				return c;

			/* sra? */
			if ((c->access == AC_SRA) && (is_sra(u->myuser)))
				return c;

			/* ircop? */
			if ((c->access == AC_IRCOP) && (is_ircop(u)))
				return c;

			/* otherwise... */
			else
			{
				notice(nicksvs.nick, origin, "You are not authorized to perform this operation.");
				return NULL;
			}
		}
	}

	/* it's a command we don't understand */
	notice(nicksvs.nick, origin, "Invalid command. Please use \2HELP\2 for help.");
	return NULL;
}
