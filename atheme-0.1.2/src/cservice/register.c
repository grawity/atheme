/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService REGISTER function.
 *
 * $Id: register.c 337 2005-06-05 01:37:28Z nenolod $
 */

#include "atheme.h"

void cs_register(char *origin)
{
	user_t *u = user_find(origin);
	channel_t *c;
	chanuser_t *cu;
	myuser_t *mu, *tmu;
	mychan_t *mc, *tmc;
	node_t *n;
	char *name = strtok(NULL, " ");
	char *pass = strtok(NULL, " ");
	uint32_t i, tcnt;

	if (!name)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2REGISTER\2.");
		notice(chansvs.nick, origin, "Syntax:");
		if (nicksvs.enable == FALSE)
			notice(chansvs.nick, origin, "To register an account: REGISTER <username> <password> <email>");
		notice(chansvs.nick, origin, "To register a channel : REGISTER <#channel> <password>");
		return;
	}

	if (*name == '#')
	{
		if (!name || !pass)
		{
			notice(chansvs.nick, origin, "Insufficient parameters specified for \2REGISTER\2.");
			notice(chansvs.nick, origin, "Syntax: REGISTER <#channel> <password>");
			return;
		}

		if (strlen(pass) > 32)
		{
			notice(chansvs.nick, origin, "Invalid parameters specified for \2REGISTER\2.");
			return;
		}

		/* make sure they're logged in */
		if (!u->myuser || !u->myuser->identified)
		{
			notice(chansvs.nick, origin, "You are not logged in.");
			return;
		}

		/* make sure it isn't already registered */
		if ((mc = mychan_find(name)))
		{
			notice(chansvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mc->name, mc->founder->name);
			return;
		}

		/* make sure the channel exists */
		if (!(c = channel_find(name)))
		{
			notice(chansvs.nick, origin, "The channel \2%s\2 must exist in order to register it.", name);
			return;
		}

		/* make sure they're in it */
		if (!(cu = chanuser_find(c, u)))
		{
			notice(chansvs.nick, origin, "You must be in \2%s\2 in order to register it.", name);
			return;
		}

		/* make sure they're opped */
		if (!(CMODE_OP & cu->modes))
		{
			notice(chansvs.nick, origin, "You must be a channel operator in \2%s\2 in order to " "register it.", name);
			return;
		}

		/* make sure they're within limits */
		for (i = 0, tcnt = 0; i < HASHSIZE; i++)
		{
			LIST_FOREACH(n, mclist[i].head)
			{
				tmc = (mychan_t *)n->data;

				if (is_founder(tmc, u->myuser))
					tcnt++;
			}
		}
		if ((tcnt >= me.maxchans) && (!is_sra(u->myuser)))
		{
			notice(chansvs.nick, origin, "You have too many channels registered.");
			return;
		}

		snoop("REGISTER: \2%s\2 to \2%s\2 as \2%s\2", name, u->nick, u->myuser->name);

		mc = mychan_add(name, pass);
		mc->founder = u->myuser;
		mc->registered = CURRTIME;
		mc->used = CURRTIME;
		mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
		mc->mlock_off |= (CMODE_INVITE | CMODE_LIMIT | CMODE_KEY);
		mc->flags |= config_options.defcflags;

		chanacs_add(mc, u->myuser, CA_FOUNDER);

		notice(chansvs.nick, origin, "\2%s\2 is now registered to \2%s\2.", mc->name, mc->founder->name);

		notice(chansvs.nick, origin, "The password is \2%s\2. Please write this down for " "future reference.", mc->pass);

		if (config_options.join_chans)
			join(mc->name, chansvs.nick);
	}
	else if (nicksvs.enable != TRUE)
	{
		char *email = strtok(NULL, " ");

		if (!name || !pass || !email)
		{
			notice(chansvs.nick, origin, "Insufficient parameters specified for \2REGISTER\2.");
			notice(chansvs.nick, origin, "Syntax: REGISTER <username> <password> <email>");
			return;
		}

		if (!strcasecmp("KEY", pass))
		{
			if (!(mu = myuser_find(name)))
			{
				notice(chansvs.nick, origin, "No such username: \2%s\2.", name);
				return;
			}

			if (!(mu->flags & MU_WAITAUTH))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not awaiting authorization.", name);
				return;
			}

			if (mu->key == atoi(email))
			{
				mu->key = 0;
				mu->flags &= ~MU_WAITAUTH;

				snoop("REGISTER:VS: \2%s\2 by \2%s\2", mu->email, origin);

				notice(chansvs.nick, origin, "\2%s\2 has now been verified.", mu->email);

				return;
			}

			snoop("REGISTER:VF: \2%s\2 by \2%s\2", mu->email, origin);

			notice(chansvs.nick, origin, "Verification failed. Invalid key for \2%s\2.", mu->email);

			return;
		}

		if ((strlen(name) > 32) || (strlen(pass) > 32) || (strlen(email) > 256))
		{
			notice(chansvs.nick, origin, "Invalid parameters specified for \2REGISTER\2.");
			return;
		}

		if (!validemail(email))
		{
			notice(chansvs.nick, origin, "\2%s\2 is not a valid email address.", email);
			return;
		}

		/* make sure it isn't registered already */
		mu = myuser_find(name);
		if (mu != NULL)
		{
			notice(chansvs.nick, origin, "\2%s\2 is already registered to \2%s\2.", mu->name, mu->email);
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
			notice(chansvs.nick, origin, "\2%s\2 has too many usernames registered.", email);
			return;
		}

		snoop("REGISTER: \2%s\2 to \2%s\2", name, email);

		mu = myuser_add(name, pass, email);
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

			notice(chansvs.nick, origin, "An email containing username activiation instructions " "has been sent to \2%s\2.", mu->email);
			notice(chansvs.nick, origin, "If you do not complete registration within one day your " "username will expire.");

			sendemail(mu->name, itoa(mu->key), 1);
		}

		notice(chansvs.nick, origin, "\2%s\2 is now registered to \2%s\2.", mu->name, mu->email);
		notice(chansvs.nick, origin, "The password is \2%s\2. Please write this down for future reference.", mu->pass);
	}
	else
	{
		notice(chansvs.nick, origin, "Account registration is not available. Please register your nickname instead.");
		notice(chansvs.nick, origin, "To do so, use: /msg %s REGISTER <password> <email>.", nicksvs.nick);
	}
}
