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

#include "pp-app-printer-dialog.h"
#include <glib-object.h>
#include <ctype.h>
#include <curl/curl.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include "pp-printer.h"
#include "pp-new-printer-dialog.h"
#include "pp-host.h"
#include "pp-samba.h"
#include "pp-utils.h"
#include "pp-ppd-selection-dialog.h"
#include "pp-printer-app-selection-dialog.h"

#define _(String) gettext(String)

#define MAX_DUMMY_APPS 10

typedef struct
{
        char                *avahi_service_browser_path;
        guint                avahi_service_browser_subscription_id;
        guint                avahi_service_type_browser_subscription_id;
        guint                unsubscribe_general_subscription_id;
        guint                done,done_1,done_2,done_3,done_4;
        GDBusConnection     *dbus_connection;
        GCancellable        *avahi_cancellable;
        GList               *system_objects;
        GMainLoop           *loop;
        gpointer             user_data;
        char*                service_type;
} Avahi;

typedef struct
{
        GList                *services;
        gchar                *location;
        gchar                *address;
        gchar                *hostname;
        gchar                *name;
        gchar                *resource_path;
        gchar                *type;
        gchar                *domain;
        gchar                *UUID;
        gchar                *object_type;
        gchar                *admin_url;
        gchar                *uri;
        gchar                *objAttr;
        gint64               printer_type,
                             printer_state;
        gboolean             got_printer_state,
                             got_printer_type;
        int                  port;
        int                  family;
        gpointer             user_data;
} AvahiData;


struct _PpNewPrinterDialog
{
  AdwWindow parent_instance;

  GPtrArray *local_cups_devices;

  GtkListStore       *devices_liststore;
  GtkTreeModelFilter *devices_model_filter;

  /* headerbar */
  AdwWindowTitle       *header_title;

  /* headerbar topleft buttons */
  GtkStack           *headerbar_topleft_buttons;
  GtkButton          *go_back_button;

  /* headerbar topright buttons */
  GtkStack           *headerbar_topright_buttons;
  GtkButton          *new_printer_add_button;
  GtkButton          *unlock_button;
  GtkButton          *authenticate_button;
  /* end headerbar */

  /* dialogstack */
  GtkStack           *dialog_stack;
  GtkStack           *stack;

  /* scrolledwindow1 */
  GtkScrolledWindow  *scrolledwindow1;
  GtkTreeView        *devices_treeview;

  GtkEntry           *search_entry;

  /* authentication page */
  GtkLabel           *authentication_title;
  GtkLabel           *authentication_text;
  GtkEntry           *username_entry;
  GtkEntry           *password_entry;
  /* end dialog stack */

  UserResponseCallback user_callback;
  gpointer             user_data;
  gpointer             ipp_data;

  cups_dest_t *dests;
  gint         num_of_dests;

  GCancellable *cancellable;
  GCancellable *remote_host_cancellable;

  gboolean  cups_searching;
  gboolean  samba_authenticated_searching;
  gboolean  samba_searching;

  PpPPDSelectionDialog *ppd_selection_dialog;

  PpPrinterAppSelectionDialog *printer_app_selection_dialog;

  PpAppPrinterDialog *app_printer_dialog;

  PpPrintDevice *new_device;

  PPDList *list;

  GIcon *local_printer_icon;
  GIcon *remote_printer_icon;
  GIcon *authenticated_server_icon;

  PpHost  *snmp_host;
  PpHost  *socket_host;
  PpHost  *lpd_host;
  PpHost  *remote_cups_host;
  PpSamba *samba_host;
  guint    host_search_timeout_id;
};

struct MemoryStruct
{
  char *memory;
  size_t size;
};

typedef struct {
    char *name;
    char *id;
    char *description;
    gboolean is_available;
} PrinterApp;

typedef struct {
    GArray *printer_apps;  // Array of PrinterApp structures
} AppsList;

// In your header file (pp-app-printer-dialog.h)
typedef struct _PpAppPrinterDialog PpAppPrinterDialog;
//typedef struct _PpAppPrinterDialogClass PpAppPrinterDialogClass;

struct _PpAppPrinterDialog
{
  GtkDialog parent;

  // Template widgets
  GtkCheckButton *installed_printer_radio;
  GtkCheckButton *internet_printer_radio;
  GtkCheckButton *legacy_driver_radio;
  GtkDropDown    *printer_app_dropdown;
  GtkButton      *web_interface_button;
  GtkButton      *setup_button;

  // Private data
  AppsList       *list;
  PpPrintDevice  *device;
  GtkStringList  *string_list;
  Avahi          *ipp_data;
  AvahiData      *selected_app;
  PpNewPrinterDialog *data;
};

struct _PpAppPrinterDialogClass
{
  GtkDialogClass parent_class;
};

//G_DECLARE_FINAL_TYPE (PpAppPrinterDialog, pp_app_printer_dialog, PP, APP_PRINTER_DIALOG, GtkDialog)

// In your implementation file (pp-app-printer-dialog.c)
G_DEFINE_TYPE (PpAppPrinterDialog, pp_app_printer_dialog, GTK_TYPE_DIALOG)

void
pp_app_printer_dialog_clear_printer_apps(PpAppPrinterDialog *self)
{
    g_return_if_fail(PP_IS_APP_PRINTER_DIALOG(self));

    if (self->list && self->list->printer_apps) {
        for (guint i = 0; i < self->list->printer_apps->len; i++) {
            PrinterApp *app = &g_array_index(self->list->printer_apps, PrinterApp, i);
            g_free(app->name);
            g_free(app->id);
            g_free(app->description);
        }
        g_array_free(self->list->printer_apps, TRUE);
        self->list->printer_apps = g_array_new(FALSE, TRUE, sizeof(PrinterApp));
    }

    if (self->string_list != NULL) {
        GtkStringList *new_model = gtk_string_list_new(NULL);
        gtk_drop_down_set_model(self->printer_app_dropdown, G_LIST_MODEL(new_model));
        g_object_unref(self->string_list);
        self->string_list = new_model;
    }
}

static void
load_avahi_printer_apps(PpAppPrinterDialog *self)
{
    g_return_if_fail(PP_IS_APP_PRINTER_DIALOG(self));
    g_return_if_fail(self->ipp_data != NULL);

    if (self->string_list != NULL) {
        GtkStringList *new_model = gtk_string_list_new(NULL);
        gtk_drop_down_set_model(self->printer_app_dropdown, G_LIST_MODEL(new_model));
        g_object_unref(self->string_list);
        self->string_list = new_model;
    }

    gboolean printers_found = FALSE;
    Avahi *avahi_data = (Avahi *)self->ipp_data;

    for (GList *l = avahi_data->system_objects; l != NULL; l = l->next) {
        AvahiData *data = (AvahiData *)l->data;
        if (data && data->name && data->object_type && g_strcmp0 (data->object_type, "SYSTEM_OBJECT") == 0) {
                gtk_string_list_append(self->string_list, g_strdup(data->name));
                printers_found = TRUE;
        }
    }

    if (!printers_found) {
        gtk_string_list_append(self->string_list, _("No Printer App found"));
        gtk_widget_set_sensitive(GTK_WIDGET(self->setup_button), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(self->web_interface_button), FALSE);
    }

    gtk_drop_down_set_selected(self->printer_app_dropdown, 0);
}

static void
on_dropdown_changed(GtkDropDown *dropdown,
                   GParamSpec  *pspec,
                   gpointer     user_data)
{
    PpAppPrinterDialog *self = PP_APP_PRINTER_DIALOG(user_data);
    guint selected = gtk_drop_down_get_selected(dropdown);

    if (selected == GTK_INVALID_LIST_POSITION) {
        return;
    }

    // Get the selected printer name from the string list
    const gchar *selected_name = gtk_string_list_get_string(self->string_list, selected);
    if (g_strcmp0(selected_name, _("No printers found")) == 0) {
        gtk_widget_set_sensitive(GTK_WIDGET(self->setup_button), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(self->web_interface_button), FALSE);
        return;
    }

    for (GList *l = self->ipp_data->system_objects; l != NULL; l = l->next) {
        AvahiData *data = (AvahiData *)l->data;
        if (data && data->name && data->object_type != NULL && g_strcmp0 (data->object_type, "SYSTEM_OBJECT") == 0) {
            // Create the same display name format as in the dropdown
            gchar *display_name = g_strdup(data->name);;
            self->selected_app = data;

            if (g_strcmp0(display_name, selected_name) == 0) {
                gtk_widget_set_sensitive(GTK_WIDGET(self->setup_button),
                    (data->address != NULL && data->port > 0));

                gtk_widget_set_sensitive(GTK_WIDGET(self->web_interface_button),
                    (data->admin_url != NULL));

                self->selected_app = data;
                g_free(display_name);
                break;
            }
            g_free(display_name);
        }
    }
}

static void
pp_app_printer_dialog_class_init(PpAppPrinterDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(widget_class,
                                              "/org/gnome/control-center/printers/app-printer-dialog.ui");

    gtk_widget_class_bind_template_child(widget_class, PpAppPrinterDialog, installed_printer_radio);
    gtk_widget_class_bind_template_child(widget_class, PpAppPrinterDialog, internet_printer_radio);
    gtk_widget_class_bind_template_child(widget_class, PpAppPrinterDialog, legacy_driver_radio);
    gtk_widget_class_bind_template_child(widget_class, PpAppPrinterDialog, printer_app_dropdown);
    gtk_widget_class_bind_template_child(widget_class, PpAppPrinterDialog, web_interface_button);
    gtk_widget_class_bind_template_child(widget_class, PpAppPrinterDialog, setup_button);

}

static void
pp_app_printer_dialog_init(PpAppPrinterDialog *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    self->string_list = gtk_string_list_new(NULL);
    gtk_drop_down_set_model(self->printer_app_dropdown, G_LIST_MODEL(self->string_list));

    g_signal_connect(self->printer_app_dropdown, "notify::selected",
                    G_CALLBACK(on_dropdown_changed), self);

    // Load initial dummy data
    //load_avahi_printer_apps(self);
}

void
setup_printer_cb(){

}

void
on_printer_option_changed(){

}

void
on_printer_app_changed(){

}

void
open_web_interface(PpAppPrinterDialog *self)
{
    GtkUriLauncher *launcher = gtk_uri_launcher_new(self->selected_app->admin_url);
    gtk_uri_launcher_launch(launcher, NULL, NULL, NULL, NULL);
    g_object_unref(launcher);
    return;
}

PpAppPrinterDialog *
pp_app_printer_dialog_new(PpPrintDevice *device , gpointer ipp_data)
{
    PpAppPrinterDialog *self;

    self = g_object_new(PP_TYPE_APP_PRINTER_DIALOG,
                       "use-header-bar", TRUE,
                       NULL);
    self->ipp_data = (Avahi *)ipp_data;

    self->device = device;
    self->selected_app = NULL;

    g_signal_connect_swapped(self->web_interface_button, "clicked",
                                G_CALLBACK(open_web_interface),
                                self);

    g_signal_connect_object (self->setup_button,
                            "clicked",
                            G_CALLBACK (setup_printer_cb),
                            self,
                            G_CONNECT_SWAPPED);

    g_signal_connect_object (self->installed_printer_radio,
                            "toggled",
                            G_CALLBACK (on_printer_option_changed),
                            self,
                            G_CONNECT_SWAPPED);

    g_signal_connect_object (self->internet_printer_radio,
                            "toggled",
                            G_CALLBACK (on_printer_option_changed),
                            self,
                            G_CONNECT_SWAPPED);

    g_signal_connect_object (self->legacy_driver_radio,
                            "toggled",
                            G_CALLBACK (on_printer_option_changed),
                            self,
                            G_CONNECT_SWAPPED);

    g_signal_connect_object (self->printer_app_dropdown,
                            "changed",
                            G_CALLBACK (on_printer_app_changed),
                            self,
                            G_CONNECT_SWAPPED);
    load_avahi_printer_apps(self);

    return self;
}
