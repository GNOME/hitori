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

#include <config.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "main.h"
#include "interface.h"
#include "generator.h"

void
hitori_print_board (Hitori *hitori)
{
	guint x, y;

	if (hitori->debug) {
		for (y = 0; y < BOARD_SIZE; y++) {
			for (x = 0; x < BOARD_SIZE; x++) {
				if (hitori->board[x][y].painted == FALSE)
					printf ("%u ", hitori->board[x][y].num);
				else
					printf ("X ");
			}
			printf ("\n");
		}
	}
}

void
hitori_quit (Hitori *hitori)
{
	if (gtk_main_level () > 0)
		gtk_main_quit ();

	if (hitori == NULL)
		exit (0);

	g_free (hitori);
}

int
main (int argc, char *argv[])
{
	Hitori *hitori;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	/* Setup */
	hitori = g_new (Hitori, 1);
	hitori->debug = TRUE; /* TODO: Make this a runtime parameter */

	hitori_generate_board (hitori);
	hitori_create_interface (hitori);
	gtk_widget_show (hitori->window);

	gtk_main ();
	return 0;
}
