/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService XOP functions.
 *
 * $Id: cs_xop.c 987 2005-07-18 09:37:25Z w00t $
 */

/*
 * XXX: This so needs to be reworked, hardcore.
 */

#include "atheme.h"

static void cs_cmd_sop(char *origin);
static void cs_cmd_aop(char *origin);
static void cs_cmd_hop(char *origin);
static void cs_cmd_vop(char *origin);

command_t cs_sop = { "SOP", "Manipulates a channel's SOP list.",
                        AC_NONE, cs_cmd_sop };
command_t cs_aop = { "AOP", "Manipulates a channel's AOP list.",
                        AC_NONE, cs_cmd_aop };
command_t cs_hop = { "HOP", "Manipulates a channel's HOP list.",
			AC_NONE, cs_cmd_hop };
command_t cs_vop = { "VOP", "Manipulates a channel's VOP list.",
                        AC_NONE, cs_cmd_vop };

extern list_t cs_cmdtree;

void _modinit(module_t *m)
{
        command_add(&cs_aop, &cs_cmdtree);
        command_add(&cs_sop, &cs_cmdtree);
	command_add(&cs_hop, &cs_cmdtree);
        command_add(&cs_vop, &cs_cmdtree);
}

void _moddeinit()
{
	command_delete(&cs_aop, &cs_cmdtree);
	command_delete(&cs_sop, &cs_cmdtree);
	command_delete(&cs_hop, &cs_cmdtree);
	command_delete(&cs_vop, &cs_cmdtree);
}

static void cs_xop(char *origin, uint32_t level)
{
	user_t *u = user_find(origin);
	myuser_t *mu;
	mychan_t *mc;
	chanacs_t *ca;
	chanuser_t *cu;
	node_t *n;
	char *chan = strtok(NULL, " ");
	char *cmd = strtok(NULL, " ");
	char *uname = strtok(NULL, " ");
	char hostbuf[BUFSIZE];

	if (!cmd || !chan)
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2xOP\2.");
		notice(chansvs.nick, origin, "Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <username>");
		return;
	}

	if ((strcasecmp("LIST", cmd)) && (!uname))
	{
		notice(chansvs.nick, origin, "Insufficient parameters specified for \2xOP\2.");
		notice(chansvs.nick, origin, "Syntax: SOP|AOP|HOP|VOP <#channel> ADD|DEL|LIST <username>");
		return;
	}

	/* make sure they're registered, logged in
	 * and the founder of the channel before
	 * we go any further.
	 */
	if (!u->myuser || !u->myuser->identified)
	{
		notice(chansvs.nick, origin, "You are not logged in.");
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		notice(chansvs.nick, origin, "The channel \2%s\2 is not registered.", chan);
		return;
	}

	mu = myuser_find(uname);

	/* ADD */
	if (!strcasecmp("ADD", cmd))
	{
		if (mu)
		{
			/* NEVEROP logic simplification and fix. Yes, this looks a little weird. --w00t */
			if (MU_NEVEROP & mu->flags)
			{
				notice(chansvs.nick, origin, "\2%s\2 does not wish to be added to access lists.", mu->name);
				return;
			}
		}

		/* VOP */
		if (level == CA_VOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_FLAGS)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				/* we might be adding a hostmask */
				if (!validhostmask(uname))
				{
					notice(chansvs.nick, origin, "\2%s\2 is neither a username nor hostmask.", uname);
					return;
				}

				if (chanacs_find_host_literal(mc, uname, CA_VOP))
				{
					notice(chansvs.nick, origin, "\2%s\2 is already on the VOP list for \2%s\2", uname, mc->name);
					return;
				}

				uname = collapse(uname);

				chanacs_add_host(mc, uname, CA_VOP);

				verbose(mc, "\2%s\2 added \2%s\2 to the VOP list.", u->nick, uname);

				notice(chansvs.nick, origin, "\2%s\2 has been added to the VOP list for \2%s\2.", uname, mc->name);

				/* run through the channel's user list and do it */
				LIST_FOREACH(n, mc->chan->members.head)
				{
					cu = (chanuser_t *)n->data;

					if (cu->modes & CMODE_VOICE)
						return;

					hostbuf[0] = '\0';

					strlcat(hostbuf, cu->user->nick, BUFSIZE);
					strlcat(hostbuf, "!", BUFSIZE);
					strlcat(hostbuf, cu->user->user, BUFSIZE);
					strlcat(hostbuf, "@", BUFSIZE);
					strlcat(hostbuf, cu->user->host, BUFSIZE);

					if (should_voice_host(mc, hostbuf))
					{
						cmode(chansvs.nick, mc->name, "+v", cu->user->nick);
						cu->modes |= CMODE_VOICE;
					}
				}

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_VOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already on the VOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_HOP)))
			{
				chanacs_delete(mc, mu, CA_HOP);
				chanacs_add(mc, mu, CA_VOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2HOP\2 to \2VOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2HOP\2 to \2VOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_AOP)))
			{
				chanacs_delete(mc, mu, CA_AOP);
				chanacs_add(mc, mu, CA_VOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2AOP\2 to \2VOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2AOP\2 to \2VOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_SOP)))
			{
				chanacs_delete(mc, mu, CA_SOP);
				chanacs_add(mc, mu, CA_VOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2SOP\2 to \2VOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2SOP\2 to \2VOP\2.", u->nick, mu->name);

				return;
			}

			chanacs_add(mc, mu, CA_VOP);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the VOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the VOP list.", u->nick, mu->name);

			return;
		}

		/* HOP */
		if (level == CA_HOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_FLAGS)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				/* we might be adding a hostmask */
				if (!validhostmask(uname))
				{
					notice(chansvs.nick, origin, "\2%s\2 is neither a username nor hostmask.", uname);
					return;
				}

				if (chanacs_find_host_literal(mc, uname, CA_HOP))
				{
					notice(chansvs.nick, origin, "\2%s\2 is already on the HOP list for \2%s\2", uname, mc->name);
					return;
				}

				uname = collapse(uname);

				chanacs_add_host(mc, uname, CA_HOP);

				verbose(mc, "\2%s\2 added \2%s\2 to the HOP list.", u->nick, uname);

				notice(chansvs.nick, origin, "\2%s\2 has been added to the HOP list for \2%s\2.", uname, mc->name);

				/* run through the channel's user list and do it */
				LIST_FOREACH(n, mc->chan->members.head)
				{
					cu = (chanuser_t *)n->data;

					if (cu->modes & ircd->halfops_mode)
						return;

					hostbuf[0] = '\0';

					strlcat(hostbuf, cu->user->nick, BUFSIZE);
					strlcat(hostbuf, "!", BUFSIZE);
					strlcat(hostbuf, cu->user->user, BUFSIZE);
					strlcat(hostbuf, "@", BUFSIZE);
					strlcat(hostbuf, cu->user->host, BUFSIZE);

					if (should_halfop_host(mc, hostbuf))
					{
						cmode(chansvs.nick, mc->name, "+h", cu->user->nick);
						cu->modes |= ircd->halfops_mode;
					}
				}

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_HOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already on the HOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_VOP)))
			{
				chanacs_delete(mc, mu, CA_VOP);
				chanacs_add(mc, mu, CA_HOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2VOP\2 to \2HOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2AOP\2 to \2HOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_AOP)))
			{
				chanacs_delete(mc, mu, CA_AOP);
				chanacs_add(mc, mu, CA_HOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2AOP\2 to \2HOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2AOP\2 to \2HOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_SOP)))
			{
				chanacs_delete(mc, mu, CA_SOP);
				chanacs_add(mc, mu, CA_HOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2SOP\2 to \2HOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2SOP\2 to \2VOP\2.", u->nick, mu->name);

				return;
			}

			chanacs_add(mc, mu, CA_HOP);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the HOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the HOP list.", u->nick, mu->name);

			return;
		}

		if (level == CA_AOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_FLAGS)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				/* we might be adding a hostmask */
				if (!validhostmask(uname))
				{
					notice(chansvs.nick, origin, "\2%s\2 is neither a username nor hostmask.", uname);
					return;
				}

				if (chanacs_find_host_literal(mc, uname, CA_AOP))
				{
					notice(chansvs.nick, origin, "\2%s\2 is already on the AOP list for \2%s\2", uname, mc->name);
					return;
				}

				uname = collapse(uname);

				chanacs_add_host(mc, uname, CA_AOP);

				verbose(mc, "\2%s\2 added \2%s\2 to the AOP list.", u->nick, uname);

				notice(chansvs.nick, origin, "\2%s\2 has been added to the AOP list for \2%s\2.", uname, mc->name);

				/* run through the channel's user list and do it */
				LIST_FOREACH(n, mc->chan->members.head)
				{
					cu = (chanuser_t *)n->data;

					if (cu->modes & CMODE_OP)
						return;

					hostbuf[0] = '\0';

					strlcat(hostbuf, cu->user->nick, BUFSIZE);
					strlcat(hostbuf, "!", BUFSIZE);
					strlcat(hostbuf, cu->user->user, BUFSIZE);
					strlcat(hostbuf, "@", BUFSIZE);
					strlcat(hostbuf, cu->user->host, BUFSIZE);

					if (should_op_host(mc, hostbuf))
					{
						cmode(chansvs.nick, mc->name, "+o", cu->user->nick);
						cu->modes |= CMODE_OP;
					}
				}

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_AOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already on the AOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_VOP)))
			{
				chanacs_delete(mc, mu, CA_VOP);
				chanacs_add(mc, mu, CA_AOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2VOP\2 to \2AOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2VOP\2 to \2AOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_HOP)))
			{
				chanacs_delete(mc, mu, CA_HOP);
				chanacs_add(mc, mu, CA_AOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2HOP\2 to \2AOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2HOP\2 to \2AOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_SOP)))
			{
				chanacs_delete(mc, mu, CA_SOP);
				chanacs_add(mc, mu, CA_AOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2SOP\2 to \2AOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2SOP\2 to \2AOP\2.", u->nick, mu->name);

				return;
			}

			chanacs_add(mc, mu, CA_AOP);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the AOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the AOP list.", u->nick, mu->name);

			return;
		}

		if (level == CA_SOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				if (validhostmask(uname))
					notice(chansvs.nick, origin, "Hostmasks cannot be added to the SOP list.");
				else
					notice(chansvs.nick, origin, "\2%s\2 is not registered.", uname);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_SOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is already on the SOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_VOP)))
			{
				chanacs_delete(mc, mu, CA_VOP);
				chanacs_add(mc, mu, CA_SOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2VOP\2 to \2SOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2VOP\2 to \2SOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_HOP)))
			{
				chanacs_delete(mc, mu, CA_HOP);
				chanacs_add(mc, mu, CA_SOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2HOP\2 to \2SOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2HOP\2 to \2SOP\2.", u->nick, mu->name);

				return;
			}

			if ((ca = chanacs_find(mc, mu, CA_AOP)))
			{
				chanacs_delete(mc, mu, CA_AOP);
				chanacs_add(mc, mu, CA_SOP);

				notice(chansvs.nick, origin, "\2%s\2's access on \2%s\2 has been changed from " "\2AOP\2 to \2SOP\2.", mu->name, mc->name);

				verbose(mc, "\2%s\2 changed \2%s\2's access from \2AOP\2 to \2SOP\2.", u->nick, mu->name);

				return;
			}

			chanacs_add(mc, mu, CA_SOP);

			notice(chansvs.nick, origin, "\2%s\2 has been added to the SOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 added \2%s\2 to the SOP list.", u->nick, mu->name);

			return;
		}
	}

	else if (!strcasecmp("DEL", cmd))
	{
		/* VOP */
		if (level == CA_VOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_FLAGS)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				/* we might be deleting a hostmask */
				if (!validhostmask(uname))
				{
					notice(chansvs.nick, origin, "\2%s\2 is neither a username nor hostmask.", uname);
					return;
				}

				if (!chanacs_find_host_literal(mc, uname, CA_VOP))
				{
					notice(chansvs.nick, origin, "\2%s\2 is not on the VOP list for \2%s\2.", uname, mc->name);
					return;
				}

				chanacs_delete_host(mc, uname, CA_VOP);

				verbose(mc, "\2%s\2 removed \2%s\2 from the VOP list.", u->nick, uname);

				notice(chansvs.nick, origin, "\2%s\2 has been removed from the VOP list for \2%s\2.", uname, mc->name);

				return;
			}

			if (!(ca = chanacs_find(mc, mu, CA_VOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the VOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			chanacs_delete(mc, mu, CA_VOP);

			notice(chansvs.nick, origin, "\2%s\2 has been removed from the VOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 removed \2%s\2 from the VOP list.", u->nick, mu->name);

			return;
		}

		/* VOP */
		if (level == CA_HOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_FLAGS)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				/* we might be deleting a hostmask */
				if (!validhostmask(uname))
				{
					notice(chansvs.nick, origin, "\2%s\2 is neither a username nor hostmask.", uname);
					return;
				}

				if (!chanacs_find_host_literal(mc, uname, CA_HOP))
				{
					notice(chansvs.nick, origin, "\2%s\2 is not on the HOP list for \2%s\2.", uname, mc->name);
					return;
				}

				chanacs_delete_host(mc, uname, CA_HOP);

				verbose(mc, "\2%s\2 removed \2%s\2 from the HOP list.", u->nick, uname);

				notice(chansvs.nick, origin, "\2%s\2 has been removed from the HOP list for \2%s\2.", uname, mc->name);

				return;
			}

			if (!(ca = chanacs_find(mc, mu, CA_HOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the HOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			chanacs_delete(mc, mu, CA_HOP);

			notice(chansvs.nick, origin, "\2%s\2 has been removed from the HOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 removed \2%s\2 from the HOP list.", u->nick, mu->name);

			return;
		}

		if (level == CA_AOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)) && (!is_xop(mc, u->myuser, CA_FLAGS)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				/* we might be deleting a hostmask */
				if (!validhostmask(uname))
				{
					notice(chansvs.nick, origin, "\2%s\2 is neither a username nor hostmask.", uname);
					return;
				}

				if (!chanacs_find_host_literal(mc, uname, CA_AOP))
				{
					notice(chansvs.nick, origin, "\2%s\2 is not on the AOP list for \2%s\2.", uname, mc->name);
					return;
				}

				chanacs_delete_host(mc, uname, CA_AOP);

				verbose(mc, "\2%s\2 removed \2%s\2 from the AOP list.", u->nick, uname);
				notice(chansvs.nick, origin, "\2%s\2 has been removed from the AOP list for \2%s\2.", uname, mc->name);

				return;
			}

			if (!(ca = chanacs_find(mc, mu, CA_AOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the AOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			chanacs_delete(mc, mu, CA_AOP);

			notice(chansvs.nick, origin, "\2%s\2 has been removed from the AOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 removed \2%s\2 from the AOP list.", u->nick, mu->name);

			return;
		}

		/* SOP */
		if (level == CA_SOP)
		{
			if ((!is_founder(mc, u->myuser)) && (!is_successor(mc, u->myuser)))
			{
				notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
				return;
			}

			if (!mu)
			{
				notice(chansvs.nick, origin, "\2%s\2 is not registered.", uname);
				return;
			}

			if (!(ca = chanacs_find(mc, mu, CA_SOP)))
			{
				notice(chansvs.nick, origin, "\2%s\2 is not on the SOP list for \2%s\2.", mu->name, mc->name);
				return;
			}

			chanacs_delete(mc, mu, CA_SOP);

			notice(chansvs.nick, origin, "\2%s\2 has been removed from the SOP list for \2%s\2.", mu->name, mc->name);

			verbose(mc, "\2%s\2 removed \2%s\2 from the SOP list.", u->nick, mu->name);

			return;
		}
	}

	else if (!strcasecmp("LIST", cmd))
	{
		if (!is_founder(mc, u->myuser) && !is_successor(mc, u->myuser) && !is_xop(mc, u->myuser, CA_ACLVIEW))
		{
			notice(chansvs.nick, origin, "You are not authorized to perform this operation.");
			return;
		}

		/* VOP */
		if (level == CA_VOP)
		{
			uint8_t i = 0;


			notice(chansvs.nick, origin, "VOP list for \2%s\2:", mc->name);

			LIST_FOREACH(n, mc->chanacs.head)
			{
				ca = (chanacs_t *)n->data;

				if (ca->level == CA_VOP)
				{
					if (ca->host)
						notice(chansvs.nick, origin, "%d: \2%s\2", ++i, ca->host);

					else if (ca->myuser->user)
						notice(chansvs.nick, origin, "%d: \2%s\2 (logged in from \2%s\2)", ++i, ca->myuser->name, ca->myuser->user->nick);
					else
						notice(chansvs.nick, origin, "%d: \2%s\2 (not logged in)", ++i, ca->myuser->name);
				}
			}

			notice(chansvs.nick, origin, "Total of \2%d\2 %s in \2%s\2's VOP list.", i, (i == 1) ? "entry" : "entries", mc->name);
		}

		if (level == CA_HOP)
		{
			uint8_t i = 0;

			notice(chansvs.nick, origin, "HOP list for \2%s\2:", mc->name);

			LIST_FOREACH(n, mc->chanacs.head)
			{
				ca = (chanacs_t *)n->data;

				if (ca->level == CA_HOP)
				{
					if (ca->host)
						notice(chansvs.nick, origin, "%d: \2%s\2", ++i, ca->host);

					else if (ca->myuser->user)
						notice(chansvs.nick, origin, "%d: \2%s\2 (logged in from \2%s\2)", ++i, ca->myuser->name, ca->myuser->user->nick);
					else
						notice(chansvs.nick, origin, "%d: \2%s\2 (not logged in)", ++i, ca->myuser->name);
				}
			}

			notice(chansvs.nick, origin, "Total of \2%d\2 %s in \2%s\2's HOP list.", i, (i == 1) ? "entry" : "entries", mc->name);
		}

		if (level == CA_AOP)
		{
			uint8_t i = 0;

			notice(chansvs.nick, origin, "AOP list for \2%s\2:", mc->name);

			LIST_FOREACH(n, mc->chanacs.head)
			{
				ca = (chanacs_t *)n->data;

				if (ca->level == CA_AOP)
				{
					if (ca->host)
						notice(chansvs.nick, origin, "%d: \2%s\2", ++i, ca->host);

					else if (ca->myuser->user)
						notice(chansvs.nick, origin, "%d: \2%s\2 (logged in from \2%s\2)", ++i, ca->myuser->name, ca->myuser->user->nick);
					else
						notice(chansvs.nick, origin, "%d: \2%s\2 (not logged in)", ++i, ca->myuser->name);
				}
			}

			notice(chansvs.nick, origin, "Total of \2%d\2 %s in \2%s\2's AOP list.", i, (i == 1) ? "entry" : "entries", mc->name);
		}

		if (level == CA_SOP)
		{
			uint8_t i = 0;

			notice(chansvs.nick, origin, "SOP list for \2%s\2:", mc->name);

			LIST_FOREACH(n, mc->chanacs.head)
			{
				ca = (chanacs_t *)n->data;

				if (ca->level == CA_SOP)
				{
					if (ca->host)
						notice(chansvs.nick, origin, "%d: \2%s\2", ++i, ca->host);

					else if (ca->myuser->user)
						notice(chansvs.nick, origin, "%d: \2%s\2 (logged in from \2%s\2)", ++i, ca->myuser->name, ca->myuser->user->nick);
					else
						notice(chansvs.nick, origin, "%d: \2%s\2 (not logged in)", ++i, ca->myuser->name);
				}
			}

			notice(chansvs.nick, origin, "Total of \2%d\2 %s in \2%s\2's SOP list.", i, (i == 1) ? "entry" : "entries", mc->name);
		}
	}
}

static void cs_cmd_sop(char *origin)
{
	cs_xop(origin, CA_SOP);
}

static void cs_cmd_aop(char *origin)
{
	cs_xop(origin, CA_AOP);
}

static void cs_cmd_vop(char *origin)
{
	cs_xop(origin, CA_VOP);
}

static void cs_cmd_hop(char *origin)
{
	cs_xop(origin, CA_HOP);
}

