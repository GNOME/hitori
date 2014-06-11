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

#define NORMAL_FONT_SCALE 0.9
#define PAINTED_FONT_SCALE 0.6
#define PAINTED_ALPHA 0.7
#define NORMAL_ALPHA 1.0
#define TAG_OFFSET 0.75
#define TAG_RADIUS 0.25
#define HINT_FLASHES 6
#define HINT_DISABLED 0
#define HINT_INTERVAL 500

static void hitori_cancel_hinting (Hitori *hitori);

/* Declarations for GtkBuilder */
gboolean hitori_draw_cb (GtkWidget *drawing_area, cairo_t *cr, Hitori *hitori);
gboolean hitori_button_release_cb (GtkWidget *drawing_area, GdkEventButton *event, Hitori *hitori);
void hitori_destroy_cb (GtkWindow *window, Hitori *hitori);
void hitori_window_state_event_cb (GtkWindow *window, GdkEventWindowState *event, Hitori *hitori);
static void new_game_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void hint_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void undo_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void redo_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void quit_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void help_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void about_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void board_size_activate_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void board_size_change_cb (GSimpleAction *action, GVariant *state, gpointer user_data);

static GActionEntry app_entries[] = {
	{ "new-game", new_game_cb, NULL, NULL, NULL },
	{ "about", about_cb, NULL, NULL, NULL },
	{ "help", help_cb, NULL, NULL, NULL },
	{ "quit", quit_cb, NULL, NULL, NULL },
};

static GActionEntry win_entries[] = {
	{ "board-size", board_size_activate_cb, "s", "'5'", board_size_change_cb },
	{ "hint", hint_cb, NULL, NULL, NULL },
	{ "undo", undo_cb, NULL, NULL, NULL },
	{ "redo", redo_cb, NULL, NULL, NULL },
};

GtkWidget *
hitori_create_interface (Hitori *hitori)
{
	GError *error = NULL;
	GtkBuilder *builder;
	GtkStyleContext *style_context;
	const PangoFontDescription *font;
	GMenuModel *app_menu, *win_menu;  /* owned */

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
	hitori->timer_label = GTK_LABEL (gtk_builder_get_object (builder, "hitori_timer"));

	/* Set up the menus (application and window). */
	app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "app_menu"));
	g_action_map_add_action_entries (G_ACTION_MAP (hitori), app_entries, G_N_ELEMENTS (app_entries), hitori);
	gtk_application_set_app_menu (GTK_APPLICATION (hitori), app_menu);
	g_object_unref (app_menu);

	win_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "win_menu"));
	g_action_map_add_action_entries (G_ACTION_MAP (hitori->window), win_entries, G_N_ELEMENTS (win_entries), hitori);
	gtk_application_set_menubar (GTK_APPLICATION (hitori), win_menu);
	g_object_unref (win_menu);

	hitori->undo_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (hitori->window), "undo"));
	hitori->redo_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (hitori->window), "redo"));
	hitori->hint_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (hitori->window), "hint"));

	g_object_unref (builder);

	/* Set up font descriptions for the drawing area */
	style_context = gtk_widget_get_style_context (hitori->drawing_area);
	gtk_style_context_get (style_context, 0, GTK_STYLE_PROPERTY_FONT, &font, NULL);
	hitori->normal_font_desc = pango_font_description_copy (font);
	hitori->painted_font_desc = pango_font_description_copy (font);

	/* Reset the timer */
	hitori_reset_timer (hitori);

	/* Disable undo/redo until a cell has been clicked. */
	g_simple_action_set_enabled (hitori->undo_action, FALSE);
	g_simple_action_set_enabled (hitori->redo_action, FALSE);

	/* Set the initial board size. */
	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (hitori->window), "board-size")), g_variant_new_string ("5"));

	return hitori->window;
}

#define BORDER_LEFT 2.0

static void
draw_cell (Hitori *hitori, GtkStyleContext *style_context, cairo_t *cr, gfloat cell_size, gdouble x_pos, gdouble y_pos,
           HitoriVector iter)
{
	gchar *text;
	PangoLayout *layout;
	gint text_width, text_height;
	GtkStateFlags state = 0;
	gboolean painted = FALSE;
	PangoFontDescription *font_desc;
	GdkRGBA colour;
	GtkBorder border;

	if (hitori->board[iter.x][iter.y].status & CELL_PAINTED) {
		painted = TRUE;
		state = GTK_STATE_FLAG_INSENSITIVE;
	}

	/* Set up the border */
	gtk_style_context_get_border (style_context, state, &border);
	border.left = BORDER_LEFT; /* Hack! */

	/* Draw the fill */
	if (hitori->debug == TRUE) {
		g_debug ("State: %u", state);
	}

	gtk_style_context_get_background_color (style_context, state, &colour);
	gdk_cairo_set_source_rgba (cr, &colour);
	cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
	cairo_fill (cr);

	/* If the cell is tagged, draw the tag dots */
	if (hitori->board[iter.x][iter.y].status & CELL_TAG1) {
		/* Tango's lightest "sky blue" */
		cairo_set_source_rgba (cr, 0.447058824, 0.623529412, 0.811764706, (painted == TRUE) ? PAINTED_ALPHA : NORMAL_ALPHA);

		cairo_move_to (cr, x_pos, y_pos + TAG_OFFSET);
		cairo_line_to (cr, x_pos, y_pos);
		cairo_line_to (cr, x_pos + TAG_OFFSET, y_pos);
		cairo_arc (cr, x_pos + TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.0, 0.5 * M_PI);
		cairo_fill (cr);
	}

	if (hitori->board[iter.x][iter.y].status & CELL_TAG2) {
		/* Tango's lightest "chameleon" */
		cairo_set_source_rgba (cr, 0.541176471, 0.88627451, 0.203921569, (painted == TRUE) ? PAINTED_ALPHA : NORMAL_ALPHA);

		cairo_move_to (cr, x_pos + cell_size - TAG_OFFSET, y_pos);
		cairo_line_to (cr, x_pos + cell_size, y_pos);
		cairo_line_to (cr, x_pos + cell_size, y_pos + TAG_OFFSET);
		cairo_arc (cr, x_pos + cell_size - TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.5 * M_PI, 1.0 * M_PI);
		cairo_fill (cr);
	}

	/* Draw the border */
	gtk_style_context_get_border_color (style_context, state, &colour);
	gdk_cairo_set_source_rgba (cr, &colour);
	cairo_set_line_width (cr, border.left);
	cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
	cairo_stroke (cr);

	/* Draw the text */
	text = g_strdup_printf ("%u", hitori->board[iter.x][iter.y].num);
	layout = pango_cairo_create_layout (cr);

	pango_layout_set_text (layout, text, -1);

	font_desc = (painted == TRUE) ? hitori->painted_font_desc : hitori->normal_font_desc;

	if (hitori->board[iter.x][iter.y].status & CELL_ERROR) {
		cairo_set_source_rgb (cr, 0.937254902, 0.160784314, 0.160784314); /* Tango's lightest "scarlet red" */
		pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
	} else {
		gtk_style_context_get_color (style_context, state, &colour);
		gdk_cairo_set_source_rgba (cr, &colour);
		pango_font_description_set_weight (font_desc, PANGO_WEIGHT_NORMAL);
	}

	pango_layout_set_font_description (layout, font_desc);

	pango_layout_get_pixel_size (layout, &text_width, &text_height);
	cairo_move_to (cr,
	               x_pos + (cell_size - text_width) / 2,
	               y_pos + (cell_size - text_height) / 2);

	pango_cairo_show_layout (cr, layout);

	g_free (text);
	g_object_unref (layout);
}

gboolean
hitori_draw_cb (GtkWidget *drawing_area, cairo_t *cr, Hitori *hitori)
{
	gint area_width, area_height;
	HitoriVector iter;
	guint board_width, board_height;
	gfloat cell_size;
	gdouble x_pos, y_pos;
	GtkStyleContext *style_context;

	area_width = gdk_window_get_width (gtk_widget_get_window (hitori->drawing_area));
	area_height = gdk_window_get_height (gtk_widget_get_window (hitori->drawing_area));
	style_context = gtk_widget_get_style_context (hitori->drawing_area);

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
	pango_font_description_set_absolute_size (hitori->normal_font_desc, cell_size * NORMAL_FONT_SCALE * 0.8 * PANGO_SCALE);
	pango_font_description_set_absolute_size (hitori->painted_font_desc, cell_size * PAINTED_FONT_SCALE * 0.8 * PANGO_SCALE);

	/* Centre the board */
	hitori->drawing_area_x_offset = (area_width - board_width) / 2;
	hitori->drawing_area_y_offset = (area_height - board_height) / 2;
	cairo_translate (cr, hitori->drawing_area_x_offset, hitori->drawing_area_y_offset);

	/* Draw the unpainted cells first. */
	for (iter.x = 0, x_pos = 0; iter.x < hitori->board_size; iter.x++, x_pos += cell_size) { /* columns (X) */
		for (iter.y = 0, y_pos = 0; iter.y < hitori->board_size; iter.y++, y_pos += cell_size) { /* rows (Y) */
			if (!(hitori->board[iter.x][iter.y].status & CELL_PAINTED)) {
				draw_cell (hitori, style_context, cr, cell_size, x_pos, y_pos, iter);
			}
		}
	}

	/* Next draw the painted cells (so that their borders are painted over those of the unpainted cells).. */
	for (iter.x = 0, x_pos = 0; iter.x < hitori->board_size; iter.x++, x_pos += cell_size) { /* columns (X) */
		for (iter.y = 0, y_pos = 0; iter.y < hitori->board_size; iter.y++, y_pos += cell_size) { /* rows (Y) */
			if (hitori->board[iter.x][iter.y].status & CELL_PAINTED) {
				draw_cell (hitori, style_context, cr, cell_size, x_pos, y_pos, iter);
			}
		}
	}

	/* Draw a hint if applicable */
	if (hitori->hint_status % 2 == 1) {
		gfloat line_width = BORDER_LEFT * 2.5;
		cairo_set_source_rgb (cr, 1, 0, 0); /* red */
		cairo_set_line_width (cr, line_width);
		cairo_rectangle (cr,
				 hitori->hint_position.x * cell_size + line_width / 2,
				 hitori->hint_position.y * cell_size + line_width / 2,
				 cell_size - line_width,
				 cell_size - line_width);
		cairo_stroke (cr);
	}

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

	cell_size = width / hitori->board_size;

	/* Determine the cell in which the button was released */
	pos.x = (guchar) ((event->x - hitori->drawing_area_x_offset) / cell_size);
	pos.y = (guchar) ((event->y - hitori->drawing_area_y_offset) / cell_size);

	if (pos.x >= hitori->board_size || pos.y >= hitori->board_size)
		return FALSE;

	/* Update the undo stack */
	undo = g_new (HitoriUndo, 1);
	undo->cell = pos;
	undo->undo = hitori->undo_stack;
	undo->redo = NULL;

	if (event->state & GDK_SHIFT_MASK && event->state & GDK_CONTROL_MASK) {
		/* Update both tags' state */
		hitori->board[pos.x][pos.y].status ^= CELL_TAG1;
		hitori->board[pos.x][pos.y].status ^= CELL_TAG2;
		undo->type = UNDO_TAGS;
	} else if (event->state & GDK_SHIFT_MASK) {
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

	if (hitori->undo_stack != NULL) {
		HitoriUndo *i, *next = NULL;

		/* Free the redo stack after this point. */
		for (i = hitori->undo_stack->redo; i != NULL; i = next) {
			next = i->redo;
			g_free (i);
		}

		hitori->undo_stack->redo = undo;
	}
	hitori->undo_stack = undo;
	g_simple_action_set_enabled (hitori->undo_action, TRUE);
	g_simple_action_set_enabled (hitori->redo_action, FALSE);

	/* Stop any current hints */
	hitori_cancel_hinting (hitori);

	/* Redraw */
	gtk_widget_queue_draw (hitori->drawing_area);

	/* Check to see if the player's won */
	if (recheck == TRUE)
		hitori_check_win (hitori);

	return FALSE;
}

void
hitori_destroy_cb (GtkWindow *window, Hitori *hitori)
{
	hitori_quit (hitori);
}

void
hitori_window_state_event_cb (GtkWindow *window, GdkEventWindowState *event, Hitori *hitori)
{
	gboolean timer_was_running = FALSE;

	if (hitori->debug)
		g_debug ("Got window state event: %u (changed: %u)", event->new_window_state, event->changed_mask);

	timer_was_running = (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (window), "hitori-timer-was-running")) > 0) ? TRUE : FALSE;

	if (!(event->new_window_state & GDK_WINDOW_STATE_FOCUSED)) {
		/* Pause the timer */
		if (hitori->timeout_id > 0) {
			g_object_set_data (G_OBJECT (window), "hitori-timer-was-running", GUINT_TO_POINTER ((hitori->timeout_id > 0) ? TRUE : FALSE));
		}

		hitori_pause_timer (hitori);
	} else if (timer_was_running == TRUE) {
		/* Re-start the timer */
		hitori_start_timer (hitori);
	}
}

static void
new_game_cb (GSimpleAction *action, GVariant *parameters, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	hitori_new_game (self, self->board_size);
}

static void
hitori_cancel_hinting (Hitori *hitori)
{
	if (hitori->debug)
		g_debug ("Stopping all current hints.");

	hitori->hint_status = HINT_DISABLED;
	if (hitori->hint_timeout_id != 0)
		g_source_remove (hitori->hint_timeout_id);
	hitori->hint_timeout_id = 0;
}

static gboolean
hitori_update_hint (Hitori *hitori)
{
	gint area_width, area_height;
	guint board_width, board_height, drawing_area_x_offset, drawing_area_y_offset;
	gfloat cell_size;

	/* Check to see if hinting's been stopped by a cell being changed (race condition) */
	if (hitori->hint_status == HINT_DISABLED)
		return FALSE;

	hitori->hint_status--;

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

	drawing_area_x_offset = (area_width - board_width) / 2;
	drawing_area_y_offset = (area_height - board_height) / 2;

	/* Redraw the cell */
	gtk_widget_queue_draw_area (hitori->drawing_area, drawing_area_x_offset + hitori->hint_position.x * cell_size,
	                            drawing_area_y_offset + hitori->hint_position.y * cell_size, cell_size, cell_size);

	if (hitori->hint_status == HINT_DISABLED) {
		hitori_cancel_hinting (hitori);
		return FALSE;
	}

	return TRUE;
}

static void
hint_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	HitoriVector iter;

	/* Bail if we're already hinting */
	if (self->hint_status != HINT_DISABLED)
		return;

	/* Find the first cell which should be painted, but isn't (or vice-versa) */
	for (iter.x = 0; iter.x < self->board_size; iter.x++) {
		for (iter.y = 0; iter.y < self->board_size; iter.y++) {
			guchar status = self->board[iter.x][iter.y].status & (CELL_PAINTED | CELL_SHOULD_BE_PAINTED);

			if (status <= MAX (CELL_SHOULD_BE_PAINTED, CELL_PAINTED) &&
			    status > 0) {
				if (self->debug)
					g_debug ("Beginning hinting in cell (%u,%u).", iter.x, iter.y);

				/* Set up the cell for hinting */
				self->hint_status = HINT_FLASHES;
				self->hint_position = iter;
				self->hint_timeout_id = g_timeout_add (HINT_INTERVAL, (GSourceFunc) hitori_update_hint, self);
				hitori_update_hint ((gpointer) self);

				return;
			}
		}
	}
}

static void
undo_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);

	if (self->undo_stack->undo == NULL)
		return;

	switch (self->undo_stack->type) {
		case UNDO_PAINT:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_PAINTED;
			break;
		case UNDO_TAG1:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG1;
			break;
		case UNDO_TAG2:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG2;
			break;
		case UNDO_TAGS:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG1;
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG2;
			break;
		case UNDO_NEW_GAME:
		default:
			/* This is just here to stop the compiler warning */
			g_assert_not_reached ();
			break;
	}

	self->undo_stack = self->undo_stack->undo;

	g_simple_action_set_enabled (self->redo_action, TRUE);
	if (self->undo_stack->undo == NULL || self->undo_stack->type == UNDO_NEW_GAME)
		g_simple_action_set_enabled (self->undo_action, FALSE);

	/* The player can't possibly have won, but we need to update the error highlighting */
	hitori_check_win (self);

	/* Redraw */
	gtk_widget_queue_draw (self->drawing_area);
}

static void
redo_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);

	if (self->undo_stack->redo == NULL)
		return;

	self->undo_stack = self->undo_stack->redo;

	switch (self->undo_stack->type) {
		case UNDO_PAINT:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_PAINTED;
			break;
		case UNDO_TAG1:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG1;
			break;
		case UNDO_TAG2:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG2;
			break;
		case UNDO_TAGS:
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG1;
			self->board[self->undo_stack->cell.x][self->undo_stack->cell.y].status ^= CELL_TAG2;
			break;
		case UNDO_NEW_GAME:
		default:
			/* This is just here to stop the compiler warning */
			g_assert_not_reached ();
			break;
	}

	g_simple_action_set_enabled (self->undo_action, TRUE);
	if (self->undo_stack->redo == NULL)
		g_simple_action_set_enabled (self->redo_action, FALSE);

	/* The player can't possibly have won, but we need to update the error highlighting */
	hitori_check_win (self);

	/* Redraw */
	gtk_widget_queue_draw (self->drawing_area);
}

static void
quit_cb (GSimpleAction *action, GVariant *parameters, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	hitori_quit (self);
}

static void
help_cb (GSimpleAction *action, GVariant *parameters, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	GError *error = NULL;

	if (gtk_show_uri (gtk_widget_get_screen (self->window), "help:hitori", gtk_get_current_event_time (), &error) == FALSE) {
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (self->window),
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

static void
about_cb (GSimpleAction *action, GVariant *parameters, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
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

	gtk_show_about_dialog (GTK_WINDOW (self->window),
				"version", VERSION,
				"copyright", _("Copyright \xc2\xa9 2007\342\200\2232010 Philip Withnall"),
				"comments", _("A logic puzzle designed by Nikoli."),
				"authors", authors,
				"translator-credits", _("translator-credits"),
				"logo-icon-name", "hitori",
				"license", license,
				"wrap-license", TRUE,
				"website-label", _("Hitori Website"),
				"website", PACKAGE_URL,
				NULL);

	g_free (license);
}

static void
board_size_activate_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	g_action_change_state (G_ACTION (action), parameter);
}

static void
board_size_change_cb (GSimpleAction *action, GVariant *state, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	const gchar *size_str;
	guint64 size;

	size_str = g_variant_get_string (state, NULL);
	size = g_ascii_strtoull (size_str, NULL, 10);
	hitori_set_board_size (self, size);

	g_simple_action_set_state (action, state);
}
