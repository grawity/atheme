/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Table rendering class.
 * NOTE: This is a work in progress and will probably change considerably
 * later on.
 *
 * $Id: table.c 8343 2007-05-30 22:58:47Z jilles $
 */

#include "atheme.h"

static void table_destroy(void *obj)
{
	table_t *table = (table_t *) obj;
	node_t *n, *tn;

	return_if_fail(table != NULL);

	LIST_FOREACH_SAFE(n, tn, table->rows.head)
	{
		table_row_t *r = (table_row_t *) n->data;
		node_t *n2, *tn2;

		return_if_fail(r != NULL);

		LIST_FOREACH_SAFE(n2, tn2, r->cells.head)
		{
			table_cell_t *c = (table_cell_t *) n2->data;

			free(c->name);
			free(c->value);
			free(c);
			node_del(n2, &r->cells);
			node_free(n2);
		}

		free(r);

		node_del(n, &table->rows);
		node_free(n);
	}

	free(table);
}

/*
 * table_new(const char *fmt, ...)
 *
 * Table constructor.
 *
 * Inputs:
 *     - printf-style string to name the table with.
 *
 * Outputs:
 *     - a table object.
 *
 * Side Effects:
 *     - none
 */
table_t *table_new(const char *fmt, ...)
{
	va_list vl;
	char buf[BUFSIZE];
	table_t *out;

	return_val_if_fail(fmt != NULL, NULL);

	va_start(vl, fmt);
	vsnprintf(buf, BUFSIZE, fmt, vl);
	va_end(vl);

	out = scalloc(sizeof(table_t), 1);
	object_init(&out->parent, buf, table_destroy);

	return out;
}

/*
 * table_row_new
 *
 * Creates a table row. This isn't an object.
 *
 * Inputs:
 *     - table to associate to.
 *
 * Outputs:
 *     - a table row
 *
 * Side Effects:
 *     - none
 */
table_row_t *table_row_new(table_t *t)
{
	table_row_t *out;

	return_val_if_fail(t != NULL, NULL);

	out = scalloc(sizeof(table_row_t), 1);

	node_add(out, node_create(), &t->rows);

	return out;
}

/*
 * table_cell_associate
 *
 * Associates a cell with a row.
 *
 * Inputs:
 *     - row to add cell to.
 *
 * Outputs:
 *     - nothing
 *
 * Side Effects:
 *     - none
 */
void table_cell_associate(table_row_t *r, const char *name, const char *value)
{
	table_cell_t *c;

	return_if_fail(r != NULL);
	return_if_fail(name != NULL);
	return_if_fail(value != NULL);

	c = scalloc(sizeof(table_cell_t), 1);

	c->name = sstrdup(name);
	c->value = sstrdup(value);

	node_add(c, node_create(), &r->cells);
}

/*
 * table_render
 *
 * Renders a table. This is a two-pass operation, the first one to find out
 * the width of each cell, and then the second to render the data.
 *
 * Inputs:
 *     - a table
 *     - a callback function
 *     - opaque data
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - a callback function is called for each row of the table.
 */
void table_render(table_t *t, void (*callback)(const char *line, void *data), void *data)
{
	node_t *n;
	table_row_t *f;
	size_t bufsz = 0;
	char *buf = NULL;
	char *p;
	int i;

	return_if_fail(t != NULL);
	return_if_fail(callback != NULL);

	f = (table_row_t *) t->rows.head->data;

	LIST_FOREACH(n, t->rows.head)
	{
		table_row_t *r = (table_row_t *) n->data;
		node_t *n2, *rn;

		/* we, uhh... we don't provide a macro for dealing with two lists at once ;) */
		for (n2 = r->cells.head, rn = f->cells.head;
		     n2 != NULL && rn != NULL; n2 = n2->next, rn = rn->next)
		{
			table_cell_t *c, *rc;
			size_t sz;

			c  = (table_cell_t *) n2->data;
			rc = (table_cell_t *) rn->data;

			if ((sz = strlen(c->name)) > (size_t)rc->width)
				rc->width = sz;

			if ((sz = strlen(c->value)) > (size_t)rc->width)
				rc->width = sz;		
		}
	}

	/* now total up the result. */
	LIST_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;
		bufsz += c->width + 1;
	}

	buf = smalloc(bufsz);
	*buf = '\0';

	/* start outputting the header. */
	callback(object(t)->name, data);
	LIST_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;
		char fmtbuf[12];
		char buf2[1024];

		/* dynamically generate the format string based on width. */
		snprintf(fmtbuf, 12, n->next != NULL ? "%%-%ds " : "%%s", c->width);
		snprintf(buf2, 1024, fmtbuf, c->name);

		strlcat(buf, buf2, bufsz);
	}
	callback(buf, data);
	*buf = '\0';

	/* separator line */
	p = buf;
	LIST_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;

		if (n->next != NULL)
		{
			for (i = 0; i < c->width; i++)
				*p++ = '-';
			*p++ = ' ';
		}
		else
			for (i = 0; i < (int)strlen(c->name); i++)
				*p++ = '-';
	}
	*p = '\0';
	callback(buf, data);
	*buf = '\0';

	LIST_FOREACH(n, t->rows.head)
	{
		table_row_t *r = (table_row_t *) n->data;
		node_t *n2, *rn;

		for (n2 = r->cells.head, rn = f->cells.head;
		     n2 != NULL && rn != NULL; n2 = n2->next, rn = rn->next)
		{
			table_cell_t *c, *rc;
			char fmtbuf[12];
			char buf2[1024];

			c  = (table_cell_t *) n2->data;
			rc = (table_cell_t *) rn->data;

			/* dynamically generate the format string based on width. */
			snprintf(fmtbuf, 12, n2->next != NULL ? "%%-%ds " : "%%s", rc->width);
			snprintf(buf2, 1024, fmtbuf, c->value);

			strlcat(buf, buf2, bufsz);
		}
		callback(buf, data);
		*buf = '\0';
	}

	/* separator line */
	p = buf;
	LIST_FOREACH(n, f->cells.head)
	{
		table_cell_t *c = (table_cell_t *) n->data;

		if (n->next != NULL)
		{
			for (i = 0; i < c->width; i++)
				*p++ = '-';
			*p++ = ' ';
		}
		else
			for (i = 0; i < (int)strlen(c->name); i++)
				*p++ = '-';
	}
	*p = '\0';
	callback(buf, data);
	*buf = '\0';
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */

