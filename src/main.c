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

#include <config.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "main.h"
#include "interface.h"
#include "generator.h"

void
hitori_new_game (Hitori *hitori, guint board_size)
{
	hitori->made_a_move = FALSE;

	hitori_generate_board (hitori, board_size, -1);
	hitori_clear_undo_stack (hitori);
	hitori_draw_board_simple (hitori, FALSE, TRUE);
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
	}

	/* Reset the "new game" item */
	hitori->undo_stack->undo = NULL;
	hitori->undo_stack->redo = NULL;

	gtk_action_set_sensitive (hitori->undo_action, FALSE);
	gtk_action_set_sensitive (hitori->redo_action, FALSE);
}

void
hitori_set_board_size (Hitori *hitori, guint board_size)
{
	/* Ask the user if they want to stop the current game, if they're playing at the moment */
	if (hitori->processing_events == TRUE && hitori->made_a_move == TRUE) {
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				_("Do you want to stop the current game?"));
		if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES) {
			/* Kill the current game and resize the board */
			gtk_widget_destroy (dialog);
			hitori_new_game (hitori, board_size);
			return;
		}

		gtk_widget_destroy (dialog);
	} else {
		/* We won't actually start a new game, but resize the board anyway */
		hitori_generate_board (hitori, board_size, -1);
		hitori_draw_board_simple (hitori, FALSE, TRUE);
	}
}

void
hitori_print_board (Hitori *hitori)
{
	if (hitori->debug) {
		guint x, y;

		for (y = 0; y < BOARD_SIZE; y++) {
			for (x = 0; x < BOARD_SIZE; x++) {
				if (hitori->board[x][y].painted == FALSE)
					g_printf ("%u ", hitori->board[x][y].num);
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

	for (i = 0; i < BOARD_SIZE; i++)
		g_slice_free1 (sizeof (HitoriCell) * BOARD_SIZE, hitori->board[i]);
	g_free (hitori->board);
}

void
hitori_enable_events (Hitori *hitori)
{
	hitori->processing_events = TRUE;

	if (hitori->undo_stack->redo != NULL)
		gtk_action_set_sensitive (hitori->redo_action, TRUE);
	if (hitori->undo_stack->undo != NULL)
		gtk_action_set_sensitive (hitori->redo_action, TRUE);
	gtk_action_set_sensitive (hitori->hint_action, TRUE);
}

void
hitori_disable_events (Hitori *hitori)
{
	hitori->processing_events = FALSE;
	gtk_action_set_sensitive (hitori->redo_action, FALSE);
	gtk_action_set_sensitive (hitori->undo_action, FALSE);
	gtk_action_set_sensitive (hitori->hint_action, FALSE);
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

	if (gtk_main_level () > 0)
		gtk_main_quit ();

	exit (0);
}

int
main (int argc, char *argv[])
{
	Hitori *hitori;
	HitoriUndo *undo;
	GOptionContext *context;
	GError *error = NULL;
	gboolean debug = FALSE;
	gint seed = -1;

	const GOptionEntry options[] = {
	        { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debug mode"), NULL },
	        { "seed", 0, 0, G_OPTION_ARG_INT, &seed, N_("Seed the board generation"), NULL },
	        { NULL }
	};

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);
	g_set_application_name (_("Hitori"));
	gtk_window_set_default_icon_name ("hitori");

	/* Options */
	context = g_option_context_new (_("- Play a game of Hitori"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		/* Show an error */
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("Command-line options could not be parsed. Error: %s"), error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_error_free (error);
		exit (1);
	}

	/* Setup */
	hitori = g_new (Hitori, 1);
	hitori->debug = debug;
	hitori->board_size = DEFAULT_BOARD_SIZE;
	hitori->board = NULL;
	hitori->hint_status = 0;
	hitori->hint_x = 0;
	hitori->hint_y = 0;

	undo = g_new (HitoriUndo, 1);
	undo->type = UNDO_NEW_GAME;
	undo->x = 0;
	undo->y = 0;
	undo->redo = NULL;
	undo->undo = NULL;
	hitori->undo_stack = undo;

	/* Showtime! */
	hitori_create_interface (hitori);
	hitori_generate_board (hitori, BOARD_SIZE, seed);
	gtk_widget_show (hitori->window);

	g_option_context_free (context);

	gtk_main ();
	return 0;
}
