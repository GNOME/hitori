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

#include <config.h>
#include <locale.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "main.h"
#include "interface.h"
#include "generator.h"

static void constructed (GObject *object);
static void get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void startup (GApplication *application);
static void activate (GApplication *application);

typedef struct {
	/* Command line parameters. */
	gboolean debug;
	guint seed;
} HitoriApplicationPrivate;

typedef enum {
	PROP_DEBUG = 1,
	PROP_SEED
} HitoriProperty;

G_DEFINE_TYPE_WITH_PRIVATE (HitoriApplication, hitori_application, GTK_TYPE_APPLICATION)

static void
hitori_application_class_init (HitoriApplicationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GApplicationClass *gapplication_class = G_APPLICATION_CLASS (klass);

	gobject_class->constructed = constructed;
	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;

	gapplication_class->startup = startup;
	gapplication_class->activate = activate;

	g_object_class_install_property (gobject_class, PROP_DEBUG,
	                                 g_param_spec_boolean ("debug",
	                                                       "Debugging Mode", "Whether debugging mode is active.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_SEED,
	                                 g_param_spec_uint ("seed",
	                                                    "Generation Seed", "Seed controlling generation of the board.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
hitori_application_init (HitoriApplication *self)
{
	HitoriApplicationPrivate *priv;

	priv = hitori_application_get_instance_private (self);

	priv->debug = FALSE;
	priv->seed = 0;
}

static void
constructed (GObject *object)
{
	HitoriApplicationPrivate *priv;

	priv = hitori_application_get_instance_private (HITORI_APPLICATION (object));

	/* Set various properties up */
	g_application_set_application_id (G_APPLICATION (object), "org.gnome.Hitori");

	/* Localisation */
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	const GOptionEntry options[] = {
		{ "debug", 0, 0, G_OPTION_ARG_NONE, &(priv->debug), N_("Enable debug mode"), NULL },
		/* Translators: This means to choose a number as the "seed" for random number generation used when creating a board */
		{ "seed", 0, 0, G_OPTION_ARG_INT, &(priv->seed), N_("Seed the board generation"), NULL },
		{ NULL }
	};

	g_application_add_main_option_entries (G_APPLICATION (object), options);
	g_application_set_option_context_parameter_string (G_APPLICATION (object), _("- Play a game of Hitori"));

	g_set_application_name (_("Hitori"));
	g_set_prgname ("org.gnome.Hitori");
	gtk_window_set_default_icon_name ("org.gnome.Hitori");

	/* Chain up to the parent class */
	G_OBJECT_CLASS (hitori_application_parent_class)->constructed (object);
}

static void
get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	HitoriApplicationPrivate *priv;

	priv = hitori_application_get_instance_private (HITORI_APPLICATION (object));

	switch (property_id) {
		case PROP_DEBUG:
			g_value_set_boolean (value, priv->debug);
			break;
		case PROP_SEED:
			g_value_set_uint (value, priv->seed);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	HitoriApplicationPrivate *priv;

	priv = hitori_application_get_instance_private (HITORI_APPLICATION (object));

	switch (property_id) {
		case PROP_DEBUG:
			priv->debug = g_value_get_boolean (value);
			break;
		case PROP_SEED:
			priv->seed = g_value_get_uint (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
debug_handler (const char *log_domain, GLogLevelFlags log_level, const char *message, HitoriApplication *self)
{
	HitoriApplicationPrivate *priv;

	priv = hitori_application_get_instance_private (self);

	/* Only display debug messages if we've been run with --debug */
	if (priv->debug == TRUE) {
		g_log_default_handler (log_domain, log_level, message, NULL);
	}
}

static void
startup (GApplication *application)
{
	/* Chain up. */
	G_APPLICATION_CLASS (hitori_application_parent_class)->startup (application);

	/* Debug log handling */
	g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc) debug_handler, application);
}

static void
activate (GApplication *application)
{
	HitoriApplication *self = HITORI_APPLICATION (application);
	HitoriApplicationPrivate *priv;

	priv = hitori_application_get_instance_private (self);

	/* Create the interface. */
	if (self->window == NULL) {
		GdkRectangle geometry;
		HitoriUndo *undo;
		gboolean window_maximized;
		gchar *size_str;

		/* Setup */
		self->debug = priv->debug;
		self->settings = g_settings_new ("org.gnome.hitori");
		size_str = g_settings_get_string (self->settings, "board-size");
		self->board_size = g_ascii_strtoull (size_str, NULL, 10);
		g_free (size_str);

		if (self->board_size > MAX_BOARD_SIZE) {
			GVariant *default_size = g_settings_get_default_value (self->settings, "board-size");
			g_variant_get (default_size, "s", &size_str);
			g_variant_unref (default_size);
			self->board_size = g_ascii_strtoull (size_str, NULL, 10);
			g_free (size_str);
			g_assert (self->board_size <= MAX_BOARD_SIZE);
		}

		undo = g_new0 (HitoriUndo, 1);
		undo->type = UNDO_NEW_GAME;
		self->undo_stack = undo;

		/* Showtime! */
		hitori_create_interface (self);
		hitori_generate_board (self, self->board_size, priv->seed);

		/* Restore window position and size */
		window_maximized = g_settings_get_boolean (self->settings,
																							 "window-maximized");
		g_settings_get (self->settings,
										"window-position", "(ii)",
										&geometry.x, &geometry.y);
		g_settings_get (self->settings,
										"window-size", "(ii)",
										&geometry.width, &geometry.height);

		if (window_maximized) {
			gtk_window_maximize (GTK_WINDOW (self->window));
		} else {
			if (geometry.x > -1 && geometry.y > -1) {
				gtk_window_move (GTK_WINDOW (self->window),
												 geometry.x, geometry.y);
			}

			if (geometry.width >= 0 && geometry.height >= 0) {
				gtk_window_resize (GTK_WINDOW (self->window),
													 geometry.width, geometry.height);
			}
		}

		gtk_window_set_application (GTK_WINDOW (self->window), GTK_APPLICATION (self));
		gtk_widget_show_all (self->window);
	}

	/* Bring it to the foreground */
	gtk_window_present (GTK_WINDOW (self->window));
}

HitoriApplication *
hitori_application_new (void)
{
	return HITORI_APPLICATION (g_object_new (HITORI_TYPE_APPLICATION, NULL));
}

void
hitori_new_game (Hitori *hitori, guint board_size)
{
	hitori->made_a_move = FALSE;

	hitori_generate_board (hitori, board_size, 0);
	hitori_clear_undo_stack (hitori);
	gtk_widget_queue_draw (hitori->drawing_area);

	hitori_reset_timer (hitori);
	hitori_start_timer (hitori);

	/* Reset the cursor position */
	hitori->cursor_position.x = 0;
	hitori->cursor_position.y = 0;
}

void
hitori_clear_undo_stack (Hitori *hitori)
{	
	/* Clear the undo stack */
	if (hitori->undo_stack != NULL) {
		while (hitori->undo_stack->redo != NULL)
			hitori->undo_stack = hitori->undo_stack->redo;

		while (hitori->undo_stack->undo != NULL) {
			hitori->undo_stack = hitori->undo_stack->undo;
			g_free (hitori->undo_stack->redo);
			if (hitori->undo_stack->type == UNDO_NEW_GAME)
				break;
		}

		/* Reset the "new game" item */
		hitori->undo_stack->undo = NULL;
		hitori->undo_stack->redo = NULL;
	}

	g_simple_action_set_enabled (hitori->undo_action, FALSE);
	g_simple_action_set_enabled (hitori->redo_action, FALSE);
}

void
hitori_set_board_size (Hitori *hitori, guint board_size)
{
	/* Ask the user if they want to stop the current game, if they're playing at the moment */
	if (hitori->processing_events == TRUE && hitori->made_a_move == TRUE) {
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (hitori->window),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_NONE,
				_("Do you want to stop the current game?"));

		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
					_("Keep _Playing"), GTK_RESPONSE_REJECT,
					_("_New Game"), GTK_RESPONSE_ACCEPT,
					NULL);

		if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT) {
			gtk_widget_destroy (dialog);
			return;
		}

		gtk_widget_destroy (dialog);
	}

	/* Kill the current game and resize the board */
	hitori_new_game (hitori, board_size);
}

void
hitori_print_board (Hitori *hitori)
{
	if (hitori->debug) {
		HitoriVector iter;

		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
				if ((hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE)
					g_printf ("%u ", hitori->board[iter.x][iter.y].num);
				else
					g_printf ("X ");
			}
			g_printf ("\n");
		}
	}
}

void
hitori_free_board (Hitori *hitori)
{
	guint i;

	if (hitori->board == NULL)
		return;

	for (i = 0; i < hitori->board_size; i++)
		g_slice_free1 (sizeof (HitoriCell) * hitori->board_size, hitori->board[i]);
	g_free (hitori->board);
}

void
hitori_enable_events (Hitori *hitori)
{
	hitori->processing_events = TRUE;

	if (hitori->undo_stack->redo != NULL)
		g_simple_action_set_enabled (hitori->redo_action, TRUE);
	if (hitori->undo_stack->undo != NULL)
		g_simple_action_set_enabled (hitori->redo_action, TRUE);
	g_simple_action_set_enabled (hitori->hint_action, TRUE);

	hitori_start_timer (hitori);
}

void
hitori_disable_events (Hitori *hitori)
{
	hitori->processing_events = FALSE;
	g_simple_action_set_enabled (hitori->redo_action, FALSE);
	g_simple_action_set_enabled (hitori->undo_action, FALSE);
	g_simple_action_set_enabled (hitori->hint_action, FALSE);

	hitori_pause_timer (hitori);
}

static void
set_timer_label (Hitori *hitori)
{
	gchar *text = g_strdup_printf ("%02u∶\xE2\x80\x8E%02u", hitori->timer_value / 60, hitori->timer_value % 60);
	gtk_label_set_text (hitori->timer_label, text);
	g_free (text);
}

static gboolean
update_timer_cb (Hitori *hitori)
{
	hitori->timer_value++;
	set_timer_label (hitori);

	return TRUE;
}

void
hitori_start_timer (Hitori *hitori)
{
	// Remove any old timeout
	hitori_pause_timer (hitori);

	set_timer_label (hitori);
	hitori->timeout_id = g_timeout_add_seconds (1, (GSourceFunc) update_timer_cb, hitori);
}

void
hitori_pause_timer (Hitori *hitori)
{
	if (hitori->timeout_id > 0) {
		g_source_remove (hitori->timeout_id);
		hitori->timeout_id = 0;
	}
}

void
hitori_reset_timer (Hitori *hitori)
{
	hitori->timer_value = 0;
	set_timer_label (hitori);
}

void
hitori_quit (Hitori *hitori)
{
	static gboolean quitting = FALSE;

	if (quitting == TRUE)
		return;
	quitting = TRUE;

	hitori_free_board (hitori);
	hitori_clear_undo_stack (hitori);
	g_free (hitori->undo_stack); /* Clear the new game element */

	if (hitori->window != NULL)
		gtk_widget_destroy (hitori->window);

	if (hitori->normal_font_desc != NULL)
		pango_font_description_free (hitori->normal_font_desc);
	if (hitori->painted_font_desc != NULL)
		pango_font_description_free (hitori->painted_font_desc);

	g_object_unref (hitori->settings);

	g_application_quit (G_APPLICATION (hitori));
}

int
main (int argc, char *argv[])
{
	HitoriApplication *app;
	int status;

#if !GLIB_CHECK_VERSION (2, 35, 0)
	g_type_init ();
#endif

	app = hitori_application_new ();
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);

	return status;
}
