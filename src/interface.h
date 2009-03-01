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

#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include "main.h"

#ifndef HITORI_INTERFACE_H
#define HITORI_INTERFACE_H

G_BEGIN_DECLS

#define FONT_SCALE 0.9
#define TAG_OFFSET 0.75
#define TAG_RADIUS 0.25
#define HINT_FLASHES 6
#define HINT_INTERVAL 500

GtkWidget* hitori_create_interface (Hitori *hitori);
void hitori_draw_board (Hitori *hitori, cairo_t *cr, gboolean check_win);
void hitori_draw_board_simple (Hitori *hitori, gboolean check_win, gboolean clear_first);

G_END_DECLS

#endif /* HITORI_INTERFACE_H */
