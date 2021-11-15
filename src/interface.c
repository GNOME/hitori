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
#define TAG_OFFSET 0.75
#define TAG_RADIUS 0.25
#define HINT_FLASHES 6
#define HINT_DISABLED 0
#define HINT_INTERVAL 500
#define CURSOR_MARGIN 3

static void hitori_cancel_hinting (Hitori *hitori);

/* Declarations for GtkBuilder */
gboolean hitori_draw_cb (GtkWidget *drawing_area, cairo_t *cr, Hitori *hitori);
gboolean hitori_button_release_cb (GtkWidget *drawing_area, GdkEventButton *event, Hitori *hitori);
gboolean hitori_key_press_cb (GtkWidget *drawing_area, GdkEventKey *event, Hitori *hitori);
void hitori_destroy_cb (GtkWindow *window, Hitori *hitori);
void hitori_window_state_event_cb (GtkWindow *window, GdkEventWindowState *event, Hitori *hitori);
static void new_game_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void hint_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void quit_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void undo_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void redo_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void help_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void about_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void board_size_change_cb (GObject *object, GParamSpec *pspec, gpointer user_data);

static GActionEntry app_entries[] = {
	{ "new-game", new_game_cb, NULL, NULL, NULL },
	{ "about", about_cb, NULL, NULL, NULL },
	{ "help", help_cb, NULL, NULL, NULL },
	{ "quit", quit_cb, NULL, NULL, NULL },
};

static GActionEntry win_entries[] = {
	{ "hint", hint_cb, NULL, NULL, NULL },
	{ "undo", undo_cb, NULL, NULL, NULL },
	{ "redo", redo_cb, NULL, NULL, NULL },
};

static void
hitori_window_unmap_cb (GtkWidget *window,
                        gpointer user_data)
{
	gboolean window_maximized;
	GdkRectangle geometry;
	HitoriApplication *hitori;

	hitori = HITORI_APPLICATION (user_data);

	window_maximized = gtk_window_is_maximized (GTK_WINDOW (window));
	g_settings_set_boolean (hitori->settings,
				"window-maximized", window_maximized);

	if (window_maximized)
		return;

	gtk_window_get_position (GTK_WINDOW (window), &geometry.x, &geometry.y);
	gtk_window_get_size (GTK_WINDOW (window),
			     &geometry.width, &geometry.height);

	g_settings_set (hitori->settings, "window-position", "(ii)",
			geometry.x, geometry.y);
	g_settings_set (hitori->settings, "window-size", "(ii)",
			geometry.width, geometry.height);
}

GtkWidget *
hitori_create_interface (Hitori *hitori)
{
	GtkBuilder *builder;
	GtkStyleContext *style_context;
	GtkCssProvider *css_provider;
	PangoFontDescription *font = NULL;
	GAction *action;

	builder = gtk_builder_new_from_resource ("/org/gnome/Hitori/ui/hitori.ui");

	gtk_builder_set_translation_domain (builder, PACKAGE);
	gtk_builder_connect_signals (builder, hitori);

	/* Setup the main window */
	hitori->window = GTK_WIDGET (gtk_builder_get_object (builder, "hitori_main_window"));
	hitori->drawing_area = GTK_WIDGET (gtk_builder_get_object (builder, "hitori_drawing_area"));
	hitori->timer_label = GTK_LABEL (gtk_builder_get_object (builder, "hitori_timer"));

	g_signal_connect (hitori->window, "unmap",
			  G_CALLBACK (hitori_window_unmap_cb), hitori);

	g_object_unref (builder);

	/* Set up actions */
	g_action_map_add_action_entries (G_ACTION_MAP (hitori), app_entries, G_N_ELEMENTS (app_entries), hitori);
	g_action_map_add_action_entries (G_ACTION_MAP (hitori->window), win_entries, G_N_ELEMENTS (win_entries), hitori);

	action = g_settings_create_action (hitori->settings, "board-size");
	g_action_map_add_action (G_ACTION_MAP (hitori), action);
	g_signal_connect (G_OBJECT (action), "notify::state", (GCallback) board_size_change_cb, hitori);
	g_object_unref (action);

	hitori->undo_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (hitori->window), "undo"));
	hitori->redo_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (hitori->window), "redo"));
	hitori->hint_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (hitori->window), "hint"));

	const gchar *vaccels_help[] = {"F1", NULL};
	const gchar *vaccels_hint[] = {"<Primary>h", NULL};
	const gchar *vaccels_new[] = {"<Primary>n", NULL};
	const gchar *vaccels_quit[] = {"<Primary>q", "<Primary>w", NULL};
	const gchar *vaccels_redo[] = {"<Primary><Shift>z", NULL};
	const gchar *vaccels_undo[] = {"<Primary>z", NULL};

	gtk_application_set_accels_for_action (GTK_APPLICATION (hitori), "app.help", vaccels_help);
	gtk_application_set_accels_for_action (GTK_APPLICATION (hitori), "win.hint", vaccels_hint);
	gtk_application_set_accels_for_action (GTK_APPLICATION (hitori), "app.new-game", vaccels_new);
	gtk_application_set_accels_for_action (GTK_APPLICATION (hitori), "app.quit", vaccels_quit);
	gtk_application_set_accels_for_action (GTK_APPLICATION (hitori), "win.redo", vaccels_redo);
	gtk_application_set_accels_for_action (GTK_APPLICATION (hitori), "win.undo", vaccels_undo);

	/* Set up font descriptions for the drawing area */
	style_context = gtk_widget_get_style_context (hitori->drawing_area);
	gtk_style_context_get (style_context,
	                       gtk_style_context_get_state (style_context),
	                       GTK_STYLE_PROPERTY_FONT, &font, NULL);
	hitori->normal_font_desc = pango_font_description_copy (font);
	hitori->painted_font_desc = pango_font_description_copy (font);

	pango_font_description_free (font);

	/* Load CSS for the drawing area */
	css_provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_resource (css_provider, "/org/gnome/Hitori/ui/hitori.css");
	gtk_style_context_add_provider (style_context, GTK_STYLE_PROVIDER (css_provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref (css_provider);

	/* Reset the timer */
	hitori_reset_timer (hitori);

	/* Disable undo/redo until a cell has been clicked. */
	g_simple_action_set_enabled (hitori->undo_action, FALSE);
	g_simple_action_set_enabled (hitori->redo_action, FALSE);

	/* Cursor is initially not active as playing with the mouse is more common */
	hitori->cursor_active = FALSE;

	return hitori->window;
}

#define BORDER_LEFT 2.0

static void
lookup_color (GtkStyleContext *style_context, const gchar *name, GdkRGBA *result)
{
	if (!gtk_style_context_lookup_color (style_context, name, result)) {
		g_warning ("Failed to look up color %s", name);
	}
}

/* Generate the text for a given cell, potentially localised to the current locale. */
static const gchar *
localise_cell_digit (Hitori             *hitori,
                     const HitoriVector *pos)
{
	guchar value = hitori->board[pos->x][pos->y].num;

	G_STATIC_ASSERT (MAX_BOARD_SIZE < 11);

	switch (value) {
	/* Translators: This is a digit rendered in a cell on the game board.
	 * Translate it to your locale’s number system if you wish the game
	 * board to be rendered in those digits. Otherwise, leave the digits as
	 * Arabic numerals. */
	case 1: return C_("Board cell", "1");
	case 2: return C_("Board cell", "2");
	case 3: return C_("Board cell", "3");
	case 4: return C_("Board cell", "4");
	case 5: return C_("Board cell", "5");
	case 6: return C_("Board cell", "6");
	case 7: return C_("Board cell", "7");
	case 8: return C_("Board cell", "8");
	case 9: return C_("Board cell", "9");
	case 10: return C_("Board cell", "10");
	case 11: return C_("Board cell", "11");
	default:
		g_assert_not_reached ();
	}
}

static void
draw_cell (Hitori *hitori, GtkStyleContext *style_context, cairo_t *cr, gdouble cell_size, gdouble x_pos, gdouble y_pos,
           HitoriVector iter)
{
	const gchar *text;
	PangoLayout *layout;
	gint text_width, text_height;
	GtkStateFlags state;
	gboolean painted = FALSE;
	PangoFontDescription *font_desc;
	GtkBorder border;
	GdkRGBA colour = {0.0, 0.0, 0.0, 0.0};

	state = gtk_style_context_get_state (style_context);

	if (hitori->board[iter.x][iter.y].status & CELL_PAINTED) {
		painted = TRUE;
		state = GTK_STATE_FLAG_INSENSITIVE;
		gtk_style_context_set_state (style_context, state);
	}

	/* Set up the border */
	gtk_style_context_get_border (style_context, state, &border);
	border.left = BORDER_LEFT; /* Hack! */

	/* Draw the fill */
	if (hitori->debug == TRUE) {
		g_debug ("State: %u", state);
	}

	if (painted) {
		lookup_color (style_context, "painted-bg-color", &colour);
	} else {
		lookup_color (style_context, "unpainted-bg-color", &colour);
	}

	gdk_cairo_set_source_rgba (cr, &colour);
	cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
	cairo_fill (cr);

	/* If the cell is tagged, draw the tag dots */
	if (hitori->board[iter.x][iter.y].status & CELL_TAG1) {
		if (painted) {
			lookup_color (style_context, "tag1-painted-color", &colour);
		} else {
			lookup_color (style_context, "tag1-unpainted-color", &colour);
		}
		gdk_cairo_set_source_rgba (cr, &colour);

		cairo_move_to (cr, x_pos, y_pos + TAG_OFFSET);
		cairo_line_to (cr, x_pos, y_pos);
		cairo_line_to (cr, x_pos + TAG_OFFSET, y_pos);
		cairo_arc (cr, x_pos + TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.0, 0.5 * M_PI);
		cairo_fill (cr);
	}

	if (hitori->board[iter.x][iter.y].status & CELL_TAG2) {
		if (painted) {
			lookup_color (style_context, "tag2-painted-color", &colour);
		} else {
			lookup_color (style_context, "tag2-unpainted-color", &colour);
		}
		gdk_cairo_set_source_rgba (cr, &colour);

		cairo_move_to (cr, x_pos + cell_size - TAG_OFFSET, y_pos);
		cairo_line_to (cr, x_pos + cell_size, y_pos);
		cairo_line_to (cr, x_pos + cell_size, y_pos + TAG_OFFSET);
		cairo_arc (cr, x_pos + cell_size - TAG_OFFSET, y_pos + TAG_OFFSET, TAG_RADIUS * cell_size, 0.5 * M_PI, 1.0 * M_PI);
		cairo_fill (cr);
	}

	/* Draw the border */
	if (painted) {
		lookup_color (style_context, "painted-border-color", &colour);
	} else {
		lookup_color (style_context, "unpainted-border-color", &colour);
	}
	gdk_cairo_set_source_rgba (cr, &colour);
	cairo_set_line_width (cr, border.left);
	cairo_rectangle (cr, x_pos, y_pos, cell_size, cell_size);
	cairo_stroke (cr);

	/* Draw the text */
	text = localise_cell_digit (hitori, &iter);
	layout = pango_cairo_create_layout (cr);

	pango_layout_set_text (layout, text, -1);

	font_desc = (painted == TRUE) ? hitori->painted_font_desc : hitori->normal_font_desc;

	if (hitori->board[iter.x][iter.y].status & CELL_ERROR) {
		lookup_color (style_context, "mistaken-number-color", &colour);
		gdk_cairo_set_source_rgba (cr, &colour);
		pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
	} else if (painted) {
		lookup_color (style_context, "painted-number-color", &colour);
		gdk_cairo_set_source_rgba (cr, &colour);
		pango_font_description_set_weight (font_desc, PANGO_WEIGHT_NORMAL);
	} else {
		g_assert (!painted);
		lookup_color (style_context, "unpainted-number-color", &colour);
		gdk_cairo_set_source_rgba (cr, &colour);
		pango_font_description_set_weight (font_desc, PANGO_WEIGHT_NORMAL);
	}

	pango_layout_set_font_description (layout, font_desc);

	pango_layout_get_pixel_size (layout, &text_width, &text_height);
	cairo_move_to (cr,
	               x_pos + (cell_size - text_width) / 2,
	               y_pos + (cell_size - text_height) / 2);

	pango_cairo_show_layout (cr, layout);

	g_object_unref (layout);

	if (hitori->cursor_active &&
	    hitori->cursor_position.x == iter.x && hitori->cursor_position.y == iter.y &&
	    gtk_widget_is_focus (hitori->drawing_area)) {
		/* Draw the cursor */
		lookup_color (style_context, "cursor-color", &colour);
		gdk_cairo_set_source_rgba (cr, &colour);
		cairo_set_line_width (cr, border.left);
		cairo_rectangle (cr,
		                 x_pos + CURSOR_MARGIN, y_pos + CURSOR_MARGIN,
		                 cell_size - (2 * CURSOR_MARGIN), cell_size - (2 * CURSOR_MARGIN));
		cairo_stroke (cr);
	}
}

gboolean
hitori_draw_cb (GtkWidget *drawing_area, cairo_t *cr, Hitori *hitori)
{
	gint area_width, area_height;
	HitoriVector iter;
	guint board_width, board_height;
	gdouble cell_size;
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

	board_width -= BORDER_LEFT;
	board_height -= BORDER_LEFT;

	/* Work out the cell size and scale all text accordingly */
	cell_size = (gdouble) board_width / (gdouble) hitori->board_size;
	pango_font_description_set_absolute_size (hitori->normal_font_desc, cell_size * NORMAL_FONT_SCALE * 0.8 * PANGO_SCALE);
	pango_font_description_set_absolute_size (hitori->painted_font_desc, cell_size * PAINTED_FONT_SCALE * 0.8 * PANGO_SCALE);

	/* Centre the board */
	hitori->drawing_area_x_offset = (area_width - board_width) / 2.0;
	hitori->drawing_area_y_offset = (area_height - board_height) / 2.0;
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
		gdouble line_width = BORDER_LEFT * 2.5;
		GdkRGBA colour;
		lookup_color (style_context, "hint-border-color", &colour);
		gdk_cairo_set_source_rgba (cr, &colour);
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

static void
hitori_update_cell_state (Hitori *hitori, HitoriVector pos, gboolean tag1, gboolean tag2)
{
	HitoriUndo *undo;
	gboolean recheck = FALSE;

	/* Update the undo stack */
	undo = g_new (HitoriUndo, 1);
	undo->cell = pos;
	undo->undo = hitori->undo_stack;
	undo->redo = NULL;

	if (tag1 && tag2) {
		/* Update both tags' state */
		hitori->board[pos.x][pos.y].status ^= CELL_TAG1;
		hitori->board[pos.x][pos.y].status ^= CELL_TAG2;
		undo->type = UNDO_TAGS;
	} else if (tag1) {
		/* Update tag 1's state */
		hitori->board[pos.x][pos.y].status ^= CELL_TAG1;
		undo->type = UNDO_TAG1;
	} else if (tag2) {
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
}

gboolean
hitori_button_release_cb (GtkWidget *drawing_area, GdkEventButton *event, Hitori *hitori)
{
	gint width, height;
	gdouble cell_size;
	HitoriVector pos;

	if (hitori->processing_events == FALSE)
		return FALSE;

	width = gdk_window_get_width (gtk_widget_get_window (hitori->drawing_area));
	height = gdk_window_get_height (gtk_widget_get_window (hitori->drawing_area));

	/* Clamp the width/height to the minimum */
	if (height < width)
		width = height;

	cell_size = (gdouble) width / (gdouble) hitori->board_size;

	/* Determine the cell in which the button was released */
	pos.x = (guchar) ((event->x - hitori->drawing_area_x_offset) / cell_size);
	pos.y = (guchar) ((event->y - hitori->drawing_area_y_offset) / cell_size);

	if (pos.x >= hitori->board_size || pos.y >= hitori->board_size)
		return FALSE;

	/* Move the cursor to the clicked cell and deactivate it
	 * (assuming player will use the mouse for the next move) */
	hitori->cursor_position.x = pos.x;
	hitori->cursor_position.y = pos.y;
	hitori->cursor_active = FALSE;

	hitori_update_cell_state (hitori, pos,
	                          event->state & GDK_SHIFT_MASK,
	                          event->state & GDK_CONTROL_MASK);

	return FALSE;
}

gboolean
hitori_key_press_cb (GtkWidget *drawing_area, GdkEventKey *event, Hitori *hitori)
{
	gboolean did_something = TRUE;

	if (hitori->cursor_active) {
		switch (event->keyval) {
			case GDK_KEY_Left:
			case GDK_KEY_h:
				if (hitori->cursor_position.x > 0) {
					hitori->cursor_position.x -= 1;
				}
				break;
			case GDK_KEY_Right:
			case GDK_KEY_l:
				if (hitori->cursor_position.x < hitori->board_size - 1) {
					hitori->cursor_position.x += 1;
				}
				break;
			case GDK_KEY_Up:
			case GDK_KEY_k:
				if (hitori->cursor_position.y > 0) {
					hitori->cursor_position.y -= 1;
				}
				break;
			case GDK_KEY_Down:
			case GDK_KEY_j:
				if (hitori->cursor_position.y < hitori->board_size - 1) {
					hitori->cursor_position.y += 1;
				}
				break;
			case GDK_KEY_space:
			case GDK_KEY_Return:
				hitori_update_cell_state (hitori, hitori->cursor_position,
				                          event->state & GDK_SHIFT_MASK,
				                          event->state & GDK_CONTROL_MASK);
				break;
			default:
				did_something = FALSE;
		}
	} else {
		/* Activate the cell cursor if any of the keys related to playing with keyboard are pressed */
		switch (event->keyval) {
			case GDK_KEY_Left:
			case GDK_KEY_h:
			case GDK_KEY_Right:
			case GDK_KEY_l:
			case GDK_KEY_Up:
			case GDK_KEY_k:
			case GDK_KEY_Down:
			case GDK_KEY_j:
			case GDK_KEY_space:
			case GDK_KEY_Return:
				hitori->cursor_active = TRUE;
				break;
			default:
				break;
		}
	}

	if (did_something) {
		/* Redraw */
		gtk_widget_queue_draw (hitori->drawing_area);
	}

	return did_something;
}

void
hitori_destroy_cb (GtkWindow *window, Hitori *hitori)
{
	hitori_quit (hitori);
}

static void
quit_cb (GSimpleAction *action, GVariant *parameters, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	hitori_quit (self);
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
	} else if (timer_was_running && hitori->processing_events) {
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
	guint board_width, board_height;
	gdouble drawing_area_x_offset, drawing_area_y_offset;
	gdouble cell_size;

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

	board_width -= BORDER_LEFT;
	board_height -= BORDER_LEFT;

	cell_size = (gdouble) board_width / (gdouble) hitori->board_size;

	drawing_area_x_offset = (area_width - board_width) / 2.0;
	drawing_area_y_offset = (area_height - board_height) / 2.0;

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

	self->cursor_position = self->undo_stack->cell;
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
	self->cursor_position = self->undo_stack->cell;

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
help_cb (GSimpleAction *action, GVariant *parameters, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	GError *error = NULL;
	gboolean retval;

#if GTK_CHECK_VERSION(3, 22, 0)
	retval = gtk_show_uri_on_window (GTK_WINDOW (self->window), "help:hitori", gtk_get_current_event_time (), &error);
#else
	retval = gtk_show_uri (gtk_widget_get_screen (self->window), "help:hitori", gtk_get_current_event_time (), &error);
#endif

	if (retval == FALSE) {
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

	const gchar *authors[] =
	{
		"Philip Withnall <philip@tecnocode.co.uk>",
		"Ben Windsor <benjw_823@hotmail.com>",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (self->window),
				"version", VERSION,
				"copyright", _("Copyright \xc2\xa9 2007\342\200\2232010 Philip Withnall"),
				"comments", _("A logic puzzle originally designed by Nikoli"),
				"authors", authors,
				"translator-credits", _("translator-credits"),
				"logo-icon-name", "org.gnome.Hitori",
				"license-type", GTK_LICENSE_GPL_3_0,
				"wrap-license", TRUE,
				"website-label", _("Hitori Website"),
				"website", PACKAGE_URL,
				NULL);
}

static void
board_size_change_cb (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	HitoriApplication *self = HITORI_APPLICATION (user_data);
	gchar *size_str;
	guint64 size;

	size_str = g_settings_get_string (self->settings, "board-size");
	size = g_ascii_strtoull (size_str, NULL, 10);
	g_free (size_str);

	if (size > MAX_BOARD_SIZE) {
		GVariant *default_size = g_settings_get_default_value (self->settings, "board-size");
		g_variant_get (default_size, "s", &size_str);
		g_variant_unref (default_size);
		size = g_ascii_strtoull (size_str, NULL, 10);
		g_free (size_str);
		g_assert (size <= MAX_BOARD_SIZE);
	}

	hitori_set_board_size (self, size);
}
