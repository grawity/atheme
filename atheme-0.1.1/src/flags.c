/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * flags.c: Functions to convert a flags table into a bitmask.
 *
 * See doc/LICENSE for licensing information.
 */

#include "atheme.h"

static char flags_buf[128];

struct flags_table chanacs_flags[] = {
	{'v', CA_VOICE},
	{'V', CA_AUTOVOICE},
	{'o', CA_OP},
	{'O', CA_AUTOOP},
	{'t', CA_TOPIC},
	{'s', CA_SET},
	{'r', CA_REMOVE},
	{'i', CA_INVITE},
	{'R', CA_RECOVER},
	{'f', CA_FLAGS},
	{'b', CA_AKICK},
	{0, 0}
};

int flags_to_bitmask(const char *string, struct flags_table table[], int flags)
{
	int bitmask = (flags ? flags : 0x0);
	int status = FLAGS_ADD;
	short i = 0;

	while (*string)
	{
		switch (*string)
		{
		  case '+':
			  status = FLAGS_ADD;
			  break;

		  case '-':
			  status = FLAGS_DEL;
			  break;

		  case '*':
			  if (status == FLAGS_ADD)
			  {
				  bitmask |= 0xFFFFFFFF;

				  /* If this is chanacs_flags[], remove the ban flag. */
				  if (table == chanacs_flags)
				  	bitmask &= ~CA_AKICK;
			  }					
			  else if (status == FLAGS_DEL)
				  bitmask &= ~0xFFFFFFFF;
			  break;

		  default:
			  for (i = 0; table[i].flag; i++)
				  if (table[i].flag == *string)
				  {
					  if (status == FLAGS_ADD)
						  bitmask |= table[i].value;
					  else if (status == FLAGS_DEL)
						  bitmask &= ~table[i].value;
				  }
		}

		*string++;
	}

	return bitmask;
}

char *bitmask_to_flags(int flags, struct flags_table table[])
{
	char *bptr;
	short i = 0;

	memset(flags_buf, 0, sizeof(flags_buf));
	bptr = flags_buf;

	*bptr++ = '+';

	for (i = 0; table[i].flag; i++)
		if (table[i].value & flags)
			*bptr++ = table[i].flag;

	*bptr++ = '\0';

	return flags_buf;
}
