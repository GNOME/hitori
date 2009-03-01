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

#include <glib.h>
#include <glib/gprintf.h>

#include "main.h"

/* Rule 1: There must only be one of each number in the unpainted cells
 * in each row and column. */
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

	/*
	 * Check the squares immediately below and to the right of the current one;
	 * if they're painted in, the rule fails.
	 */

	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			if (hitori->board[iter.x][iter.y].status & CELL_PAINTED) {
				if ((iter.x < hitori->board_size - 1 && hitori->board[iter.x+1][iter.y].status & CELL_PAINTED) ||
				    (iter.y < hitori->board_size - 1 && hitori->board[iter.x][iter.y+1].status & CELL_PAINTED)) {
					if (hitori->debug)
						    g_debug ("Rule 2 failed");
					return FALSE;
				}
			}
		}
	}

	if (hitori->debug)
		g_debug ("Rule 2 OK");

	return TRUE;
}

/* Rule 3: all the unpainted cells must be joined together in one group. */
gboolean
hitori_check_rule3 (Hitori *hitori)
{
	HitoriVector iter;
	guint max_group = 0;
	GQueue *unchecked_cells_x, *unchecked_cells_y;
	guint **groups = g_new (guint*, hitori->board_size);
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		groups[iter.x] = g_new0 (guint, hitori->board_size);
	guint *group_bases = g_new0 (guint, hitori->board_size * hitori->board_size / 2);

	/* HACKHACK! TODO: Clean up this horrible mess */
	unchecked_cells_x = g_queue_new ();
	unchecked_cells_y = g_queue_new ();

	iter.x = 0;
	iter.y = 0;

	do {
		if (hitori->board[iter.x][iter.y].status & CELL_PAINTED) {
			/* If it's painted ensure it's in group 0 */
			groups[iter.x][iter.y] = 0;
		} else {
			/* Try and apply a group from a surrounding cell */
			if (iter.y >= 1 && groups[iter.x][iter.y-1] != 0 && (hitori->board[iter.x][iter.y-1].status & CELL_PAINTED) == FALSE)
				groups[iter.x][iter.y] = groups[iter.x][iter.y-1];
			else if (iter.y + 1 < hitori->board_size && groups[iter.x][iter.y+1] != 0 && (hitori->board[iter.x][iter.y+1].status & CELL_PAINTED) == FALSE)
				groups[iter.x][iter.y] = groups[iter.x][iter.y+1];
			else if (iter.x >= 1 && groups[iter.x-1][iter.y] != 0 && (hitori->board[iter.x-1][iter.y].status & CELL_PAINTED) == FALSE)
				groups[iter.x][iter.y] = groups[iter.x-1][iter.y];
			else if (iter.x + 1 < hitori->board_size && groups[iter.x+1][iter.y] != 0 && (hitori->board[iter.x+1][iter.y].status & CELL_PAINTED) == FALSE)
				groups[iter.x][iter.y] = groups[iter.x+1][iter.y];
			else {
				max_group++;
				group_bases[max_group] = max_group;
				groups[iter.x][iter.y] = max_group;
			}

			/* Check for converged groups */
			if (iter.y >= 1 && (hitori->board[iter.x][iter.y-1].status & CELL_PAINTED) == FALSE && groups[iter.x][iter.y-1] != 0 && group_bases[groups[iter.x][iter.y-1]] != group_bases[groups[iter.x][iter.y]])
				group_bases[groups[iter.x][iter.y]] = group_bases[groups[iter.x][iter.y-1]];
			else if (iter.y + 1 < hitori->board_size && (hitori->board[iter.x][iter.y+1].status & CELL_PAINTED) == FALSE && groups[iter.x][iter.y+1] != 0 && group_bases[groups[iter.x][iter.y+1]] != group_bases[groups[iter.x][iter.y]])
				group_bases[groups[iter.x][iter.y]] = group_bases[groups[iter.x][iter.y+1]];
			else if (iter.x >= 1 && (hitori->board[iter.x-1][iter.y].status & CELL_PAINTED) == FALSE && groups[iter.x-1][iter.y] != 0 && group_bases[groups[iter.x-1][iter.y]] != group_bases[groups[iter.x][iter.y]])
				group_bases[groups[iter.x][iter.y]] = group_bases[groups[iter.x-1][iter.y]];
			else if (iter.x + 1 < hitori->board_size && (hitori->board[iter.x+1][iter.y].status & CELL_PAINTED) == FALSE && groups[iter.x+1][iter.y] != 0 && group_bases[groups[iter.x+1][iter.y]] != group_bases[groups[iter.x][iter.y]])
				group_bases[groups[iter.x][iter.y]] = group_bases[groups[iter.x+1][iter.y]];
		}

		/* Find somewhere else to go */
		if (iter.y >= 1 && iter.x < hitori->board_size && groups[iter.x][iter.y-1] == 0 && (hitori->board[iter.x][iter.y-1].status & CELL_PAINTED) == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (iter.x));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (iter.y-1));
		}
		if (iter.y + 1 < hitori->board_size && groups[iter.x][iter.y+1] == 0 && (hitori->board[iter.x][iter.y+1].status & CELL_PAINTED) == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (iter.x));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (iter.y+1));
		}
		if (iter.x >= 1 && iter.y < hitori->board_size && groups[iter.x-1][iter.y] == 0 && (hitori->board[iter.x-1][iter.y].status & CELL_PAINTED) == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (iter.x-1));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (iter.y));
		}
		if (iter.x + 1 < hitori->board_size && groups[iter.x+1][iter.y] == 0 && (hitori->board[iter.x+1][iter.y].status & CELL_PAINTED) == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (iter.x+1));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (iter.y));
		}

		/* Fetch some new coordinates to spider */
		iter.x = GPOINTER_TO_UINT (g_queue_pop_head (unchecked_cells_x));
		iter.y = GPOINTER_TO_UINT (g_queue_pop_head (unchecked_cells_y));
	} while ((GUINT_TO_POINTER (iter.x) != NULL || GUINT_TO_POINTER (iter.y) != NULL) &&
		 g_queue_get_length (unchecked_cells_x) >= 0);

	if (hitori->debug) {
		/* Print out the groups */
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
				if ((hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE)
					g_printf ("%u ", groups[iter.x][iter.y]);
				else
					g_printf ("X ");
			}
			g_printf ("\n");
		}
	}

	/* Check that there's only one group; if there's more than
	 * one, the rule fails. */
	max_group = 0;
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			if ((hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE) {
				if (max_group == 0)
					max_group = group_bases[groups[iter.x][iter.y]];
				else if (group_bases[groups[iter.x][iter.y]] != max_group) {
					/* Rule failed */
					g_queue_free (unchecked_cells_x);
					g_queue_free (unchecked_cells_y);
					g_free (group_bases);
					for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
						g_free (groups[iter.x]);
					g_free (groups);

					if (hitori->debug)
						g_debug ("Rule 3 failed");

					return FALSE;
				}
			}
		}
	}

	/* Free everything */
	g_queue_free (unchecked_cells_x);
	g_queue_free (unchecked_cells_y);
	g_free (group_bases);
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		g_free (groups[iter.x]);
	g_free (groups);

	if (hitori->debug)
		g_debug ("Rule 3 OK");

	return TRUE;
}
