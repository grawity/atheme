/*
 * Copyright (c) 2005 Atheme Development Group
 *
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains IRC interaction routines.
 *
 * $Id: parse.c 1851 2005-08-24 10:17:45Z w00t $
 */

#include "atheme.h"

/* parse the incoming server line, then toss it to its function */
void irc_parse(char *line)
{
	char *origin = NULL;
	char *pos;
	char *command = NULL;
	char *message = NULL;
	char *parv[20];
	static char coreLine[BUFSIZE];
	uint8_t parc = 0;
	uint8_t i;
	pcommand_t *pcmd;

	/* clear the parv */
	for (i = 0; i < 20; i++)
		parv[i] = NULL;

	if (line != NULL)
	{
		/* sometimes we'll get a blank line with just a \n on it...
		 * catch those here... they'll core us later on if we don't
		 */
		if (*line == '\n')
			return;
		if (*line == '\000')
			return;

		/* copy the original line so we know what we crashed on */
		memset((char *)&coreLine, '\0', BUFSIZE);
		strlcpy(coreLine, line, BUFSIZE);

		slog(LG_DEBUG, "-> %s", line);

		/* find the first space */
		if ((pos = strchr(line, ' ')))
		{
			*pos = '\0';
			pos++;
			/* if it starts with a : we have a prefix/origin
			 * pull the origin off into `origin', and have pos for the
			 * command, message will be the part afterwards
			 */
			if (*line == ':' || (ircd->uses_p10 == TRUE && me.recvsvr == TRUE))
			{
				origin = line;

				if (ircd->uses_p10 == FALSE)
					origin++;

				if ((message = strchr(pos, ' ')))
				{
					*message = '\0';
					message++;
					command = pos;
				}
				else
				{
					command = pos;
					message = NULL;
				}
			}
			else
			{
				message = pos;
				command = line;
			}
		}
		/* okay, the nasty part is over, now we need to make a
		 * parv out of what's left
		 */

		if (message)
		{
			if (*message == ':')
			{
				message++;
				parv[0] = message;
				parc = 1;
			}
			else
				parc = tokenize(message, parv);
		}
		else
			parc = 0;

		/* now we should have origin (or NULL), command, and a parv
		 * with it's accompanying parc... let's make ABSOLUTELY sure
		 */
		if (!command)
		{
			slog(LG_DEBUG, "irc_parse(): command not found: %s", coreLine);
			return;
		}

		/* take the command through the hash table */
		if ((pcmd = pcommand_find(command)))
			if (pcmd->handler)
			{
				pcmd->handler(origin, parc, parv);
				return;
			}
	}
}

