/*
 * Copyright (C) 2010 Red Hat, Inc
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
#include "pp-print-device.h"

G_BEGIN_DECLS

#define PP_TYPE_APP_PRINTER_DIALOG (pp_app_printer_dialog_get_type ())
G_DECLARE_FINAL_TYPE (PpAppPrinterDialog, pp_app_printer_dialog, PP, APP_PRINTER_DIALOG, GtkDialog)

PpAppPrinterDialog *pp_app_printer_dialog_new               (PpPrintDevice *device);

G_END_DECLS
