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

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>

#include "main.h"
#include "rules.h"

/* Rule 1: There must only be one of each number in the unpainted cells
 * in each row and column.
 * NOTE: We don't set the error position with this rule, or it would give
 * the game away! */
gboolean
hitori_check_rule1 (Hitori *hitori)
{
	HitoriVector iter;
	gboolean *accum = g_new0 (gboolean, hitori->board_size + 1);

	/*
	 * The accumulator is an array of all the possible numbers on
	 * the board (which is one more than the board size). We then
	 * iterate through each row and column, setting a flag for each
	 * unpainted number we encounter. If we encounter a number whose
	 * flag has already been set, that number is already in the row
	 * or column unpainted, and the rule fails.
	 */

	/* Check columns for repeating numbers */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		/* Reset accum */
		for (iter.y = 0; iter.y < hitori->board_size + 1; iter.y++)
			accum[iter.y] = FALSE;

		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			if ((hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE) {
				if (accum[hitori->board[iter.x][iter.y].num-1] == TRUE) {
					if (hitori->debug) {
						g_debug ("Rule 1 failed in column %u, row %u", iter.x, iter.y);

						/* Print out the accumulator */
						for (iter.y = 0; iter.y < hitori->board_size + 1; iter.y++) {
							if (accum[iter.y] == TRUE)
								g_printf ("X");
							else
								g_printf ("_");
						}
						g_printf ("\n");
					}

					g_free (accum);
					return FALSE;
				}

				accum[hitori->board[iter.x][iter.y].num-1] = TRUE;
			}
		}
	}

	/* Now check the rows */
	for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
		/* Reset accum */
		for (iter.x = 0; iter.x < hitori->board_size+1; iter.x++)
			accum[iter.x] = FALSE;

		for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
			if ((hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE) {
				if (accum[hitori->board[iter.x][iter.y].num-1] == TRUE) {
					if (hitori->debug) {
						g_debug ("Rule 1 failed in row %u, column %u", iter.y, iter.x);

						/* Print out the accumulator */
						for (iter.y = 0; iter.y < hitori->board_size+1; iter.y++) {
							if (accum[iter.y] == TRUE)
								g_printf ("X");
							else
								g_printf ("_");
						}
						g_printf ("\n");
					}

					g_free (accum);
					return FALSE;
				}

				accum[hitori->board[iter.x][iter.y].num-1] = TRUE;
			}
		}
	}

	g_free (accum);

	if (hitori->debug)
		g_debug ("Rule 1 OK");

	return TRUE;
}

/* Rule 2: No painted cell may be adjacent to another, vertically or horizontally. */
gboolean
hitori_check_rule2 (Hitori *hitori)
{
	HitoriVector iter;
	gboolean success = TRUE;

	/* Check the squares immediately next to the current one; if they're painted, the rule fails. */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			if (hitori->board[iter.x][iter.y].status & CELL_PAINTED &&
			    ((iter.x < hitori->board_size - 1 && hitori->board[iter.x+1][iter.y].status & CELL_PAINTED) ||
			     (iter.y < hitori->board_size - 1 && hitori->board[iter.x][iter.y+1].status & CELL_PAINTED) ||
			     (iter.x > 0 && hitori->board[iter.x - 1][iter.y].status & CELL_PAINTED) ||
			     (iter.y > 0 && hitori->board[iter.x][iter.y - 1].status & CELL_PAINTED))) {
				if (hitori->debug)
					    g_debug ("Rule 2 failed");

				/* Mark the cell as being erroneous and continue to the other cells so that they also get marked */
				hitori->board[iter.x][iter.y].status |= CELL_ERROR;
				success = FALSE;
			} else {
				/* Clear any error in the cell */
				hitori->board[iter.x][iter.y].status &= ~CELL_ERROR;
			}
		}
	}

	if (hitori->debug && success)
		g_debug ("Rule 2 OK");

	return success;
}

/* Rule 3: all the unpainted cells must be joined together in one group. */
gboolean
hitori_check_rule3 (Hitori *hitori)
{
	GQueue queue = G_QUEUE_INIT;
	gboolean **reached;
	HitoriVector iter, *first = NULL;
	gboolean success;

	/* Pick an unpainted cell. */
	for (iter.x = 0; first == NULL && iter.x < hitori->board_size; iter.x++)
		for (iter.y = 0; !first && iter.y < hitori->board_size; iter.y++)
			if ((hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE)
				first = g_slice_dup (HitoriVector, &iter);
	if (first == NULL)
		return FALSE;

	/* Allocate a board of booleans to keep track of which cells we can reach */
	reached = g_new (gboolean*, hitori->board_size);
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		reached[iter.x] = g_new0 (gboolean, hitori->board_size);

	/* Use a basic floodfill algorithm to traverse the board */
	g_queue_push_tail (&queue, first);
	while (g_queue_is_empty (&queue) == FALSE) {
		HitoriVector *ptr = g_queue_pop_head (&queue);

		iter = *ptr;

		if (reached[iter.x][iter.y] == FALSE && (hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE) {
			/* Mark the cell as having been reached */
			reached[iter.x][iter.y] = TRUE;

			if (iter.x > 0) {
				/* Cell to our left */
				HitoriVector *neighbour = g_slice_new (HitoriVector);
				neighbour->x = iter.x - 1;
				neighbour->y = iter.y;
				g_queue_push_tail (&queue, neighbour);
			}
			if (iter.y > 0) {
				/* Cell above us */
				HitoriVector *neighbour = g_slice_new (HitoriVector);
				neighbour->x = iter.x;
				neighbour->y = iter.y - 1;
				g_queue_push_tail (&queue, neighbour);
			}
			if (iter.x < hitori->board_size - 1) {
				/* Cell to our right */
				HitoriVector *neighbour = g_slice_new (HitoriVector);
				neighbour->x = iter.x + 1;
				neighbour->y = iter.y;
				g_queue_push_tail (&queue, neighbour);
			}
			if (iter.y < hitori->board_size - 1) {
				/* Cell below us */
				HitoriVector *neighbour = g_slice_new (HitoriVector);
				neighbour->x = iter.x;
				neighbour->y = iter.y + 1;
				g_queue_push_tail (&queue, neighbour);
			}
		}

		g_slice_free (HitoriVector, ptr);
	}

	/* Check if there's an unpainted cell we haven't reached */
	success = TRUE;
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			if (reached[iter.x][iter.y] == FALSE && (hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE) {
				success = FALSE;

				/* Highlight its neighbours as erroneous */
				if (iter.x > 0 && hitori->board[iter.x - 1][iter.y].status & CELL_PAINTED)
					hitori->board[iter.x - 1][iter.y].status |= CELL_ERROR;
				if (iter.y > 0 && hitori->board[iter.x][iter.y - 1].status & CELL_PAINTED)
					hitori->board[iter.x][iter.y - 1].status |= CELL_ERROR;
				if (iter.x < hitori->board_size - 1 && hitori->board[iter.x + 1][iter.y].status & CELL_PAINTED)
					hitori->board[iter.x + 1][iter.y].status |= CELL_ERROR;
				if (iter.y < hitori->board_size - 1 && hitori->board[iter.x][iter.y + 1].status & CELL_PAINTED)
					hitori->board[iter.x][iter.y + 1].status |= CELL_ERROR;
			}
		}
	}

	/* Free everything */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		g_free (reached[iter.x]);
	g_free (reached);

	if (hitori->debug)
		g_debug (success ? "Rule 3 OK" : "Rule 3 failed");

	return success;
}

gboolean
hitori_check_win (Hitori *hitori)
{
	/* Check to see if all three rules are satisfied yet. If they are, we've won.
	 * NOTE: We check rule 1 last, as it's the only rule which won't set an error position.
	 * We check rules 2 and 3 unconditionally because they both set errors. */
	gboolean rule2 = hitori_check_rule2 (hitori);
	gboolean rule3 = hitori_check_rule3 (hitori);

	if (rule2 && rule3 && hitori_check_rule1 (hitori)) {
		/* Win! */
		GtkWidget *dialog;
		gchar *message;
		gint ret;

		/* Tell the user they've won */
		hitori_disable_events (hitori);

		/* Translators: The first parameter is the number of minutes which have elapsed since the start of the game; the second parameter is
		 * the number of seconds. */
		message = g_strdup_printf (_("You’ve won in a time of %02u:%02u!"), hitori->timer_value / 60, hitori->timer_value % 60);
		dialog = gtk_message_dialog_new (GTK_WINDOW (hitori->window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, "%s", message);
		g_free (message);

		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
					_("_Quit"), GTK_RESPONSE_REJECT,
					_("_Play Again"), GTK_RESPONSE_ACCEPT,
					NULL);

		ret = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (ret == GTK_RESPONSE_REJECT)
			hitori_quit (hitori);
		else if (ret == GTK_RESPONSE_ACCEPT)
			hitori_new_game (hitori, hitori->board_size);

		return TRUE;
	}

	return FALSE;
}
