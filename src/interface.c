/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Hitori
 * Copyright (C) Philip Withnall 2007 <philip@tecnocode.co.uk>
 * 
 * Hitori is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * Hitori is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Hitori.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include <math.h>
#include "config.h"
#include "main.h"
#include "interface.h"
#include "rules.h"

GtkWidget*
hitori_create_interface (Hitori *hitori)
{
	GError *error = NULL;
	GtkBuilder *builder;

	builder = gtk_builder_new ();

	if (gtk_builder_add_from_file (builder, PACKAGE_DATA_DIR"/hitori/hitori.ui", &error) == FALSE) {
		/* Show an error */
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("UI file \""PACKAGE_DATA_DIR"/hitori/hitori.ui\" could not be loaded. Error: %s"), error->message);
		g_signal_connect_swapped (dialog, "response", 
				G_CALLBACK (gtk_widget_destroy),
				dialog);
		gtk_widget_show_all (dialog);

		g_error_free (error);
		g_object_unref (builder);
		hitori_quit (hitori);

		return NULL;
	}

	gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
	gtk_builder_connect_signals (builder, hitori);

	/* Setup the main window */
	hitori->window = GTK_WIDGET (gtk_builder_get_object (builder, "hitori_main_window"));
	hitori->drawing_area = GTK_WIDGET (gtk_builder_get_object (builder, "hitori_drawing_area"));
	hitori->undo_action = GTK_ACTION (gtk_builder_get_object (builder, "undo_menu"));
	hitori->redo_action = GTK_ACTION (gtk_builder_get_object (builder, "redo_menu"));
	hitori->hint_action = GTK_ACTION (gtk_builder_get_object (builder, "hint_menu"));

	g_object_unref (builder);

	return hitori->window;
}

void
hitori_draw_board (Hitori *hitori, cairo_t *cr, gboolean check_win)
{
	gint area_width, area_height;
	guint x, y, board_width, board_height;
	gfloat cell_size, colour_modifier;
	gdouble x_pos, y_pos;
	cairo_text_extents_t extents;

	gdk_drawable_get_size (GDK_DRAWABLE (hitori->drawing_area->window), &area_width, &area_height);

	/* Clamp the width/height to the minimum */
	if (area_height < area_width) {
		board_width = area_height;
		board_height = area_height;
	} else {
		board_width = area_width;
		board_height = area_width;
	}

	cell_size = board_width / BOARD_SIZE;
	cairo_set_font_size (cr, cell_size * FONT_SCALE);

	/* Centre the board */
	hitori->drawing_area_x_offset = (area_width - board_width) / 2;
	hitori->drawing_area_y_offset = (area_height - board_height) / 2;
	cairo_translate (cr, hitori->drawing_area_x_offset, hitori->drawing_area_y_offset);

	/* Draw the cells */
	x_pos = 0;
	for (x = 0; x < BOARD_SIZE; x++) { /* columns (X) */
		y_pos = 0;
		for (y = 0; y < BOARD_SIZE; y++) { /* rows (Y) */
			if (hitori->board[x][y].painted == TRUE)
				colour_modifier = 1.5;
			else
				colour_modifier = 1.0;

			/* Draw the fill */
			cairo_set_source_rgb (cr, 1.0 / colour_modifier, 1.0 / colour_modifier, 1.0 / colour_modifier); /* white / 50% grey */
			cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
			cairo_fill (cr);

			/* If the cell is tagged, draw the tag dots */
			if (hitori->board[x][y].tag1 == TRUE) {
				cairo_set_source_rgb (cr, 1.0 / colour_modifier, 0, 0); /* red */
				cairo_move_to (cr, x_pos, y_pos + TAG_OFFSET);
				cairo_line_to (cr, x_pos, y_pos);
				cairo_line_to (cr, x_pos + TAG_OFFSET, y_pos);
				cairo_arc (cr, x_pos + TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.0, 0.5 * M_PI);
				cairo_fill (cr);
			}
			if (hitori->board[x][y].tag2 == TRUE) {
				cairo_set_source_rgb (cr, 0, 1.0 / colour_modifier, 0); /* green */
				cairo_move_to (cr, x_pos + cell_size - TAG_OFFSET, y_pos);
				cairo_line_to (cr, x_pos + cell_size, y_pos);
				cairo_line_to (cr, x_pos + cell_size, y_pos + TAG_OFFSET);
				cairo_arc (cr, x_pos + cell_size - TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.5 * M_PI, 1.0 * M_PI);
				cairo_fill (cr);
			}

			/* Draw the border */
			cairo_set_source_rgb (cr, 0, 0, 0); /* black */
			cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
			cairo_stroke (cr);

			/* Draw the text */
			gchar *text;

			text = g_strdup_printf ("%u", hitori->board[x][y].num);
			cairo_text_extents (cr, text, &extents);
			cairo_set_source_rgb (cr, 0, 0, 0); /* black */
			cairo_move_to (cr,
				x_pos + (cell_size - extents.width) / 2,
				y_pos + cell_size - (cell_size - extents.height) / 2);
			cairo_show_text (cr, text);
			g_free (text);

			y_pos += cell_size;
		}

		x_pos += cell_size;
	}

	/* Draw a hint if applicable */
	if (hitori->hint_status % 2 == 1) {
		cairo_set_source_rgb (cr, 1, 0, 0); /* red */
		cairo_rectangle (cr, hitori->hint_x * cell_size, hitori->hint_y * cell_size, cell_size, cell_size);
		cairo_stroke (cr);
	}

	/* Check to see if all three rules are satisfied yet.
	 * If they are, we've won. */
	if (check_win &&
	    hitori_check_rule1 (hitori) &&
	    hitori_check_rule2 (hitori) &&
	    hitori_check_rule3 (hitori)) {
		/* Win! */
		hitori_disable_events (hitori);
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				_("You've won!"));
		g_signal_connect_swapped (dialog, "response", 
				G_CALLBACK (gtk_widget_destroy),
				dialog);
		gtk_widget_show_all (dialog);
	}
}

void
hitori_draw_board_simple (Hitori *hitori, gboolean check_win)
{
	cairo_t *cr;

	cr = gdk_cairo_create (GDK_DRAWABLE (hitori->drawing_area->window));
	hitori_draw_board (hitori, cr, check_win);
	cairo_destroy (cr);
}

gboolean
hitori_expose_cb (GtkWidget *drawing_area, GdkEventExpose *event, Hitori *hitori)
{
	cairo_t *cr;

	cr = gdk_cairo_create (GDK_DRAWABLE (hitori->drawing_area->window));
	cairo_rectangle (cr, event->area.x, event->area.y,
				event->area.width, event->area.height);
	cairo_clip (cr);
	hitori_draw_board (hitori, cr, FALSE);
	cairo_destroy (cr);

	return FALSE;
}

gboolean
hitori_button_release_cb (GtkWidget *drawing_area, GdkEventButton *event, Hitori *hitori)
{
	if (hitori->processing_events == FALSE)
		return FALSE;

	gint width, height;
	gfloat cell_size;
	guint x, y;
	cairo_t *cr;
	HitoriUndo *undo;
	gboolean recheck = FALSE;

	gdk_drawable_get_size (GDK_DRAWABLE (hitori->drawing_area->window), &width, &height);

	/* Clamp the width/height to the minimum */
	if (height < width)
		width = height;
	else if (width < height)
		height = width;

	cell_size = width / BOARD_SIZE;

	/* Determine the cell in which the button was released */
	x = floor ((event->x - hitori->drawing_area_x_offset) / cell_size);
	y = floor ((event->y - hitori->drawing_area_y_offset) / cell_size);

	if (x >= BOARD_SIZE || y >= BOARD_SIZE)
		return FALSE;

	/* Update the undo stack */
	undo = g_new (HitoriUndo, 1);
	undo->x = x;
	undo->y = y;
	undo->undo = hitori->undo_stack;
	undo->redo = NULL;

	if (event->state & GDK_SHIFT_MASK) {
		/* Update tag 1's state */
		hitori->board[x][y].tag1 = !(hitori->board[x][y].tag1);
		undo->type = UNDO_TAG1;
	} else if (event->state & GDK_CONTROL_MASK) {
		/* Update tag 2's state */
		hitori->board[x][y].tag2 = !(hitori->board[x][y].tag2);
		undo->type = UNDO_TAG2;
	} else {
		/* Update the paint overlay */
		hitori->board[x][y].painted = !(hitori->board[x][y].painted);
		undo->type = UNDO_PAINT;
		recheck = TRUE;
	}

	if (hitori->undo_stack != NULL)
		hitori->undo_stack->redo = undo;
	hitori->undo_stack = undo;
	gtk_action_set_sensitive (hitori->undo_action, TRUE);

	/* Stop any current hints */
	hitori->hint_status = HINT_FLASHES;

	/* Redraw */
	cr = gdk_cairo_create (GDK_DRAWABLE (hitori->drawing_area->window));
	cairo_rectangle (cr, x * cell_size + hitori->drawing_area_x_offset, y * cell_size + hitori->drawing_area_y_offset,
				cell_size, cell_size);
	cairo_clip (cr);
	hitori_draw_board (hitori, cr, recheck);
	cairo_destroy (cr);

	return FALSE;
}

void
hitori_destroy_cb (GtkWindow *window, Hitori *hitori)
{
	hitori_quit (hitori);
}

void
hitori_new_game_cb (GtkAction *action, Hitori *hitori)
{
	hitori_new_game (hitori);
}

static gboolean
hitori_update_hint (gpointer user_data)
{
	Hitori *hitori;
	gboolean return_value = FALSE;

	hitori = (Hitori*) user_data;
	hitori->hint_status++;

	if (hitori->hint_status >= HINT_FLASHES) {
		hitori->hint_x = 0;
		hitori->hint_y = 0;
		hitori->hint_status = 0;
	} else {
		return_value = TRUE;
	}

	hitori_draw_board_simple (hitori, FALSE);

	return return_value;
}

void
hitori_hint_cb (GtkAction *action, Hitori *hitori)
{
	guint x, y;

	/* Find the first cell which should be painted, but isn't (or vice-versa) */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if ((hitori->board[x][y].painted == FALSE && hitori->board[x][y].should_be_painted == TRUE) ||
			    (hitori->board[x][y].painted == TRUE && hitori->board[x][y].should_be_painted == FALSE)) {
				/* Set up the cell for hinting */
				hitori->hint_status = 0;
				hitori->hint_x = x;
				hitori->hint_y = y;
				g_timeout_add (HINT_INTERVAL, hitori_update_hint,(gpointer) hitori);
				hitori_update_hint ((gpointer) hitori);
				return;
			}
		}
	}
}

void
hitori_undo_cb (GtkAction *action, Hitori *hitori)
{
	if (hitori->undo_stack->undo == NULL)
		return;

	switch (hitori->undo_stack->type) {
		case UNDO_PAINT:
			hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].painted = !(hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].painted);
			break;
		case UNDO_TAG1:
			hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag1 = !(hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag1);
			break;
		case UNDO_TAG2:
			hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag2 = !(hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag2);
			break;
		case UNDO_NEW_GAME:
			/* This is just here to stop the compiler warning */
			break;
	}

	hitori->undo_stack = hitori->undo_stack->undo;

	gtk_action_set_sensitive (hitori->redo_action, TRUE);
	if (hitori->undo_stack->undo == NULL || hitori->undo_stack->type == UNDO_NEW_GAME)
		gtk_action_set_sensitive (hitori->undo_action, FALSE);

	/* Redraw */
	hitori_draw_board_simple (hitori, TRUE);
}

void
hitori_redo_cb (GtkAction *action, Hitori *hitori)
{
	if (hitori->undo_stack->redo == NULL)
		return;

	hitori->undo_stack = hitori->undo_stack->redo;

	switch (hitori->undo_stack->type) {
		case UNDO_PAINT:
			hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].painted = !(hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].painted);
			break;
		case UNDO_TAG1:
			hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag1 = !(hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag1);
			break;
		case UNDO_TAG2:
			hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag2 = !(hitori->board[hitori->undo_stack->x][hitori->undo_stack->y].tag2);
			break;
		case UNDO_NEW_GAME:
			/* This is just here to stop the compiler warning */
			break;
	}

	gtk_action_set_sensitive (hitori->undo_action, TRUE);
	if (hitori->undo_stack->redo == NULL)
		gtk_action_set_sensitive (hitori->redo_action, FALSE);

	/* Redraw */
	hitori_draw_board_simple (hitori, TRUE);
}

void
hitori_quit_cb (GtkAction *action, Hitori *hitori)
{
	hitori_quit (hitori);
}

void
hitori_about_cb (GtkAction *action, Hitori *hitori)
{
	gchar *license;
	const gchar *description = N_("A logic puzzle designed by Nikoli.");
	const gchar *authors[] =
	{
		"Philip Withnall <philip@tecnocode.co.uk>",
		"Ben Windsor <benjw_823@hotmail.com>",
		NULL
	};
	const gchar *license_parts[] = {
		N_("Hitori is free software; you can redistribute it and/or modify "
		   "it under the terms of the GNU General Public License as published by "
		   "the Free Software Foundation; either version 2 of the License, or "
		   "(at your option) any later version."),
		N_("Hitori is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		   "along with Hitori; if not, write to the Free Software Foundation, Inc., "
		   "59 Temple Place, Suite 330, Boston, MA  02111-1307  USA"),
	};

	license = g_strjoin ("\n\n",
			  _(license_parts[0]),
			  _(license_parts[1]),
			  _(license_parts[2]),
			  NULL);

	gtk_show_about_dialog (GTK_WINDOW (hitori->window),
				"version", VERSION,
				"copyright", _("Copyright \xc2\xa9 2007 Philip Withnall"),
				"comments", _(description),
				"authors", authors,
				"translator-credits", _("translator-credits"),
				"logo-icon-name", "hitori",
				"license", license,
				"wrap-license", TRUE,
				"website-label", _("Hitori Website"),
				"website", "http://tecnocode.co.uk?page=blog&action=view_item&id=60",
				NULL);

	g_free (license);
}
