/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Hitori
 * Copyright (C) Philip Withnall 2007-2009 <philip@tecnocode.co.uk>
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
#include <cairo/cairo.h>
#include <math.h>
#include <glib/gi18n.h>

#include "config.h"
#include "main.h"
#include "interface.h"
#include "rules.h"

#define FONT_SCALE 0.9
#define TAG_OFFSET 0.75
#define TAG_RADIUS 0.25
#define HINT_FLASHES 6
#define HINT_INTERVAL 500

/* Declarations for GtkBuilder */
gboolean hitori_draw_cb (GtkWidget *drawing_area, cairo_t *cr, Hitori *hitori);
gboolean hitori_button_release_cb (GtkWidget *drawing_area, GdkEventButton *event, Hitori *hitori);
void hitori_destroy_cb (GtkWindow *window, Hitori *hitori);
void hitori_new_game_cb (GtkAction *action, Hitori *hitori);
void hitori_hint_cb (GtkAction *action, Hitori *hitori);
void hitori_undo_cb (GtkAction *action, Hitori *hitori);
void hitori_redo_cb (GtkAction *action, Hitori *hitori);
void hitori_quit_cb (GtkAction *action, Hitori *hitori);
void hitori_contents_cb (GtkAction *action, Hitori *hitori);
void hitori_about_cb (GtkAction *action, Hitori *hitori);
void hitori_board_size_cb (GtkRadioAction *action, GtkRadioAction *current, Hitori *hitori);

GtkWidget *
hitori_create_interface (Hitori *hitori)
{
	GError *error = NULL;
	GtkBuilder *builder;

	builder = gtk_builder_new ();

	if (gtk_builder_add_from_file (builder, PACKAGE_DATA_DIR"/hitori/hitori.ui", &error) == FALSE &&
	    gtk_builder_add_from_file (builder, "./data/hitori.ui", NULL) == FALSE) {
		/* Show an error */
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("UI file “%s” could not be loaded"), PACKAGE_DATA_DIR"/hitori/hitori.ui");
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

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
	HitoriVector iter;
	guint board_width, board_height;
	gfloat cell_size;
	gdouble x_pos, y_pos;
	GtkStyle *style;
	gboolean has_won = FALSE;

	/* Check to see if all three rules are satisfied yet.
	 * If they are, we've won.
	 * NOTE: We check rule 1 last, as it's the only rule
	 * which won't set an error position. */
	if (check_win &&
	    hitori_check_rule2 (hitori) &&
	    hitori_check_rule3 (hitori) &&
	    hitori_check_rule1 (hitori)) {
		/* Win! (Tell them later, once we've re-rendered the winning board) */
		has_won = TRUE;
	}

	area_width = gdk_window_get_width (gtk_widget_get_window (hitori->drawing_area));
	area_height = gdk_window_get_height (gtk_widget_get_window (hitori->drawing_area));
	style = gtk_widget_get_style (hitori->drawing_area);

	/* Clamp the width/height to the minimum */
	if (area_height < area_width) {
		board_width = area_height;
		board_height = area_height;
	} else {
		board_width = area_width;
		board_height = area_width;
	}

	/* Work out the cell size and scale all text accordingly */
	cell_size = board_width / hitori->board_size;
	pango_font_description_set_absolute_size (style->font_desc, cell_size * FONT_SCALE * 0.8 * PANGO_SCALE);

	/* Centre the board */
	hitori->drawing_area_x_offset = (area_width - board_width) / 2;
	hitori->drawing_area_y_offset = (area_height - board_height) / 2;
	cairo_translate (cr, hitori->drawing_area_x_offset, hitori->drawing_area_y_offset);

	/* Draw the cells */
	x_pos = 0;
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) { /* columns (X) */
		y_pos = 0;
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) { /* rows (Y) */
			gchar *text;
			PangoLayout *layout;
			gint text_width, text_height;
			GtkStateType state = GTK_STATE_NORMAL;

			if (hitori->board[iter.x][iter.y].status & CELL_PAINTED)
				state = GTK_STATE_INSENSITIVE;

			/* Draw the fill */
			if (hitori->display_error == TRUE &&
			    hitori->error_position.x == iter.x && hitori->error_position.y == iter.y)
				cairo_set_source_rgb (cr, 0.678431373, 0.498039216, 0.658823529); /* Tango's lightest "plum" */
			else
				gdk_cairo_set_source_color (cr, &style->bg[state]);
			cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
			cairo_fill (cr);

			/* If the cell is tagged, draw the tag dots */
			if (hitori->board[iter.x][iter.y].status & CELL_TAG1) {
				if (hitori->board[iter.x][iter.y].status & CELL_PAINTED)
					cairo_set_source_rgb (cr, 0.643137255, 0, 0); /* Tango's darkest "scarlet red" */				
				else
					cairo_set_source_rgb (cr, 0.937254902, 0.160784314, 0.160784314); /* Tango's lightest "scarlet red" */

				cairo_move_to (cr, x_pos, y_pos + TAG_OFFSET);
				cairo_line_to (cr, x_pos, y_pos);
				cairo_line_to (cr, x_pos + TAG_OFFSET, y_pos);
				cairo_arc (cr, x_pos + TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.0, 0.5 * M_PI);
				cairo_fill (cr);
			}

			if (hitori->board[iter.x][iter.y].status & CELL_TAG2) {
				if (hitori->board[iter.x][iter.y].status & CELL_PAINTED)
					cairo_set_source_rgb (cr, 0.305882353, 0.603921569, 0.023529412); /* Tango's darkest "chameleon" */
				else
					cairo_set_source_rgb (cr, 0.541176471, 0.88627451, 0.203921569); /* Tango's lightest "chameleon" */

				cairo_move_to (cr, x_pos + cell_size - TAG_OFFSET, y_pos);
				cairo_line_to (cr, x_pos + cell_size, y_pos);
				cairo_line_to (cr, x_pos + cell_size, y_pos + TAG_OFFSET);
				cairo_arc (cr, x_pos + cell_size - TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.5 * M_PI, 1.0 * M_PI);
				cairo_fill (cr);
			}

			/* Draw the border */
			gdk_cairo_set_source_color (cr, &style->dark[state]);
			cairo_set_line_width (cr, style->xthickness);
			cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
			cairo_stroke (cr);

			/* Draw the text */
			text = g_strdup_printf ("%u", hitori->board[iter.x][iter.y].num);
			layout = pango_cairo_create_layout (cr);

			pango_layout_set_text (layout, text, -1);
			pango_layout_set_font_description (layout, style->font_desc);

			pango_layout_get_pixel_size (layout, &text_width, &text_height);
			cairo_move_to (cr,
				       x_pos + (cell_size - text_width) / 2,
				       y_pos + (cell_size - text_height) / 2);

			gdk_cairo_set_source_color (cr, &style->text[state]);
			pango_cairo_show_layout (cr, layout);

			g_free (text);
			g_object_unref (layout);

			y_pos += cell_size;
		}

		x_pos += cell_size;
	}

	if (has_won == TRUE) {
		GtkWidget *dialog;

		/* Tell the user they've won, and don't draw any hints */
		hitori_disable_events (hitori);
		dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
						 _("You've won!"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}

	/* Draw a hint if applicable */
	if (hitori->hint_status % 2 == 1) {
		cairo_set_source_rgb (cr, 1, 0, 0); /* red */
		cairo_rectangle (cr, hitori->hint_position.x * cell_size, hitori->hint_position.y * cell_size, cell_size, cell_size);
		cairo_stroke (cr);
	}
}

/**
 * hitori_draw_board_simple:
 * @hitori: a #Hitori
 * @check_win: %TRUE if the win status should be checked after drawing
 * @clear_first: %TRUE if the drawing area should be cleared before re-drawing
 *
 * Allows easy re-drawing of the board. If @check_win is %TRUE, the board will
 * be checked to see if the player's won after it's been drawn, and if @clear_first
 * is %TRUE, the drawing area will be completely cleared before being drawn.
 * @clear_first is intended to be used only if there's a possibility the size of the
 * board has changed.
 **/
void
hitori_draw_board_simple (Hitori *hitori, gboolean check_win, gboolean clear_first)
{
	cairo_t *cr;
	GdkWindow *window;

	window = gtk_widget_get_window (hitori->drawing_area);
	cr = gdk_cairo_create (GDK_DRAWABLE (window));

	if (clear_first) {
		gint width, height;
		GtkStyle *style;

		width = gdk_window_get_width (window);
		height = gdk_window_get_height (window);
		style = gtk_widget_get_style (hitori->drawing_area);

		cairo_set_source (cr, style->background[GTK_STATE_NORMAL]);
		cairo_rectangle (cr, 0.0, 0.0, width, height);
		cairo_paint (cr);
	}

	hitori_draw_board (hitori, cr, check_win);
	cairo_destroy (cr);
}

gboolean
hitori_draw_cb (GtkWidget *drawing_area, cairo_t *cr, Hitori *hitori)
{
	hitori_draw_board (hitori, cr, FALSE);

	return FALSE;
}

gboolean
hitori_button_release_cb (GtkWidget *drawing_area, GdkEventButton *event, Hitori *hitori)
{
	gint width, height;
	gfloat cell_size;
	HitoriVector pos;
	HitoriUndo *undo;
	gboolean recheck = FALSE;

	if (hitori->processing_events == FALSE)
		return FALSE;

	width = gdk_window_get_width (gtk_widget_get_window (hitori->drawing_area));
	height = gdk_window_get_height (gtk_widget_get_window (hitori->drawing_area));

	/* Clamp the width/height to the minimum */
	if (height < width)
		width = height;
	else if (width < height)
		height = width;

	cell_size = width / hitori->board_size;

	/* Determine the cell in which the button was released */
	pos.x = floor ((event->x - hitori->drawing_area_x_offset) / cell_size);
	pos.y = floor ((event->y - hitori->drawing_area_y_offset) / cell_size);

	if (pos.x >= hitori->board_size || pos.y >= hitori->board_size)
		return FALSE;

	/* Update the undo stack */
	undo = g_new (HitoriUndo, 1);
	undo->cell = pos;
	undo->undo = hitori->undo_stack;
	undo->redo = NULL;

	if (event->state & GDK_SHIFT_MASK) {
		/* Update tag 1's state */
		hitori->board[pos.x][pos.y].status ^= CELL_TAG1;
		undo->type = UNDO_TAG1;
	} else if (event->state & GDK_CONTROL_MASK) {
		/* Update tag 2's state */
		hitori->board[pos.x][pos.y].status ^= CELL_TAG2;
		undo->type = UNDO_TAG2;
	} else {
		/* Update the paint overlay */
		hitori->board[pos.x][pos.y].status ^= CELL_PAINTED;
		undo->type = UNDO_PAINT;
		recheck = TRUE;
	}

	hitori->made_a_move = TRUE;

	if (hitori->undo_stack != NULL)
		hitori->undo_stack->redo = undo;
	hitori->undo_stack = undo;
	gtk_action_set_sensitive (hitori->undo_action, TRUE);

	/* Stop any current hints */
	hitori->hint_status = HINT_FLASHES;

	if (hitori->debug)
		g_debug ("Stopping all current hints.");

	/* Redraw */
	hitori_draw_board_simple (hitori, recheck, FALSE);

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
	hitori_new_game (hitori, hitori->board_size);
}

static gboolean
hitori_update_hint (Hitori *hitori)
{
	cairo_t *cr;
	gint area_width, area_height;
	guint board_width, board_height;
	gfloat cell_size;

	/* Check to see if hinting's been stopped by a cell being changed (race condition) */
	if (hitori->hint_status >= HINT_FLASHES)
		return FALSE;

	hitori->hint_status++;

	if (hitori->debug)
		g_debug ("Updating hint status to %u.", hitori->hint_status);

	/* Calculate the area to redraw (just the hinted cell, hopefully) */
	area_width = gdk_window_get_width (gtk_widget_get_window (hitori->drawing_area));
	area_height = gdk_window_get_height (gtk_widget_get_window (hitori->drawing_area));

	/* Clamp the width/height to the minimum */
	if (area_height < area_width) {
		board_width = area_height;
		board_height = area_height;
	} else {
		board_width = area_width;
		board_height = area_width;
	}

	cell_size = board_width / hitori->board_size;

	/* Centre the board */
	cr = gdk_cairo_create (GDK_DRAWABLE (gtk_widget_get_window (hitori->drawing_area)));
	cairo_save (cr);

	hitori->drawing_area_x_offset = (area_width - board_width) / 2;
	hitori->drawing_area_y_offset = (area_height - board_height) / 2;
	cairo_translate (cr, hitori->drawing_area_x_offset, hitori->drawing_area_y_offset);

	/* Clip to the cell */
	cairo_rectangle (cr, hitori->hint_position.x * cell_size, hitori->hint_position.y * cell_size, cell_size, cell_size);
	cairo_clip (cr);
	cairo_restore (cr);

	hitori_draw_board (hitori, cr, FALSE);
	cairo_destroy (cr);

	return (hitori->hint_status < HINT_FLASHES) ? TRUE : FALSE;
}

void
hitori_hint_cb (GtkAction *action, Hitori *hitori)
{
	HitoriVector iter;

	/* Find the first cell which should be painted, but isn't (or vice-versa) */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			guchar status = hitori->board[iter.x][iter.y].status & (CELL_PAINTED | CELL_SHOULD_BE_PAINTED);

			if (status <= MAX (CELL_SHOULD_BE_PAINTED, CELL_PAINTED) &&
			    status > 0) {
				if (hitori->debug)
					g_debug ("Beginning hinting in cell (%u,%u).", iter.x, iter.y);

				/* Set up the cell for hinting */
				hitori->hint_status = 0;
				hitori->hint_position = iter;
				g_timeout_add (HINT_INTERVAL, (GSourceFunc) hitori_update_hint, hitori);
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
			hitori->board[hitori->undo_stack->cell.x][hitori->undo_stack->cell.y].status ^= CELL_PAINTED;
			break;
		case UNDO_TAG1:
			hitori->board[hitori->undo_stack->cell.x][hitori->undo_stack->cell.y].status ^= CELL_TAG1;
			break;
		case UNDO_TAG2:
			hitori->board[hitori->undo_stack->cell.x][hitori->undo_stack->cell.y].status ^= CELL_TAG2;
			break;
		case UNDO_NEW_GAME:
		default:
			/* This is just here to stop the compiler warning */
			g_assert_not_reached ();
			break;
	}

	hitori->undo_stack = hitori->undo_stack->undo;

	gtk_action_set_sensitive (hitori->redo_action, TRUE);
	if (hitori->undo_stack->undo == NULL || hitori->undo_stack->type == UNDO_NEW_GAME)
		gtk_action_set_sensitive (hitori->undo_action, FALSE);

	/* Redraw */
	hitori_draw_board_simple (hitori, TRUE, FALSE);
}

void
hitori_redo_cb (GtkAction *action, Hitori *hitori)
{
	if (hitori->undo_stack->redo == NULL)
		return;

	hitori->undo_stack = hitori->undo_stack->redo;

	switch (hitori->undo_stack->type) {
		case UNDO_PAINT:
			hitori->board[hitori->undo_stack->cell.x][hitori->undo_stack->cell.y].status ^= CELL_PAINTED;
			break;
		case UNDO_TAG1:
			hitori->board[hitori->undo_stack->cell.x][hitori->undo_stack->cell.y].status ^= CELL_TAG1;
			break;
		case UNDO_TAG2:
			hitori->board[hitori->undo_stack->cell.x][hitori->undo_stack->cell.y].status ^= CELL_TAG2;
			break;
		case UNDO_NEW_GAME:
		default:
			/* This is just here to stop the compiler warning */
			g_assert_not_reached ();
			break;
	}

	gtk_action_set_sensitive (hitori->undo_action, TRUE);
	if (hitori->undo_stack->redo == NULL)
		gtk_action_set_sensitive (hitori->redo_action, FALSE);

	/* Redraw */
	hitori_draw_board_simple (hitori, TRUE, FALSE);
}

void
hitori_quit_cb (GtkAction *action, Hitori *hitori)
{
	hitori_quit (hitori);
}

void
hitori_contents_cb (GtkAction *action, Hitori *hitori)
{
	GError *error = NULL;

	if (gtk_show_uri (gtk_widget_get_screen (hitori->window), "ghelp:hitori", gtk_get_current_event_time (), &error) == FALSE) {
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
							    GTK_DIALOG_MODAL,
							    GTK_MESSAGE_ERROR,
							    GTK_BUTTONS_OK,
							    _("The help contents could not be displayed"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);

		gtk_dialog_run (GTK_DIALOG (dialog));

		gtk_widget_destroy (dialog);
		g_error_free (error);
	}
}

void
hitori_about_cb (GtkAction *action, Hitori *hitori)
{
	gchar *license;
	const gchar *authors[] =
	{
		"Philip Withnall <philip@tecnocode.co.uk>",
		"Ben Windsor <benjw_823@hotmail.com>",
		NULL
	};
	const gchar *license_parts[] = {
		N_("Hitori is free software: you can redistribute it and/or modify "
		   "it under the terms of the GNU General Public License as published by "
		   "the Free Software Foundation, either version 3 of the License, or "
		   "(at your option) any later version."),
		N_("Hitori is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		   "along with Hitori.  If not, see <http://www.gnu.org/licenses/>.")
	};

	license = g_strjoin ("\n\n",
			  _(license_parts[0]),
			  _(license_parts[1]),
			  _(license_parts[2]),
			  NULL);

	gtk_show_about_dialog (GTK_WINDOW (hitori->window),
				"version", VERSION,
				"copyright", _("Copyright \xc2\xa9 2007\342\200\2232010 Philip Withnall"),
				"comments", _("A logic puzzle designed by Nikoli."),
				"authors", authors,
				"translator-credits", _("translator-credits"),
				"logo-icon-name", "hitori",
				"license", license,
				"wrap-license", TRUE,
				"website-label", _("Hitori Website"),
				"website", "http://live.gnome.org/Hitori",
				NULL);

	g_free (license);
}

void
hitori_board_size_cb (GtkRadioAction *action, GtkRadioAction *current, Hitori *hitori)
{
	hitori_set_board_size (hitori, gtk_radio_action_get_current_value (current));
}
