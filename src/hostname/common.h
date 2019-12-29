/*

  Copyright 2016-2019 Ataraxia Linux

*/

#include <glib.h>
#include <gio/gio.h>
#include <dbus/dbus-protocol.h>
#include <polkit/polkit.h>

char *read_key_file (const char *filename,
                               const char *key);
gboolean
write_key_file (const char  *filename,
                                const char  *key,
                                const char  *value,
                                GError     **error);
gboolean
check_polkit_finish (GAsyncResult *res,
                     GError **error);
void
check_polkit_async (const gchar *unique_name,
                    const gchar *action_id,
                    const gboolean user_interaction,
                    GAsyncReadyCallback callback,
                    gpointer user_data);
static void
check_polkit_authority_cb (GObject *source_object,
			GAsyncResult *res,
			gpointer _data);
static void
check_polkit_authorization_cb (GObject *source_object,
				GAsyncResult *res,
				gpointer _data);
void component_started();
