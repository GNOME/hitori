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

/* Callbacks for GtkBuilder */
gboolean hitori_expose_cb (GtkWidget *drawing_area, GdkEventExpose *event, Hitori *hitori);

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
hitori_draw_board (Hitori *hitori, cairo_t *cr)
{
	gint width, height;
	guint i, x;
	gfloat cell_size;
	cairo_text_extents_t extents;

	gdk_drawable_get_size (GDK_DRAWABLE (hitori->drawing_area->window), &width, &height);

	/* Clamp the width/height to the minimum */
	if (height < width)
		width = height;
	else if (width < height)
		height = width;

	cell_size = width / BOARD_SIZE;

	/* Draw the border rectangle */
	cairo_rectangle (cr, 0, 0, (gdouble) width, (gdouble) height);
	cairo_set_source_rgb (cr, 1, 1, 1); /* white */
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0, 0, 0); /* black */
	cairo_stroke (cr);

	i = 0;
	for (x = cell_size; x < width; x += cell_size) {
		if (i >= BOARD_SIZE - 1)
			break;

		/* Draw the vertical line */
		cairo_move_to (cr, x, 0);
		cairo_line_to (cr, x, height);
		cairo_stroke (cr);

		/* Draw the horizontal line */
		cairo_move_to (cr, 0, x);
		cairo_line_to (cr, width, x);
		cairo_stroke (cr);

		i++;
	}

	/* Draw the paint overlay */
	cairo_set_source_rgb (cr, 0.5, 0.5, 0.5); /* 50% grey */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (i = 0; i < BOARD_SIZE; i++) {
			if (hitori->paint_overlay[x][i] == FALSE)
				continue;

			cairo_rectangle (cr, x * cell_size, i * cell_size, cell_size, cell_size);
			cairo_fill (cr);
		}
	}

	/* Setup font rendering */
	cairo_set_font_size (cr, cell_size * FONT_SCALE);
	cairo_set_source_rgb (cr, 0, 0, 0); /* black */

	/* Draw the cell numbers */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (i = 1; i <= BOARD_SIZE; i++) {
			gchar *text;

			text = g_strdup_printf ("%u", hitori->board[x][i]);
			cairo_text_extents (cr, text, &extents);
			cairo_move_to (cr,
				x * cell_size + (cell_size - extents.width) / 2,
				i * cell_size - (cell_size - extents.height) / 2);
			cairo_show_text (cr, text);
			g_free (text);
		}
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

	hitori_draw_board (hitori, cr);

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

	/* Update the paint overlay */
	hitori->paint_overlay[x][y] = !(hitori->paint_overlay[x][y]);

	/* Redraw */
	cr = gdk_cairo_create (GDK_DRAWABLE (hitori->drawing_area->window));

	cairo_rectangle (cr, x * cell_size, y * cell_size,
				cell_size, cell_size);
	cairo_clip (cr);

	hitori_draw_board (hitori, cr);

	cairo_destroy (cr);

	return FALSE;
}
