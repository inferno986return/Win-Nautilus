/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* nautilus-file-conflict-dialog: dialog that handles file conflicts
   during transfer operations.

   Copyright (C) 2008, Cosimo Cecchi

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
   
   Authors: Cosimo Cecchi <cosimoc@gnome.org>
*/

#include <config.h>
#include "nautilus-file-conflict-dialog.h"

#include <string.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtkmessagedialog.h>
#include "nautilus-file.h"
#include "nautilus-icon-info.h"

struct _NautilusFileConflictDialogDetails
{
	/* conflicting objects */
	GFile *source;
	GFile *destination;
	GFile *dest_dir;
	
	/* UI objects */
	GtkWidget *titles_vbox;
	GtkWidget *first_hbox;
	GtkWidget *second_hbox;
	GtkWidget *expander;
	GtkWidget *entry;
	GtkWidget *checkbox;
	GtkWidget *rename_button;
};

G_DEFINE_TYPE (NautilusFileConflictDialog,
	       nautilus_file_conflict_dialog,
	       GTK_TYPE_DIALOG);

#define NAUTILUS_FILE_CONFLICT_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), NAUTILUS_TYPE_FILE_CONFLICT_DIALOG, NautilusFileConflictDialogDetails))

static void
build_dialog_appearance (NautilusFileConflictDialog *fcd)
{
	GtkDialog *dialog;
	gboolean source_is_dir,	dest_is_dir;
	NautilusFileConflictDialogDetails *details;
	char *primary_text, *secondary_text, *primary_markup;
	char *src_name, *dest_name, *dest_dir_name;
	char *label_text;
	char *size, *date, *type;
	GdkPixbuf *pixbuf;
	GtkWidget *image, *label, *button;
	NautilusFile *src, *dest, *dest_dir;
	
	dialog = GTK_DIALOG (fcd);
	details = fcd->details;

	src = nautilus_file_get (details->source);
	dest = nautilus_file_get (details->destination);
	dest_dir = nautilus_file_get (details->dest_dir);

	src_name = nautilus_file_get_display_name (src);
	dest_name = nautilus_file_get_display_name (dest);
	dest_dir_name = nautilus_file_get_display_name (dest_dir);
	
	source_is_dir = nautilus_file_is_directory (src);
	dest_is_dir = nautilus_file_is_directory (dest);

	/* Set up the right labels */
	if (dest_is_dir) {
		if (source_is_dir) {
			primary_text = g_strdup_printf
				(_("A folder named \"%s\" already exists.  "
				   "Do you want to merge the source folder?"),
				 dest_name);
			secondary_text = g_strdup_printf
				(_("The source folder already exists in \"%s\".  "
				   "Merging will ask for confirmation before "
				   "replacing any files in the folder that "
				   "conflict with the files being copied."),
				 dest_dir_name);
		} else {
			primary_text = g_strdup_printf
				(_("A folder named \"%s\" already exists.  "
				   "Do you want to replace it?"), 
				 dest_name);
			secondary_text = g_strdup_printf
				(_("The folder already exists in \"%s\".  "
				   "Replacing it will remove all files in the folder."),
				 dest_dir_name);
		}
	} else {
		primary_text = g_strdup_printf
			(_("A file named \"%s\" already exists.  "
			   "Do you want to replace it?"),
			 dest_name);
		secondary_text = g_strdup_printf
			(_("The file already exists in \"%s\".  "
			   "Replacing it will overwrite its content."),
			 dest_dir_name);
	}

	label = gtk_label_new (NULL);
	primary_markup = g_strconcat ("<b>", primary_text, "</b>", NULL);
	gtk_label_set_markup (GTK_LABEL (label), primary_markup);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (details->titles_vbox),
			    label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	label = gtk_label_new (secondary_text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (details->titles_vbox),
			    label, TRUE, TRUE, 0);
	gtk_widget_show (label);
	g_free (primary_text);
	g_free (primary_markup);
	g_free (secondary_text);

	/* Set up file icons */
	pixbuf = nautilus_file_get_icon_pixbuf (dest,
						NAUTILUS_ICON_SIZE_LARGE,
						TRUE,
						NAUTILUS_FILE_ICON_FLAGS_USE_THUMBNAILS);
	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_box_pack_start (GTK_BOX (details->first_hbox),
			    image, FALSE, FALSE, 0);
	gtk_widget_show (image);
	g_object_unref (pixbuf);

	pixbuf = nautilus_file_get_icon_pixbuf (src,
						NAUTILUS_ICON_SIZE_LARGE,
						TRUE,
						NAUTILUS_FILE_ICON_FLAGS_USE_THUMBNAILS);
	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_box_pack_start (GTK_BOX (details->second_hbox),
			    image, FALSE, FALSE, 0);
	gtk_widget_show (image);
	g_object_unref (pixbuf);

	/* Set up labels */
	label = gtk_label_new (NULL);
	date = nautilus_file_get_string_attribute (dest,
						   "date_modified");
	size = nautilus_file_get_string_attribute (dest, "size");
	type = nautilus_file_get_string_attribute (dest, "type");
	label_text = g_markup_printf_escaped (_("<b>Original file</b>\n"
						"<i>Size:</i> %s\n"
						"<i>Type:</i> %s\n"
						"<i>Last modified:</i> %s"),
						size,
						type,
						date);
	gtk_label_set_markup (GTK_LABEL (label),
			      label_text);
	gtk_box_pack_start (GTK_BOX (details->first_hbox),
			    label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	g_free (size);
	g_free (type);
	g_free (date);
	g_free (label_text);
	
	label = gtk_label_new (NULL);
	date = nautilus_file_get_string_attribute (src,
						   "date_modified");
	size = nautilus_file_get_string_attribute (src, "size");
	type = nautilus_file_get_string_attribute (src, "type");
	label_text = g_markup_printf_escaped (_("<b>Replace with</b>\n"
						"<i>Size:</i> %s\n"
						"<i>Type:</i> %s\n"
						"<i>Last modified:</i> %s"),
						size,
						type,
						date);
	gtk_label_set_markup (GTK_LABEL (label),
			      label_text);
	gtk_box_pack_start (GTK_BOX (details->second_hbox),
			    label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	g_free (size);
	g_free (date);
	g_free (label_text);
	
	/* Add buttons */
	gtk_dialog_add_buttons (dialog,
				GTK_STOCK_CANCEL,
				GTK_RESPONSE_CANCEL,
				_("_Skip"),
				CONFLICT_RESPONSE_SKIP,
				NULL);
	button = gtk_dialog_add_button (dialog,
					_("Re_name"),
					CONFLICT_RESPONSE_RENAME);
	gtk_widget_set_sensitive (button,
				  FALSE);
	details->rename_button = button;
	gtk_dialog_add_button (dialog,
			       (source_is_dir && dest_is_dir) ?
			       _("_Merge") : _("_Replace"),
			       CONFLICT_RESPONSE_REPLACE);
}

static void
set_source_and_destination (GtkWidget *w,
			    GFile *source,
			    GFile *destination,
			    GFile *dest_dir)
{
	NautilusFileConflictDialog *dialog;
	NautilusFileConflictDialogDetails *details;

	dialog = NAUTILUS_FILE_CONFLICT_DIALOG (w);
	details = dialog->details;

	details->source = source;
	details->destination = destination;
	details->dest_dir = dest_dir;

	build_dialog_appearance (dialog);
}

static void
entry_text_notify_cb (GtkEntry *entry,
		      GParamSpec *pspec,
		      NautilusFileConflictDialog *dialog)
{
	NautilusFileConflictDialogDetails *details;

	details = dialog->details;

	/* The rename button is sensitive only if there's text
	 * in the entry.
	 */
	gtk_widget_set_sensitive (details->rename_button,
				  strcmp (gtk_entry_get_text (GTK_ENTRY (details->entry)),
					  ""));
}

static void
checkbox_toggled_cb (GtkToggleButton *t,
		     NautilusFileConflictDialog *dialog)
{
	NautilusFileConflictDialogDetails *details;

	details = dialog->details;

	gtk_widget_set_sensitive (details->expander,
				  !gtk_toggle_button_get_active (t));
	gtk_widget_set_sensitive (details->rename_button,
				  !gtk_toggle_button_get_active (t));
}

static void
nautilus_file_conflict_dialog_init (NautilusFileConflictDialog *fcd)
{
	GtkWidget *hbox, *vbox;
	GtkWidget *widget;
	NautilusFileConflictDialogDetails *details;
	GtkDialog *dialog;

	details = fcd->details = NAUTILUS_FILE_CONFLICT_DIALOG_GET_PRIVATE (fcd);
	dialog = GTK_DIALOG (fcd);

	/* Setup the main hbox */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dialog->vbox),
			    hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_widget_show (hbox);

	/* Setup the dialog image */
	widget = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
					   GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start (GTK_BOX (hbox),
			    widget, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (widget), 0.5, 0.0);
	gtk_widget_show (widget);

	/* Setup the vbox containing the dialog body */
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (hbox),
			    vbox, FALSE, FALSE, 0);
	gtk_widget_show (vbox);

	/* Setup the vbox for the dialog labels */
	widget = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox),
			    widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);
	details->titles_vbox = widget;

	/* Setup the hboxes to pack file infos into */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox),
			    hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	details->first_hbox = hbox;
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox),
			    hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);
	details->second_hbox = hbox;

	/* Setup the checkbox to apply the action to all files */
	widget = gtk_check_button_new_with_mnemonic (_("Apply this action to all files"));
	gtk_box_pack_start (GTK_BOX (vbox),
			    widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);
	details->checkbox = widget;
	g_signal_connect (widget, "toggled",
			  G_CALLBACK (checkbox_toggled_cb),
			  dialog);

	/* Setup the expander for the rename action */
	widget = gtk_expander_new_with_mnemonic (_("_Select a new name for the destination"));
	gtk_box_pack_start (GTK_BOX (vbox),
			    widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);
	details->expander = widget;
	widget = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (details->expander),
			   widget);
	gtk_widget_show (widget);
	details->entry = widget;
	g_signal_connect_object (widget, "notify::text",
				 G_CALLBACK (entry_text_notify_cb),
				 dialog, 0);

	/* Setup HIG properties */
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);		
	gtk_box_set_spacing (GTK_BOX (dialog->vbox), 14);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_dialog_set_has_separator (dialog, FALSE);
}

static void
nautilus_file_conflict_dialog_class_init (NautilusFileConflictDialogClass *klass)
{
	g_type_class_add_private (klass, sizeof (NautilusFileConflictDialogDetails));
}

char *
nautilus_file_conflict_dialog_get_new_name (NautilusFileConflictDialog *dialog)
{
	return g_strdup (gtk_entry_get_text
			 (GTK_ENTRY (dialog->details->entry)));
}

gboolean
nautilus_file_conflict_dialog_get_apply_to_all (NautilusFileConflictDialog *dialog)
{
	return gtk_toggle_button_get_active 
		(GTK_TOGGLE_BUTTON (dialog->details->checkbox));
}

GtkWidget *
nautilus_file_conflict_dialog_new (GtkWindow *parent,
				   GFile *source,
				   GFile *destination,
				   GFile *dest_dir)
{
	GtkWidget *dialog;
	
	dialog = GTK_WIDGET (g_object_new (NAUTILUS_TYPE_FILE_CONFLICT_DIALOG,
					   "title", _("File conflict"),
					   NULL));
	set_source_and_destination (dialog,
				    source,
				    destination,
				    dest_dir);
	gtk_window_set_transient_for (GTK_WINDOW (dialog),
				      parent);
	return dialog;
}