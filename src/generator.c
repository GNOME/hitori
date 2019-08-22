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
#include <stdlib.h>

#include "main.h"
#include "generator.h"
#include "rules.h"

void
hitori_generate_board (Hitori *hitori, guint new_board_size, guint seed)
{
	guchar i;
	guint total, old_total;
	HitoriVector iter;
	gboolean *accum, **horiz_accum;

	g_return_if_fail (hitori != NULL);
	g_return_if_fail (new_board_size > 0);

	/* Seed the random number generator */
	if (seed == 0)
		seed = g_get_real_time ();

	if (hitori->debug)
		g_debug ("Seed value: %u", seed);

	srand (seed);

	/* Deallocate any previous board */
	hitori_free_board (hitori);

	hitori->board_size = new_board_size;

	accum = g_new0 (gboolean, hitori->board_size + 2); /* Stores which numbers have been used in the current column */
	horiz_accum = g_new (gboolean*, hitori->board_size); /* Stores which numbers have been used in each row */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		horiz_accum[iter.x] = g_slice_alloc0 (sizeof (gboolean) * (hitori->board_size + 2));

	/* Allocate the board */
	hitori->board = g_new (HitoriCell*, hitori->board_size);
	for (i = 0; i < hitori->board_size; i++)
		hitori->board[i] = g_slice_alloc0 (sizeof (HitoriCell) * hitori->board_size);

	/* Generate some randomly-placed painted cells */
	total = rand () % 5 + 13; /* Total number of painted cells (between 14 and 18 inclusive) */
	/* For the moment, I'm hardcoding the range in the number of painted
	 * cells, and only specifying it for 8x8 grids. This will change in the
	 * future. */
	for (i = 0; i < total; i++) {
		/* Generate pairs of coordinates until we find one which lies between unpainted cells (or at the edge of the board) */
		do {
			iter.x = rand () % hitori->board_size;
			iter.y = rand () % hitori->board_size;

			if ((iter.y < 1 || (hitori->board[iter.x][iter.y-1].status & CELL_PAINTED) == FALSE) &&
			    (iter.y + 1 >= hitori->board_size || (hitori->board[iter.x][iter.y+1].status & CELL_PAINTED) == FALSE) &&
			    (iter.x < 1 || (hitori->board[iter.x-1][iter.y].status & CELL_PAINTED) == FALSE) &&
			    (iter.x + 1 >= hitori->board_size || (hitori->board[iter.x+1][iter.y].status & CELL_PAINTED) == FALSE))
				break;
		} while (TRUE);

		hitori->board[iter.x][iter.y].status |= (CELL_PAINTED | CELL_SHOULD_BE_PAINTED);
	}

	/* Check that the painted squares don't mess everything up */
	if (hitori_check_rule2 (hitori) == FALSE ||
	    hitori_check_rule3 (hitori) == FALSE) {
	    	g_free (accum);
		for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
			g_slice_free1 (sizeof (gboolean) * (hitori->board_size + 2), horiz_accum[iter.x]);
		g_free (horiz_accum);

		hitori_generate_board (hitori, hitori->board_size, seed + 1);
		return;
	}

	/* Fill in the squares, leaving the painted ones blank,
	 * and making sure not to repeat any previous numbers. */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		/* Reset the vertical accumulator */
		for (iter.y = 1; iter.y < hitori->board_size + 2; iter.y++)
			accum[iter.y] = FALSE;

		i = 0;
		accum[0] = TRUE;
		total = hitori->board_size + 1;
		old_total = total;

		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			if ((hitori->board[iter.x][iter.y].status & CELL_PAINTED) == FALSE) {
				while (accum[i] == TRUE || horiz_accum[iter.y][i] == TRUE) {
					if (horiz_accum[iter.y][i] == TRUE && accum[i] == FALSE)
						total--;

					if (total < 1) {
						g_free (accum);
						for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
							g_slice_free1 (sizeof (gboolean) * (hitori->board_size + 2), horiz_accum[iter.x]);
						g_free (horiz_accum);

						hitori_generate_board (hitori, hitori->board_size, seed + 1); /* We're buggered */
						return;
					}

					i = rand () % (hitori->board_size + 1) + 1;
				}

				accum[i] = TRUE;
				horiz_accum[iter.y][i] = TRUE;
				total = old_total;
				total--;

				hitori->board[iter.x][iter.y].num = i;
			}
		}
	}

	g_free (accum);
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		g_slice_free1 (sizeof (gboolean) * (hitori->board_size + 2), horiz_accum[iter.x]);
	g_free (horiz_accum);

	/* Fill in the painted squares, making sure they duplicate a number
	 * already in the column/row. */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			if (hitori->board[iter.x][iter.y].status & CELL_PAINTED) {
				do {
					i = rand () % hitori->board_size;
					if (iter.x > iter.y)
						total = hitori->board[iter.x][i].num; /* Take a number from the row */
					else
						total = hitori->board[i][iter.y].num; /* Take a number from the column */
				} while (total == 0);

				hitori->board[iter.x][iter.y].num = total;
				hitori->board[iter.x][iter.y].status &= (~CELL_PAINTED & ~CELL_ERROR);
			}
		}
	}

	/* Update things */
	hitori_enable_events (hitori);
}
