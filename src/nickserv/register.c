/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ REGISTER function.
 *
 * $Id: register.c 312 2005-06-02 05:44:10Z nenolod $
 */

#include "atheme.h"

void ns_register(char *origin)
{
	user_t *u = user_find(origin);
	myuser_t *mu, *tmu;
	node_t *n;
	char *pass = strtok(NULL, " ");
	char *email = strtok(NULL, " ");
	uint32_t i, tcnt;

	if (!pass || !email)
	{
		notice(nicksvs.nick, origin, "Insufficient parameters specified for \2REGISTER\2.");
		notice(nicksvs.nick, origin, "Syntax: REGISTER <password> <email>");
		return;
	}

	if (!strcasecmp("KEY", pass))
	{
		if (!(mu = myuser_find(origin)))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not registered.", origin);
			return;
		}

		if (!(mu->flags & MU_WAITAUTH))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not awaiting authorization.", origin);
			return;
		}

		if (mu->key == atoi(email))
		{
			mu->key = 0;
			mu->flags &= ~MU_WAITAUTH;

			snoop("REGISTER:VS: \2%s\2 by \2%s\2", mu->email, origin);

			notice(nicksvs.nick, origin, "\2%s\2 has now been verified.", mu->email);
			notice(nicksvs.nick, origin, "Thank you for verifying your e-mail address! You have taken steps in "
				"ensuring that your registrations are not exploited.");

			return;
		}

		snoop("REGISTER:VF: \2%s\2 by \2%s\2", mu->email, origin);

		notice(nicksvs.nick, origin, "Verification failed. Invalid key for \2%s\2.", mu->email);

		return;
	}
	else
	{
		if ((strlen(pass) > 32) || (strlen(email) > 256))
		{
			notice(nicksvs.nick, origin, "Invalid parameters specified for \2REGISTER\2.");
			return;
		}

		if (!validemail(email))
		{
			notice(nicksvs.nick, origin, "\2%s\2 is not a valid email address.", email);
			return;
		}

		/* make sure it isn't registered already */
		mu = myuser_find(origin);
		if (mu != NULL)
		{
			notice(nicksvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mu->name, mu->email);
			return;
		}

		/* make sure they're within limits */
		for (i = 0, tcnt = 0; i < HASHSIZE; i++)
		{
			LIST_FOREACH(n, mulist[i].head)
			{
				tmu = (myuser_t *)n->data;

				if (!strcasecmp(email, tmu->email))
					tcnt++;
			}
		}
		if (tcnt >= me.maxusers)
		{
			notice(nicksvs.nick, origin, "\2%s\2 has too many nicknames registered.", email);
			return;
		}

		snoop("REGISTER: \2%s\2 to \2%s\2", origin, email);

		mu = myuser_add(origin, pass, email);
		u = user_find(origin);

		if (u->myuser)
			u->myuser->user = NULL;

		u->myuser = mu;
		mu->user = u;
		mu->registered = CURRTIME;
		mu->identified = TRUE;
		mu->lastlogin = CURRTIME;
		mu->flags |= config_options.defuflags;

		if (me.auth == AUTH_EMAIL)
		{
			mu->key = makekey();
			mu->flags |= MU_WAITAUTH;

			notice(nicksvs.nick, origin, "An email containing nickname activiation instructions has been sent to \2%s\2.", mu->email);
			notice(nicksvs.nick, origin, "If you do not complete registration within one day your nickname will expire.");

			sendemail(mu->name, itoa(mu->key), 1);
		}

		notice(nicksvs.nick, origin, "\2%s\2 is now registered to \2%s\2.", mu->name, mu->email);
		notice(nicksvs.nick, origin, "The password is \2%s\2. Please write this down for future reference.", mu->pass);
	}
}
