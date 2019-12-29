/*
  Copyright 2019 Ataraxia Linux
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include <dbus/dbus-protocol.h>
#include <polkit/polkit.h>

#include "common.h"
#include "hostname-glue.h"

#define QUOTE(macro) #macro
#define STR(macro) QUOTE(macro)
#define ETC_RC_CONF "/etc/rc.conf"
#define MACHINE_INFO "/etc/machine-info"

guint bus_id = 0;
gboolean read_only = FALSE;

static OpenSettingsHostname1 *hostname1 = NULL;

static gchar *hostname = NULL;
G_LOCK_DEFINE_STATIC (hostname);
static gchar *static_hostname = NULL;
static GFile *static_hostname_file = NULL;
G_LOCK_DEFINE_STATIC (static_hostname);
static gchar *pretty_hostname = NULL;
static gchar *icon_name = NULL;
static GFile *machine_info_file = NULL;
G_LOCK_DEFINE_STATIC (machine_info);

struct invoked_name {
	GDBusMethodInvocation *invocation;
	gchar *name; /* newly allocated */
};

static gboolean
hostname_is_valid (const gchar *name) {
	if (name == NULL)
		return 0;

	return g_regex_match_simple ("^[a-zA-Z0-9_.-]{1," STR(HOST_NAME_MAX) "}$", name, G_REGEX_MULTILINE, 0);
}

static gchar *
guess_icon_name() {
	gchar *filebuf = NULL;
	gchar *ret = NULL;

#if defined(__i386__) || defined(__x86_64__)
	if (g_file_get_contents ("/sys/class/dmi/id/chassis_type", &filebuf, NULL, NULL)) {
		switch (g_ascii_strtoull (filebuf, NULL, 10)) {
			case 0x3: /* Desktop */
			case 0x4: /* Low Profile Desktop */
			case 0x6: /* Mini Tower */
			case 0x7: /* Tower */
				ret = g_strdup ("computer-desktop");
				goto out;

			case 0x8: /* Portable */
			case 0x9: /* Laptop */
			case 0xA: /* Notebook */
			case 0xE: /* Sub Notebook */
				ret = g_strdup ("computer-laptop");
				goto out;

			case 0xB: /* Hand Held */
				ret = g_strdup ("computer-handset");
				goto out;

			case 0x11: /* Main Server Chassis */
				case 0x1C: /* Blade */
				case 0x1D: /* Blade Enclosure */
				ret = g_strdup ("computer-server");
				goto out;

			case 0x1E: /* Tablet */
				return "tablet";

			case 0x1F: /* Convertible */
			case 0x20: /* Detachable */
				ret = g_strdup ("computer-convertible");
				goto out;
		}
	}
#endif
	ret = g_strdup ("computer");
	out:
		g_free (filebuf);
		return ret;
}

static void
on_handle_set_hostname_authorized_cb (GObject *source_object,
					GAsyncResult *res,
					gpointer user_data)
{
	GError *err = NULL;
	struct invoked_name *data;
	
	data = (struct invoked_name *) user_data;
	if (!check_polkit_finish (res, &err)) {
		g_dbus_method_invocation_return_gerror (data->invocation, err);
		goto out;
	}

	G_LOCK (hostname);
	if (!hostname_is_valid (data->name)) {
		if (data->name != NULL)
			g_free (data->name);

		if (hostname_is_valid (static_hostname))
			data->name = g_strdup (static_hostname);
		else
			data->name = g_strdup ("localhost");
	}
	if (sethostname (data->name, strlen(data->name))) {
		int errsv = errno;
		g_dbus_method_invocation_return_dbus_error (data->invocation,
													DBUS_ERROR_FAILED,
													strerror (errsv));
		G_UNLOCK (hostname);
		goto out;
	}
	g_free (hostname);
	hostname = data->name; /* data->name is g_strdup-ed already */;
	open_settings_hostname1_complete_set_hostname (hostname1, data->invocation);
	open_settings_hostname1_set_hostname (hostname1, hostname);
	G_UNLOCK (hostname);

	out:
		g_free (data);
		if (err != NULL)
			g_error_free (err);
}

static gboolean
on_handle_set_hostname (OpenSettingsHostname1 *hostname1,
                        GDBusMethodInvocation *invocation,
                        const gchar *name,
                        const gboolean user_interaction,
                        gpointer user_data)
{
	if (read_only)
		g_dbus_method_invocation_return_dbus_error (invocation,
							DBUS_ERROR_NOT_SUPPORTED,
							"opensetiings-hostname is in read-only mode");
	else {
		struct invoked_name *data;
		data = g_new0 (struct invoked_name, 1);
		data->invocation = invocation;
		data->name = g_strdup (name);
		check_polkit_async (g_dbus_method_invocation_get_sender (invocation), "org.freedesktop.hostname1.set-hostname", user_interaction, on_handle_set_hostname_authorized_cb, data);
	}

	return TRUE;
}

static void
on_handle_set_static_hostname_authorized_cb (GObject *source_object,
                                             GAsyncResult *res,
                                             gpointer user_data)
{
	GError *err = NULL;
	struct invoked_name *data;
    
	data = (struct invoked_name *) user_data;
	if (!check_polkit_finish (res, &err)) {
		g_dbus_method_invocation_return_gerror (data->invocation, err);
		goto out;
	}

	G_LOCK (static_hostname);
	if (!hostname_is_valid (data->name)) {
		if (data->name != NULL)
		g_free (data->name);

		data->name = g_strdup ("localhost");
	}

	if (!read_key_file (ETC_RC_CONF, "hostname")) {
		g_dbus_method_invocation_return_gerror (data->invocation, err);
		G_UNLOCK (static_hostname);
		goto out;
	}

	g_free (static_hostname);
	static_hostname = data->name;
	open_settings_hostname1_complete_set_static_hostname (hostname1, data->invocation);
	open_settings_hostname1_set_static_hostname (hostname1, static_hostname);
	G_UNLOCK (static_hostname);

	out:
		g_free (data);
		if (err != NULL)
			g_error_free (err);
}

static gboolean
on_handle_set_static_hostname (OpenSettingsHostname1 *hostname1,
                               GDBusMethodInvocation *invocation,
                               const gchar *name,
                               const gboolean user_interaction,
                               gpointer user_data)
{
	if (read_only)
		g_dbus_method_invocation_return_dbus_error (invocation,
                                                    DBUS_ERROR_NOT_SUPPORTED,
                                                    "opensetiings-hostname is in read-only mode");
	else {
		struct invoked_name *data;
		data = g_new0 (struct invoked_name, 1);
		data->invocation = invocation;
		data->name = g_strdup (name);
		check_polkit_async (g_dbus_method_invocation_get_sender (invocation), "org.freedesktop.hostname1.set-static-hostname", user_interaction, on_handle_set_static_hostname_authorized_cb, data);
	}

	return TRUE;
}

static void
on_handle_set_pretty_hostname_authorized_cb (GObject *source_object,
                                             GAsyncResult *res,
                                             gpointer user_data)
{
	GError *err = NULL;
	struct invoked_name *data;
    
	data = (struct invoked_name *) user_data;
	if (!check_polkit_finish (res, &err)) {
		g_dbus_method_invocation_return_gerror (data->invocation, err);
		goto out;
	}

	G_LOCK (machine_info);
	/* Don't allow a null pretty hostname */
	if (data->name == NULL)
	data->name = g_strdup ("");

	if (!read_key_file (MACHINE_INFO, "PRETTY_HOSTNAME")) {
		g_dbus_method_invocation_return_gerror (data->invocation, err);
		G_UNLOCK (machine_info);
		goto out;
	}

	g_free (pretty_hostname);
	pretty_hostname = data->name; /* data->name is g_strdup-ed already */
	open_settings_hostname1_complete_set_pretty_hostname (hostname1, data->invocation);
	open_settings_hostname1_set_pretty_hostname (hostname1, pretty_hostname);
	G_UNLOCK (machine_info);

	out:
		g_free (data);
		if (err != NULL)
			g_error_free (err);
}

static gboolean
on_handle_set_pretty_hostname (OpenSettingsHostname1 *hostname1,
                               GDBusMethodInvocation *invocation,
                               const gchar *name,
                               const gboolean user_interaction,
                               gpointer user_data)
{
	if (read_only)
		g_dbus_method_invocation_return_dbus_error (invocation,
                                                    DBUS_ERROR_NOT_SUPPORTED,
                                                    "opensetiings-hostname is in read-only mode");
	else {
		struct invoked_name *data;
		data = g_new0 (struct invoked_name, 1);
		data->invocation = invocation;
		data->name = g_strdup (name);
		check_polkit_async (g_dbus_method_invocation_get_sender (invocation), "org.freedesktop.hostname1.set-machine-info", user_interaction, on_handle_set_pretty_hostname_authorized_cb, data);
	}

	return TRUE; /* Always return TRUE to indicate signal has been handled */
}

static void
on_handle_set_icon_name_authorized_cb (GObject *source_object,
                                       GAsyncResult *res,
                                       gpointer user_data)
{
	GError *err = NULL;
	struct invoked_name *data;
    
	data = (struct invoked_name *) user_data;
	if (!check_polkit_finish (res, &err)) {
		g_dbus_method_invocation_return_gerror (data->invocation, err);
		goto out;
	}

	G_LOCK (machine_info);
	/* Don't allow a null pretty hostname */
	if (data->name == NULL)
		data->name = g_strdup ("");

	if (!read_key_file (MACHINE_INFO, "ICON_NAME")) {
		g_dbus_method_invocation_return_gerror (data->invocation, err);
		G_UNLOCK (machine_info);
		goto out;
	}

	g_free (icon_name);
	icon_name = data->name; /* data->name is g_strdup-ed already */
	open_settings_hostname1_complete_set_icon_name (hostname1, data->invocation);
	open_settings_hostname1_set_icon_name (hostname1, icon_name);
	G_UNLOCK (machine_info);

	out:
		g_free (data);
		if (err != NULL)
			g_error_free (err);
}

static gboolean
on_handle_set_icon_name (OpenSettingsHostname1 *hostname1,
                         GDBusMethodInvocation *invocation,
                         const gchar *name,
                         const gboolean user_interaction,
                         gpointer user_data)
{
	if (read_only)
		g_dbus_method_invocation_return_dbus_error (invocation,
                                                    DBUS_ERROR_NOT_SUPPORTED,
                                                    "opensetiings-hostname is in read-only mode");
	else {
		struct invoked_name *data;
		data = g_new0 (struct invoked_name, 1);
		data->invocation = invocation;
		data->name = g_strdup (name);
		check_polkit_async (g_dbus_method_invocation_get_sender (invocation), "org.freedesktop.hostname1.set-machine-info", user_interaction, on_handle_set_icon_name_authorized_cb, data);
	}

	return TRUE; /* Always return TRUE to indicate signal has been handled */
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *bus_name,
                 gpointer         user_data)
{
	gchar *name;
	GError *err = NULL;

	g_debug ("Acquired a message bus connection");

	hostname1 = open_settings_hostname1_skeleton_new();

	open_settings_hostname1_set_hostname (hostname1, hostname);
	open_settings_hostname1_set_static_hostname (hostname1, static_hostname);
	open_settings_hostname1_set_pretty_hostname (hostname1, pretty_hostname);
	open_settings_hostname1_set_icon_name (hostname1, icon_name);

	g_signal_connect (hostname1, "handle-set-hostname", G_CALLBACK (on_handle_set_hostname), NULL);
	g_signal_connect (hostname1, "handle-set-static-hostname", G_CALLBACK (on_handle_set_static_hostname), NULL);
	g_signal_connect (hostname1, "handle-set-pretty-hostname", G_CALLBACK (on_handle_set_pretty_hostname), NULL);
	g_signal_connect (hostname1, "handle-set-icon-name", G_CALLBACK (on_handle_set_icon_name), NULL);

	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (hostname1),
					connection,
					"/org/freedesktop/hostname1",
					 &err)) {
	if (err != NULL) {
		g_critical ("Failed to export interface on /org/freedesktop/hostname1: %s", err->message);
		exit(1);
		}
	}
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *bus_name,
                  gpointer         user_data)
{
	g_debug ("Acquired the name %s", bus_name);
	component_started();
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *bus_name,
              gpointer         user_data)
{
	if (connection == NULL)
		g_critical ("Failed to acquire a dbus connection");
	else
		g_critical ("Failed to acquire dbus name %s", bus_name);
	exit(1);
}

void
destroy (void)
{
	g_bus_unown_name (bus_id);
	bus_id = 0;
	read_only = FALSE;
	g_free (hostname);
	g_free (static_hostname);
	g_free (pretty_hostname);
	g_free (icon_name);
}

void
init (gboolean _read_only)
{
	GError *err = NULL;

	hostname = g_malloc0 (HOST_NAME_MAX + 1);
	if (gethostname (hostname, HOST_NAME_MAX)) {
		perror (NULL);
		g_strlcpy (hostname, "localhost", HOST_NAME_MAX + 1);
	}

	static_hostname = read_key_file (ETC_RC_CONF, "hostname");
	if (err != NULL) {
		g_debug ("%s", err->message);
		g_error_free (err);
		err = NULL;
	}
	pretty_hostname = read_key_file (MACHINE_INFO, "PRETTY_HOSTNAME");
	if (pretty_hostname == NULL)
		pretty_hostname = g_strdup ("");
	if (err != NULL) {
		g_debug ("%s", err->message);
		g_error_free (err);
		err = NULL;
	}
	icon_name = read_key_file (MACHINE_INFO, "ICON_NAME");
	if (icon_name == NULL)
		icon_name = g_strdup ("");
	if (err != NULL) {
		g_debug ("%s", err->message);
		g_error_free (err);
		err = NULL;
	}

	if (icon_name == NULL || *icon_name == 0) {
		g_free (icon_name);
		icon_name = guess_icon_name ();
	}

	read_only = _read_only;

	bus_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
				"org.freedesktop.hostname1",
				G_BUS_NAME_OWNER_FLAGS_NONE,
				on_bus_acquired,
				on_name_acquired,
				on_name_lost,
				NULL,
				NULL);
}

gint main() {
	GError *error = NULL;
	GOptionContext *option_context;
	GMainLoop *loop = NULL;
	pid_t pid;

	g_type_init();

	init(read_only);
	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run(loop);

	g_main_loop_unref(loop);

	destroy();

	g_clear_error (&error);
	exit(0);
}
