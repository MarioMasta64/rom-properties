/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * DosAttrView.c: MS-DOS file system attribute viewer widget.              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "DosAttrView.h"

// MS-DOS and Windows attributes
// NOTE: Does not depend on the Windows SDK.
#include "librpfile/xattr/dos_attrs.h"

/* Property identifiers */
typedef enum {
	PROP_0,

	PROP_ATTRS,

	PROP_LAST
} RpDosAttrViewPropID;

static void	rp_dos_attr_view_set_property(GObject		*object,
					      guint		 prop_id,
					      const GValue	*value,
					      GParamSpec	*pspec);
static void	rp_dos_attr_view_get_property(GObject		*object,
					      guint		 prop_id,
					      GValue		*value,
					      GParamSpec	*pspec);

/** Signal handlers **/
static void	checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
						  RpDosAttrView		*widget);

/** Update attributes display **/
static void	rp_dos_attr_view_update_attrs_display(RpDosAttrView *widget);

static GParamSpec *props[PROP_LAST];

static GQuark DosAttrView_value_quark;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#define USE_GTK_GRID 1	// Use GtkGrid instead of GtkTable.
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// DosAttrView class
struct _RpDosAttrViewClass {
	superclass __parent__;
};

// DosAttrView instance
struct _RpDosAttrView {
	super __parent__;

	unsigned int attrs;

	// Inhibit checkbox toggling while updating.
	gboolean inhibit_checkbox_no_toggle;

	// Attribute checkboxes
	GtkWidget *chkReadOnly;
	GtkWidget *chkHidden;
	GtkWidget *chkArchive;
	GtkWidget *chkSystem;
	GtkWidget *chkCompressed;
	GtkWidget *chkEncrypted;
};

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpDosAttrView, rp_dos_attr_view,
	GTK_TYPE_SUPER, (GTypeFlags)0, {});

static void
rp_dos_attr_view_class_init(RpDosAttrViewClass *klass)
{
	GObjectClass *const gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = rp_dos_attr_view_set_property;
	gobject_class->get_property = rp_dos_attr_view_get_property;

	/** Quarks **/

	// NOTE: Not using g_quark_from_static_string()
	// because the extension can be unloaded.
	DosAttrView_value_quark = g_quark_from_string("DosAttrValue.value");

	/** Properties **/

	props[PROP_ATTRS] = g_param_spec_uint(
		"attrs", "attrs", "MS-DOS file attributes",
		0U, ~0U, 0U,
		(GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// Install the properties.
	g_object_class_install_properties(gobject_class, PROP_LAST, props);
}

static void
rp_dos_attr_view_init(RpDosAttrView *widget)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// Checkboxes: DOS attributes
	GtkWidget *const hboxDOSAttrs = rp_gtk_hbox_new(4);
	gtk_widget_set_name(hboxDOSAttrs, "hboxDOSAttrs");

	widget->chkReadOnly = gtk_check_button_new_with_label(C_("DosAttrView", "Read-only"));
	gtk_widget_set_name(widget->chkReadOnly, "chkReadOnly");
	widget->chkHidden = gtk_check_button_new_with_label(C_("DosAttrView", "Hidden"));
	gtk_widget_set_name(widget->chkHidden, "chkHidden");
	widget->chkArchive = gtk_check_button_new_with_label(C_("DosAttrView", "Archive"));
	gtk_widget_set_name(widget->chkArchive, "chkArchive");
	widget->chkSystem = gtk_check_button_new_with_label(C_("DosAttrView", "System"));
	gtk_widget_set_name(widget->chkSystem, "chkSystem");

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(hboxDOSAttrs), widget->chkReadOnly);
	gtk_box_append(GTK_BOX(hboxDOSAttrs), widget->chkHidden);
	gtk_box_append(GTK_BOX(hboxDOSAttrs), widget->chkArchive);
	gtk_box_append(GTK_BOX(hboxDOSAttrs), widget->chkSystem);
	gtk_box_append(GTK_BOX(widget), hboxDOSAttrs);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(hboxDOSAttrs), widget->chkReadOnly, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxDOSAttrs), widget->chkHidden, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxDOSAttrs), widget->chkArchive, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxDOSAttrs), widget->chkSystem, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), hboxDOSAttrs, FALSE, FALSE, 0);
	gtk_widget_show_all(hboxDOSAttrs);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Checkboxes: NTFS attributes
	GtkWidget *const hboxNTFSAttrs = rp_gtk_hbox_new(4);
	gtk_widget_set_name(hboxNTFSAttrs, "hboxNTFSAttrs");

	widget->chkCompressed = gtk_check_button_new_with_label(C_("DosAttrView", "Compressed"));
	gtk_widget_set_name(widget->chkReadOnly, "chkCompressed");
	widget->chkEncrypted = gtk_check_button_new_with_label(C_("DosAttrView", "Encrypted"));
	gtk_widget_set_name(widget->chkEncrypted, "chkEncrypted");

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(hboxNTFSAttrs), widget->chkCompressed);
	gtk_box_append(GTK_BOX(hboxNTFSAttrs), widget->chkEncrypted);
	gtk_box_append(GTK_BOX(widget), hboxNTFSAttrs);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(hboxNTFSAttrs), widget->chkCompressed, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hboxNTFSAttrs), widget->chkEncrypted, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(widget), hboxNTFSAttrs, FALSE, FALSE, 0);
	gtk_widget_show_all(hboxNTFSAttrs);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Disable user modifications.
	// NOTE: Unlike Qt, both the "clicked" and "toggled" signals are
	// emitted for both user and program modifications, so we have to
	// connect this signal *after* setting the initial value.
	g_signal_connect(widget->chkReadOnly, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);
	g_signal_connect(widget->chkHidden, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);
	g_signal_connect(widget->chkArchive, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);
	g_signal_connect(widget->chkSystem, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);
	g_signal_connect(widget->chkCompressed, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);
	g_signal_connect(widget->chkEncrypted, "toggled", G_CALLBACK(checkbox_no_toggle_signal_handler), widget);
}

GtkWidget*
rp_dos_attr_view_new(void)
{
	return (GtkWidget*)g_object_new(RP_TYPE_DOS_ATTR_VIEW, NULL);
}

/** Properties **/

static void
rp_dos_attr_view_set_property(GObject		*object,
				guint		 prop_id,
				const GValue	*value,
				GParamSpec	*pspec)
{
	RpDosAttrView *const widget = RP_DOS_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_ATTRS: {
			const unsigned int attrs = g_value_get_uint(value);
			if (widget->attrs != attrs) {
				widget->attrs = attrs;
				rp_dos_attr_view_update_attrs_display(widget);
			}
			break;
		}

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
rp_dos_attr_view_get_property(GObject		*object,
				guint		 prop_id,
				GValue		*value,
				GParamSpec	*pspec)
{
	RpDosAttrView *const widget = RP_DOS_ATTR_VIEW(object);

	switch (prop_id) {
		case PROP_ATTRS:
			g_value_set_uint(value, widget->attrs);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

/** Update attributes display **/

/**
 * Update the attributes display.
 * @param widget DosAttrView
 */
static void
rp_dos_attr_view_update_attrs_display(RpDosAttrView *widget)
{
	widget->inhibit_checkbox_no_toggle = TRUE;

	gboolean val;
#define UPDATE_CHECKBOX(attr, obj) \
	val = !!(widget->attrs & (attr)); \
	gtk_check_button_set_active(GTK_CHECK_BUTTON(widget->obj), val); \
	g_object_set_qdata(G_OBJECT(widget->obj), DosAttrView_value_quark, GUINT_TO_POINTER((guint)val))

	UPDATE_CHECKBOX(FILE_ATTRIBUTE_READONLY, chkReadOnly);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_HIDDEN, chkHidden);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_ARCHIVE, chkArchive);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_SYSTEM, chkSystem);

	UPDATE_CHECKBOX(FILE_ATTRIBUTE_COMPRESSED, chkCompressed);
	UPDATE_CHECKBOX(FILE_ATTRIBUTE_ENCRYPTED, chkEncrypted);

	widget->inhibit_checkbox_no_toggle = FALSE;
}

/** Property accessors / mutators **/

/**
 * Set the current MS-DOS attributes.
 * @param widget DosAttrView
 * @param attrs MS-DOS attributes
 */
void
rp_dos_attr_view_set_attrs(RpDosAttrView *widget, unsigned int attrs)
{
	g_return_if_fail(RP_IS_DOS_ATTR_VIEW(widget));
	if (widget->attrs != attrs) {
		widget->attrs = attrs;
		rp_dos_attr_view_update_attrs_display(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_ATTRS]);
	}
}

/**
 * Get the current MS-DOS attributes.
 * @param widget DosAttrView
 * @return MS-DOS attributes
 */
unsigned int
rp_dos_attr_view_get_attrs(RpDosAttrView *widget)
{
	g_return_val_if_fail(RP_IS_DOS_ATTR_VIEW(widget), 0);
	return widget->attrs;
}

/**
 * Clear the current MS-DOS attributes.
 * @param widget DosAttrView
 */
void
rp_dos_attr_view_clear_attrs(RpDosAttrView *widget)
{
	g_return_if_fail(RP_IS_DOS_ATTR_VIEW(widget));
	if (widget->attrs != 0) {
		widget->attrs = 0;
		rp_dos_attr_view_update_attrs_display(widget);
		g_object_notify_by_pspec(G_OBJECT(widget), props[PROP_ATTRS]);
	}
}

/** Signal handlers **/

/**
 * Prevent bitfield checkboxes from being toggled.
 * @param checkbutton Bitfield checkbox
 * @param page DosAttrView
 */
static void
checkbox_no_toggle_signal_handler(GtkCheckButton	*checkbutton,
				  RpDosAttrView		*widget)
{
	if (widget->inhibit_checkbox_no_toggle) {
		// Inhibiting the no-toggle handler.
		return;
	}

	// Get the saved DosAttrView value.
	const gboolean value = (gboolean)GPOINTER_TO_UINT(
		g_object_get_qdata(G_OBJECT(checkbutton), DosAttrView_value_quark));
	if (gtk_check_button_get_active(checkbutton) != value) {
		// Toggle this box.
		gtk_check_button_set_active(checkbutton, value);
	}
}
