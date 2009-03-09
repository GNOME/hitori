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

					/* Set the error position */
					hitori_set_error_position (hitori, iter);

					return FALSE;
				}
			}
		}
	}

	if (hitori->debug)
		g_debug ("Rule 2 OK");

	/* Clear the error */
	hitori->display_error = FALSE;

	return TRUE;
}

/* Rule 3: all the unpainted cells must be joined together in one group. */
gboolean
hitori_check_rule3 (Hitori *hitori)
{
	HitoriVector iter;
	guint i, max_group, *group_bases, **groups;

	max_group = 0;

	groups = g_new (guint*, hitori->board_size);
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		groups[iter.x] = g_new0 (guint, hitori->board_size);
	group_bases = g_new0 (guint, hitori->board_size * hitori->board_size / 2);

	/* Loop through each cell assigning it a group, and gradually merging the
	 * groups until either there's only one group of unpainted cells left
	 * (the rule is satisfied), or there are several (the rule is broken).
	 * We only look at the cells above and to the left of the current one, as
	 * this eliminates a lot of lookups, yet we still examine the relationship
	 * between each pair of contiguous cells. */
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			guint *this_group, up_group, left_group;

			/* To save lots of lookups in the following code,
			 * get the local groups up now. If the indices are invalid,
			 * they're set to 0 so that some checks below can be removed. */
			this_group = &groups[iter.x][iter.y];
			up_group = (iter.y >= 1) ? groups[iter.x][iter.y - 1] : 0;
			left_group = (iter.x >= 1) ? groups[iter.x - 1][iter.y] : 0;

			if (hitori->board[iter.x][iter.y].status & CELL_PAINTED) {
				/* If it's painted ensure it's in group 0 */
				*this_group = 0;
			} else {
				/* Try and apply a group from a surrounding cell */
				if (up_group != 0)
					*this_group = up_group;
				else if (left_group != 0)
					*this_group = left_group;
				else {
					/* Create a new group */
					max_group++;
					group_bases[max_group] = max_group;
					*this_group = max_group;
				}

				/* Check for converged groups */
				if (up_group != 0 && group_bases[up_group] != group_bases[*this_group])
					group_bases[*this_group] = group_bases[up_group];
				else if (left_group != 0 && group_bases[left_group] != group_bases[*this_group])
					group_bases[*this_group] = group_bases[left_group];
			}
		}
	}

	if (hitori->debug) {
		/* Print out the groups */
		for (iter.y = 0; iter.y < hitori->board_size; iter.y++) {
			for (iter.x = 0; iter.x < hitori->board_size; iter.x++) {
				if (hitori->board[iter.x][iter.y].status & CELL_PAINTED)
					g_printf ("X ");
				else
					g_printf ("%u ", groups[iter.x][iter.y]);
			}
			g_printf ("\n");
		}
	}

	/* Check that there's only one group; if there's more than
	 * one, the rule fails. Start at 2 to give room to subtract
	 * 1 without underflowing, and also skip the first entry, as
	 * it's reserved for ungrouped cells. */
	for (i = 2; i < max_group + 1; i++) {
		if (group_bases[i - 1] != group_bases[i]) {
			/* Rule failed */
			g_free (group_bases);
			for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
				g_free (groups[iter.x]);
			g_free (groups);

			if (hitori->debug)
				g_debug ("Rule 3 failed");

			return FALSE;
		}
	}

	/* Free everything */
	g_free (group_bases);
	for (iter.x = 0; iter.x < hitori->board_size; iter.x++)
		g_free (groups[iter.x]);
	g_free (groups);

	if (hitori->debug)
		g_debug ("Rule 3 OK");

	return TRUE;
}
