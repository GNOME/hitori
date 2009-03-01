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
#include <stdlib.h>
#include "main.h"
#include "generator.h"
#include "rules.h"

void
hitori_generate_board (Hitori *hitori, guint new_board_size, gint seed)
{
	guint i, total, old_total, x, y;
	gboolean *accum, **horiz_accum;

	/* Seed the random number generator */
	if (seed == -1)
		seed = time (0);
	if (hitori->debug)
		g_debug ("Seed value: %u", seed);

	srand (seed);

	/* Deallocate any previous board */
	if (hitori->board != NULL) {
		for (i = 0; i < BOARD_SIZE; i++)
			g_free (hitori->board[i]);
		g_free (hitori->board);
	}

	hitori->board_size = new_board_size;

	accum = g_malloc0 (sizeof (gboolean) * (BOARD_SIZE + 2)); /* Stores which numbers have been used in the current column */
	horiz_accum = g_malloc (sizeof (gboolean*) * BOARD_SIZE); /* Stores which numbers have been used in each row */
	for (x = 0; x < BOARD_SIZE; x++)
		horiz_accum[x] = g_malloc0 (sizeof (gboolean) * (BOARD_SIZE + 2));

	/* Allocate the board */
	hitori->board = g_malloc (sizeof (HitoriCell*) * BOARD_SIZE);
	for (i = 0; i < BOARD_SIZE; i++)
		hitori->board[i] = g_malloc0 (sizeof (HitoriCell) * BOARD_SIZE);

	/* Clear the board */
	/*for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			hitori->board[x][y].painted = FALSE;
			hitori->board[x][y].tag1 = FALSE;
			hitori->board[x][y].tag2 = FALSE;
			hitori->board[x][y].should_be_painted = FALSE;
			hitori->board[x][y].num = 0;
		}
	}*/

	/* TODO: Different ranges for different grid sizes */
	/* Generate some randomly-placed painted cells */
	total = rand () % 5 + 13; /* Total number of painted cells (between 14 and 18 inclusive) */
	/* For the moment, I'm hardcoding the range in the number of painted
	 * cells, and only specifying it for 8x8 grids. This will change in the
	 * future. */
	for (i = 0; i < total; i++) {
		do {
			x = rand () % BOARD_SIZE;
			y = rand () % BOARD_SIZE;

			if (y >= 1 && hitori->board[x][y-1].painted == FALSE &&
			    y + 1 < BOARD_SIZE && hitori->board[x][y+1].painted == FALSE &&
			    x >= 1 && hitori->board[x-1][y].painted == FALSE &&
			    x + 1 < BOARD_SIZE && hitori->board[x+1][y].painted == FALSE)
				break;
		} while (TRUE);

		hitori->board[x][y].painted = TRUE;
		hitori->board[x][y].should_be_painted = TRUE;
	}

	/* Check that the painted squares don't mess everything up */
	if (hitori_check_rule2 (hitori) == FALSE ||
	    hitori_check_rule3 (hitori) == FALSE)
		return hitori_generate_board (hitori, BOARD_SIZE, seed + 1);

	/* Fill in the squares, leaving the painted ones blank,
	 * and making sure not to repeat any previous numbers. */
	for (x = 0; x < BOARD_SIZE; x++) {
		/* Reset the vertical accumulator */
		for (y = 1; y < BOARD_SIZE + 2; y++)
			accum[y] = FALSE;

		i = 0;
		accum[0] = TRUE;
		total = BOARD_SIZE + 1;
		old_total = total;

		for (y = 0; y < BOARD_SIZE; y++) {
			if (hitori->board[x][y].painted == FALSE) {
				while (accum[i] == TRUE || horiz_accum[y][i] == TRUE) {
					if (horiz_accum[y][i] == TRUE && accum[i] == FALSE)
						total--;

					if (total < 1)
						return hitori_generate_board (hitori, BOARD_SIZE, seed + 1); /* We're buggered */

					i = rand () % (BOARD_SIZE + 1) + 1;
				}

				accum[i] = TRUE;
				horiz_accum[y][i] = TRUE;
				total = old_total;
				total--;

				hitori->board[x][y].num = i;
			}
		}
	}

	g_free (accum);
	for (x = 0; x < BOARD_SIZE; x++)
		g_free (horiz_accum[x]);
	g_free (horiz_accum);

	/* Fill in the painted squares, making sure they duplicate a number
	 * already in the column/row. */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if (hitori->board[x][y].painted == TRUE) {
				do {
					i = rand () % BOARD_SIZE;
					if (x > y)
						total = hitori->board[x][i].num; /* Take a number from the row */
					else
						total = hitori->board[i][y].num; /* Take a number from the column */
				} while (total == 0);

				hitori->board[x][y].num = total;
				hitori->board[x][y].painted = FALSE;
			}
		}
	}

	/* Update things */
	hitori_enable_events (hitori);
}
