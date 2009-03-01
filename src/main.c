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
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
/* TODO: Remove me (below) */
#include <stdlib.h>
#include <math.h>
#include <config.h>
#include <gtk/gtk.h>

#include "main.h"
#include "interface.h"

gboolean
hitori_check_rule1 (Hitori *hitori)
{
	guint x, y;
	gboolean accum[BOARD_SIZE];

	/* Check columns for repeating numbers */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if (hitori->board[x][y].painted == FALSE) {
				if (accum[hitori->board[x][y].num-1] == TRUE)
					return FALSE;

				accum[hitori->board[x][y].num-1] = TRUE;
			}
		}

		/* Reset accum */
		for (y = 0; y < BOARD_SIZE; y++) {
			accum[y] = FALSE;
		}
	}

	/* Now check the rows */
	for (y = 0; y < BOARD_SIZE; y++) {
		for (x = 0; x < BOARD_SIZE; x++) {
			if (hitori->board[x][y].painted == FALSE) {
				if (accum[hitori->board[x][y].num-1] == TRUE)
					return FALSE;

				accum[hitori->board[x][y].num-1] = TRUE;
			}
		}

		/* Reset accum */
		for (y = 0; y < BOARD_SIZE; y++) {
			accum[y] = FALSE;
		}
	}

	g_message("Rule 1 OK");
	return TRUE;
}

gboolean
hitori_check_rule2 (Hitori *hitori)
{
	guint x, y;
return TRUE;
	/* Check the squares immediately below and to the right of the current one;
	 * if they're painted in, it's illegal. */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			if (hitori->board[x][y].painted == TRUE) {
				if ((x < BOARD_SIZE - 1 && hitori->board[x+1][y].painted == TRUE) ||
				    (y < BOARD_SIZE - 1 && hitori->board[x][y+1].painted == TRUE))
					return FALSE;
			}
		}
	}

	g_message("Rule 2 OK");
	return TRUE;
}

gboolean
hitori_check_rule3 (Hitori *hitori)
{
	guint groups[BOARD_SIZE][BOARD_SIZE];
	guint group_bases[BOARD_SIZE * BOARD_SIZE / 2];
	guint x = 0, y = 0;
	guint max_group = 0;
	GQueue *unchecked_cells_x, *unchecked_cells_y;

	/* Clear the groups */
	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			groups[x][y] = 0;
		}
	}

	/* HACKHACK! */
	unchecked_cells_x = g_queue_new ();
	unchecked_cells_y = g_queue_new ();

	/* Find the first unpainted cell */
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
		if (y >= 1 && groups[x][y-1] == 0 && hitori->board[x][y-1].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y-1));
		}
		if (y + 1 < BOARD_SIZE && groups[x][y+1] == 0 && hitori->board[x][y+1].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y+1));
		}
		if (x >= 1 && groups[x-1][y] == 0 && hitori->board[x-1][y].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x-1));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y));
		}
		if (x + 1 < BOARD_SIZE && groups[x+1][y] == 0 && hitori->board[x+1][y].painted == FALSE) {
			g_queue_push_head (unchecked_cells_x, GUINT_TO_POINTER (x+1));
			g_queue_push_head (unchecked_cells_y, GUINT_TO_POINTER (y));
		}

		/* Fetch some new coordinates to spider */
		x = GPOINTER_TO_UINT (g_queue_pop_head (unchecked_cells_x));
		y = GPOINTER_TO_UINT (g_queue_pop_head (unchecked_cells_y));
	} while (GUINT_TO_POINTER (x) != NULL &&
		 GUINT_TO_POINTER (y) != NULL &&
		 g_queue_get_length (unchecked_cells_x) >= 0);

	/* TODO: Check groups */
	for (y = 0; y < BOARD_SIZE; y++) {
		for (x = 0; x < BOARD_SIZE; x++) {
			if (hitori->board[x][y].painted == FALSE)
				printf ("%u ", group_bases[groups[x][y]]);
			else
				printf ("X ");
		}
		printf ("\n");
	}
	g_message("Rule 3 OK");
	return TRUE;
}

void
hitori_quit (Hitori *hitori)
{
	g_free (hitori->window);
	g_free (hitori->builder);
	g_free (hitori);
}

int
main (int argc, char *argv[])
{
	Hitori *hitori;
	guint x, y;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	/* Setup */
	hitori = g_new (Hitori, 1);

	/* TODO: Generate a board */
	/*for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			hitori->board[x][y].num = ceil (rand () % (BOARD_SIZE + 1));
			if (hitori->board[x][y].num < 1)
				hitori->board[x][y].num = 1;
		}
	}*/
	guint temp_board[BOARD_SIZE][BOARD_SIZE] = {
		{4, 8, 1, 6, 3, 2, 5, 7},
		{3, 6, 7, 2, 1, 6, 5, 4},
		{2, 3, 4, 8, 2, 8, 6, 1},
		{4, 1, 6, 5, 7, 7, 3, 5},
		{7, 2, 3, 1, 8, 5, 1, 2},
		{3, 5, 6, 7, 3, 1, 8, 4},
		{6, 4, 2, 3, 5, 4, 7, 8},
		{8, 7, 1, 4, 2, 3, 5, 6}
	};

	for (x = 0; x < BOARD_SIZE; x++) {
		for (y = 0; y < BOARD_SIZE; y++) {
			hitori->board[x][y].num = temp_board[y][x];
		}
	}

	hitori_create_interface (hitori);
	gtk_widget_show (hitori->window);

	gtk_main ();
	return 0;
}
