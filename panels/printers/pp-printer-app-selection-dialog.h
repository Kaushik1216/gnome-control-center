/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2009-2010  Red Hat, Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <gtk/gtk.h>
#include "pp-utils.h"

G_BEGIN_DECLS

#define PP_TYPE_PRINTER_APP_SELECTION_DIALOG (pp_printer_app_selection_dialog_get_type ())
G_DECLARE_FINAL_TYPE (PpPrinterAppSelectionDialog, pp_printer_app_selection_dialog, PP, PRINTER_APP_SELECTION_DIALOG, GtkDialog)

PpPrinterAppSelectionDialog *pp_printer_app_selection_dialog_new   ();
gchar                *pp_printer_app_selection_dialog_get_ppd_name         (PpPrinterAppSelectionDialog      *dialog);
gchar                *pp_printer_app_selection_dialog_get_ppd_display_name (PpPrinterAppSelectionDialog      *dialog);
void                  pp_printer_app_selection_dialog_set_ppd_list         (PpPrinterAppSelectionDialog      *dialog,
                                                                            PPDList                   *list);

G_END_DECLS
