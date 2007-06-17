/*
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Table rendering class.
 *
 * $Id: table.h 7941 2007-03-13 07:16:52Z nenolod $
 */

#ifndef ATHEME_TABLE_H
#define ATHEME_TABLE_H

typedef struct {
	object_t parent;
	list_t rows;
} table_t;

typedef struct {
	int id;
	list_t cells;
} table_row_t;

typedef struct {
	int width;	/* only if first row. */
	char *name;
	char *value;
} table_cell_t;

/*
 * Creates a new table object. Use object_unref() to destroy it.
 */
E table_t *table_new(const char *fmt, ...);

/*
 * Renders a table, each line going to callback().
 */
E void table_render(table_t *t, void (*callback)(const char *line, void *data), void *data);

/*
 * Associates a value with a row.
 */
E void table_cell_associate(table_row_t *r, const char *name, const char *value);

/*
 * Associates a row with a table.
 */
E table_row_t *table_row_new(table_t *t);

#endif
