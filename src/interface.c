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
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
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

/* Callbacks for GtkBuilder */
gboolean hitori_expose_cb (GtkWidget *drawing_area, GdkEventExpose *event, Hitori *hitori);
gboolean hitori_button_release_cb (GtkWidget *drawing_area, GdkEventButton *event, Hitori *hitori);
void hitori_destroy_cb (GtkWindow *window, Hitori *hitori);
void hitori_quit_cb (GtkAction *action, Hitori *hitori);

GtkWidget*
hitori_create_interface (Hitori *hitori)
{
	GError *error = NULL;

	hitori->builder = gtk_builder_new ();
	if (gtk_builder_add_from_file (hitori->builder, UI_FILE, &error) == FALSE) {
		/* Show an error */
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("UI file \"%s\" could not be loaded. Error: %s"), UI_FILE, error->message);
		g_signal_connect_swapped (dialog, "response", 
				G_CALLBACK (gtk_widget_destroy),
				dialog);
		gtk_widget_show_all (dialog);

		g_error_free (error);
		hitori_quit (hitori);
		return NULL;
	}

	gtk_builder_set_translation_domain (hitori->builder, GETTEXT_PACKAGE);
	gtk_builder_connect_signals (hitori->builder, hitori);

	/* Setup the main window */
	hitori->window = GTK_WIDGET (gtk_builder_get_object (hitori->builder, "hitori_main_window"));
	hitori->drawing_area = GTK_WIDGET (gtk_builder_get_object (hitori->builder, "hitori_drawing_area"));

	return hitori->window;
}

void
hitori_draw_board (Hitori *hitori, cairo_t *cr, gboolean check_win)
{
	gint width, height;
	guint x, y;
	gfloat cell_size;
	cairo_text_extents_t extents;

	gdk_drawable_get_size (GDK_DRAWABLE (hitori->drawing_area->window), &width, &height);

	/* Clamp the width/height to the minimum */
	if (height < width)
		width = height;
	else if (width < height)
		height = width;

	cell_size = width / BOARD_SIZE;
	cairo_set_font_size (cr, cell_size * FONT_SCALE);

	/* Draw the cells */
	for (x = 0; x < BOARD_SIZE; x++) { /* columns (X) */
		for (y = 0; y < BOARD_SIZE; y++) { /* rows (Y) */
			/* Draw the fill */
			if (hitori->board[x][y].painted == TRUE)
				cairo_set_source_rgb (cr, 0.5, 0.5, 0.5); /* 50% grey */
			else
				cairo_set_source_rgb (cr, 1, 1, 1); /* white */

			/* TODO: Optimise maths in these loops */
			cairo_rectangle (cr, x * cell_size, y * cell_size, cell_size, cell_size);
			cairo_fill_preserve (cr);

			/* Draw the border */
			cairo_set_source_rgb (cr, 0, 0, 0); /* black */
			cairo_stroke (cr);

			/* Draw the text */
			gchar *text;

			text = g_strdup_printf ("%u", hitori->board[x][y].num);
			cairo_text_extents (cr, text, &extents);
			cairo_set_source_rgb (cr, 0, 0, 0); /* black */
			cairo_move_to (cr,
				x * cell_size + (cell_size - extents.width) / 2,
				(y + 1) * cell_size - (cell_size - extents.height) / 2);
			cairo_show_text (cr, text);
			g_free (text);

			/* If the cell is tagged, draw the tag lines */
			if (hitori->board[x][y].tag1 == TRUE) {
				cairo_set_source_rgb (cr, 1, 0, 0); /* red */
				cairo_move_to (cr, (x + TAG_OFFSET) * cell_size, y * cell_size);
				cairo_line_to (cr, (x + 1) * cell_size, (y + 1.0 - TAG_OFFSET) * cell_size);
				cairo_stroke (cr);
			}
			if (hitori->board[x][y].tag2 == TRUE) {
				cairo_set_source_rgb (cr, 0, 1, 0); /* green */
				cairo_move_to (cr, x * cell_size, (y + 1.0 - TAG_OFFSET) * cell_size);
				cairo_line_to (cr, (x + 1.0 - TAG_OFFSET) * cell_size, y * cell_size);
				cairo_stroke (cr);
			}
		}
	}

	/* Check to see if all three rules are satisfied yet.
	 * If they are, we've won. */
	if (check_win &&
	    hitori_check_rule1 (hitori) &&
	    hitori_check_rule2 (hitori) &&
	    hitori_check_rule3 (hitori)) {
		/* Win! */
		g_message("TODO: win");
	}
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
	gint width, height;
	gfloat cell_size;
	guint x, y;
	cairo_t *cr;

	gdk_drawable_get_size (GDK_DRAWABLE (hitori->drawing_area->window), &width, &height);

	/* Clamp the width/height to the minimum */
	if (height < width)
		width = height;
	else if (width < height)
		height = width;

	cell_size = width / BOARD_SIZE;

	/* Determine the cell in which the button was released */
	x = floor (event->x / cell_size);
	y = floor (event->y / cell_size);

	if (x >= BOARD_SIZE || y >= BOARD_SIZE)
		return FALSE;

	if (event->state & GDK_SHIFT_MASK)
		hitori->board[x][y].tag1 = !(hitori->board[x][y].tag1); /* Update tag 1's state */
	else if (event->state & GDK_CONTROL_MASK)
		hitori->board[x][y].tag2 = !(hitori->board[x][y].tag2); /* Update tag 2's state */
	else
		hitori->board[x][y].painted = !(hitori->board[x][y].painted); /* Update the paint overlay */

	/* Redraw */
	cr = gdk_cairo_create (GDK_DRAWABLE (hitori->drawing_area->window));
	cairo_rectangle (cr, x * cell_size, y * cell_size,
				cell_size, cell_size);
	cairo_clip (cr);
	hitori_draw_board (hitori, cr, TRUE);
	cairo_destroy (cr);

	return FALSE;
}

void
hitori_destroy_cb (GtkWindow *window, Hitori *hitori)
{
	hitori_quit (hitori);
}

void
hitori_quit_cb (GtkAction *action, Hitori *hitori)
{
	hitori_quit (hitori);
}
