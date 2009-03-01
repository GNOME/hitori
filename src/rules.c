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

#include <glib.h>
#include "main.h"

/* Rule 1: There must only be one of each number in the unpainted cells
 * in each row and column. */
gboolean
hitori_check_rule1 (Hitori *hitori)
{
	guint x, y;
	gboolean accum[BOARD_SIZE+1];

	/* Check columns for repeating numbers */
	for (x = 0; x < BOARD_SIZE; x++) {
		/* Reset accum */
		for (y = 0; y < BOARD_SIZE+1; y++)
			accum[y] = FALSE;

		for (y = 0; y < BOARD_SIZE; y++) {
			if (hitori->board[x][y].painted == FALSE) {
				if (accum[hitori->board[x][y].num-1] == TRUE) {
					if (hitori->debug) {
						g_message ("Rule 1 failed in column %u, row %u", x, y);

						/* Print out the accumulator */
						for (y = 0; y < BOARD_SIZE+1; y++) {
							if (accum[y] == TRUE)
								printf ("X");
							else
								printf ("_");
						}
						printf ("\n");
					}
					return FALSE;
				}

				accum[hitori->board[x][y].num-1] = TRUE;
			}
		}
	}

	/* Now check the rows */
	for (y = 0; y < BOARD_SIZE; y++) {
		/* Reset accum */
		for (x = 0; x < BOARD_SIZE+1; x++)
			accum[x] = FALSE;

		for (x = 0; x < BOARD_SIZE; x++) {
			if (hitori->board[x][y].painted == FALSE) {
				if (accum[hitori->board[x][y].num-1] == TRUE) {
					if (hitori->debug) {
						g_message ("Rule 1 failed in row %u, column %u", y, x);

						/* Print out the accumulator */
						for (y = 0; y < BOARD_SIZE+1; y++) {
							if (accum[y] == TRUE)
								printf ("X");
							else
								printf ("_");
						}
						printf ("\n");
					}
					return FALSE;
				}

				accum[hitori->board[x][y].num-1] = TRUE;
			}
		}
	}

	if (hitori->debug)
		g_message("Rule 1 OK");
	return TRUE;
}

/* Rule 2: No painted cell may be adjacent to another, vertically or horizontally. */
gboolean
hitori_check_rule2 (Hitori *hitori)
{
	guint x, y;

	/* Check the squares immediately below and to the right of the current one;
	 * if they're painted in, it's illegal. */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if (hitori->board[x][y].painted == TRUE) {
				if ((x < BOARD_SIZE - 1 && hitori->board[x+1][y].painted == TRUE) ||
				    (y < BOARD_SIZE - 1 && hitori->board[x][y+1].painted == TRUE)) {
					if (hitori->debug)
						    g_message ("Rule 2 failed");
					return FALSE;
				}
			}
		}
	}

	if (hitori->debug)
		g_message("Rule 2 OK");
	return TRUE;
}

/* Rule 3: all the unpainted cells must be joined together in one group. */
gboolean
hitori_check_rule3 (Hitori *hitori)
{
	guint groups[BOARD_SIZE][BOARD_SIZE];
	guint group_bases[BOARD_SIZE * BOARD_SIZE / 2];
	guint x = 0, y = 0, max_group = 0;
	GQueue *unchecked_cells_x, *unchecked_cells_y;

	/* Clear the groups */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++)
			groups[x][y] = 0;
	}

	/* HACKHACK! */
	unchecked_cells_x = g_queue_new ();
	unchecked_cells_y = g_queue_new ();

	/* Find the first unpainted cell */
	x = 0;
	y = 0;
	if (hitori->board[x][y].painted == TRUE)
		x = 1;

	do {
		/* Try and apply a group from a surrounding cell */
		if (y >= 1 && groups[x][y-1] != 0 && hitori->board[x][y-1].painted == FALSE)
			groups[x][y] = groups[x][y-1];
		else if (y + 1 < BOARD_SIZE && groups[x][y+1] != 0 && hitori->board[x][y+1].painted == FALSE)
			groups[x][y] = groups[x][y+1];
		else if (x >= 1 && groups[x-1][y] != 0 && hitori->board[x-1][y].painted == FALSE)
			groups[x][y] = groups[x-1][y];
		else if (x + 1 < BOARD_SIZE && groups[x+1][y] != 0 && hitori->board[x+1][y].painted == FALSE)
			groups[x][y] = groups[x+1][y];
		else {
			max_group++;
			group_bases[max_group] = max_group;
			groups[x][y] = max_group;
		}

		/* Check for converged groups */
		if (y >= 1 && hitori->board[x][y-1].painted == FALSE && group_bases[groups[x][y-1]] != group_bases[groups[x][y]])
			group_bases[groups[x][y]] = group_bases[groups[x][y-1]];
		else if (y + 1 < BOARD_SIZE && hitori->board[x][y+1].painted == FALSE && group_bases[groups[x][y+1]] != group_bases[groups[x][y]])
			group_bases[groups[x][y]] = group_bases[groups[x][y+1]];
		else if (x >= 1 && hitori->board[x-1][y].painted == FALSE && group_bases[groups[x-1][y]] != group_bases[groups[x][y]])
			group_bases[groups[x][y]] = group_bases[groups[x-1][y]];
		else if (x + 1 < BOARD_SIZE && hitori->board[x+1][y].painted == FALSE && group_bases[groups[x+1][y]] != group_bases[groups[x][y]])
			group_bases[groups[x][y]] = group_bases[groups[x+1][y]];

		/* Find somewhere else to go */
		if (y >= 1 && x < BOARD_SIZE && groups[x][y-1] == 0 && hitori->board[x][y-1].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y-1));
		}
		if (y + 1 < BOARD_SIZE && x >= 0 && groups[x][y+1] == 0 && hitori->board[x][y+1].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y+1));
		}
		if (x >= 1 && y < BOARD_SIZE && groups[x-1][y] == 0 && hitori->board[x-1][y].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x-1));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y));
		}
		if (x + 1 < BOARD_SIZE && y >= 0 && groups[x+1][y] == 0 && hitori->board[x+1][y].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x+1));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y));
		}

		/* Fetch some new coordinates to spider */
		x = GPOINTER_TO_UINT (g_queue_pop_head (unchecked_cells_x));
		y = GPOINTER_TO_UINT (g_queue_pop_head (unchecked_cells_y));
	} while (GUINT_TO_POINTER (x) != NULL &&
		 GUINT_TO_POINTER (y) != NULL &&
		 g_queue_get_length (unchecked_cells_x) >= 0);

	if (hitori->debug) {
		/* Print out the groups */
		for (y = 0; y < BOARD_SIZE; y++) {
			for (x = 0; x < BOARD_SIZE; x++) {
				if (hitori->board[x][y].painted == FALSE)
					printf ("%u ", group_bases[groups[x][y]]);
				else
					printf ("X ");
			}
			printf ("\n");
		}
	}

	/* Check that there's only one group */
	max_group = 0;
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if (hitori->board[x][y].painted == FALSE) {
				if (max_group == 0)
					max_group = group_bases[groups[x][y]];
				else if (group_bases[groups[x][y]] != max_group) {
					if (hitori->debug)
						g_message ("Rule 3 failed");
					return FALSE;
				}
			}
		}
	}

	/* Free everything */
	g_queue_free (unchecked_cells_x);
	g_queue_free (unchecked_cells_y);

	if (hitori->debug)
		g_message("Rule 3 OK");

	return TRUE;
}
