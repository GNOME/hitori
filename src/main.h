/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Hitori
 * Copyright (C) Philip Withnall 2007-2008 <philip@tecnocode.co.uk>
 * 
 * Hitori is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Hitori is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hitori.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <glib.h>

#ifndef HITORI_MAIN_H
#define HITORI_MAIN_H

G_BEGIN_DECLS

#define DEFAULT_BOARD_SIZE 5

typedef struct {
	guchar x;
	guchar y;
} HitoriVector;

typedef enum {
	UNDO_NEW_GAME,
	UNDO_PAINT,
	UNDO_TAG1,
	UNDO_TAG2
} HitoriUndoType;

typedef struct _HitoriUndo HitoriUndo;
struct _HitoriUndo {
	HitoriUndoType type;
	HitoriVector cell;
	HitoriUndo *undo;
	HitoriUndo *redo;
};

typedef enum {
	CELL_PAINTED = 1 << 1,
	CELL_SHOULD_BE_PAINTED = 1 << 2,
	CELL_TAG1 = 1 << 3,
	CELL_TAG2 = 1 << 4
} HitoriCellStatus;

typedef struct {
	guchar num;
	guchar status;
} HitoriCell;

typedef struct {
	GtkWidget *window;
	GtkWidget *drawing_area;
	GtkAction *undo_action;
	GtkAction *redo_action;
	GtkAction *hint_action;

	gdouble drawing_area_width;
	gdouble drawing_area_height;

	guint drawing_area_x_offset;
	guint drawing_area_y_offset;

	guint board_size;
	HitoriCell **board;

	gboolean debug;
	gboolean processing_events;
	gboolean made_a_move;
	HitoriUndo *undo_stack;

	guint hint_status;
	HitoriVector hint_position;

	gboolean display_error;
	HitoriVector error_position;
} Hitori;

void hitori_new_game (Hitori *hitori, guint board_size);
void hitori_clear_undo_stack (Hitori *hitori);
void hitori_set_board_size (Hitori *hitori, guint board_size);
void hitori_print_board (Hitori *hitori);
void hitori_free_board (Hitori *hitori);
void hitori_enable_events (Hitori *hitori);
void hitori_disable_events (Hitori *hitori);
void hitori_set_error_position (Hitori *hitori, HitoriVector position);
void hitori_quit (Hitori *hitori);

G_END_DECLS

#endif /* HITORI_MAIN_H */
