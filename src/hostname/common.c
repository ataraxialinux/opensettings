/*

  Copyright 2016-2019 Ataraxia Linux

*/

#include <glib.h>
#include <gio/gio.h>
#include <dbus/dbus-protocol.h>
#include <polkit/polkit.h>

#include "common.h"

#define READ_ERROR 1
#define READ_ERROR_GENERAL 1
#define PIDFILE "/run/hostname1.pid"

struct check_polkit_data {
	const gchar *unique_name;
	const gchar *action_id;
	gboolean user_interaction;
	GAsyncReadyCallback callback;
	gpointer user_data;

	PolkitAuthority *authority;
	PolkitSubject *subject;
};

char *read_key_file (const char *filename,
                               const char *key)
{
        GIOChannel *channel;
        char       *key_eq;
        char       *line;
        char       *retval;

        if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
                return NULL;

        channel = g_io_channel_new_file (filename, "r", NULL);
        if (!channel)
                return NULL;

        key_eq = g_strdup_printf ("%s=", key);
        retval = NULL;

        while (g_io_channel_read_line (channel, &line, NULL,
                                       NULL, NULL) == G_IO_STATUS_NORMAL) {
                if (g_str_has_prefix (line, key_eq)) {
                        char *value;
                        int   len;

                        value = line + strlen (key_eq);
                        g_strstrip (value);

                        len = strlen (value);

                        if (value[0] == '\"') {
                                if (value[len - 1] == '\"') {
                                        if (retval)
                                                g_free (retval);

                                        retval = g_strndup (value + 1,
                                                            len - 2);
                                }
                        } else {
                                if (retval)
                                        g_free (retval);

                                retval = g_strdup (line + strlen (key_eq));
                        }

                        g_strstrip (retval);
                }

                g_free (line);
        }

        g_free (key_eq);
        g_io_channel_unref (channel);

        return retval;
}

gboolean write_key_file (const char  *filename,
                                const char  *key,
                                const char  *value,
                                GError     **error)
{
        GError    *our_error;
        char      *content;
        gsize      len;
        char      *key_eq;
        char     **lines;
        gboolean   replaced;
        gboolean   retval;
        int        n;
        
        if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
                return TRUE;

        our_error = NULL;

        if (!g_file_get_contents (filename, &content, &len, &our_error)) {
                g_set_error (error, READ_ERROR,
                             READ_ERROR_GENERAL,
                             "%s cannot be read: %s",
                             filename, our_error->message);
                g_error_free (our_error);
                return FALSE;
        }

        lines = g_strsplit (content, "\n", 0);
        g_free (content);

        key_eq = g_strdup_printf ("%s=", key);
        replaced = FALSE;

        for (n = 0; lines[n] != NULL; n++) {
                if (g_str_has_prefix (lines[n], key_eq)) {
                        char     *old_value;
                        gboolean  use_quotes;

                        old_value = lines[n] + strlen (key_eq);
                        g_strstrip (old_value);
                        use_quotes = old_value[0] == '\"';

                        g_free (lines[n]);

                        if (use_quotes)
                                lines[n] = g_strdup_printf ("%s\"%s\"",
                                                            key_eq, value);
                        else
                                lines[n] = g_strdup_printf ("%s%s",
                                                            key_eq, value);

                        replaced = TRUE;
                }
        }

        g_free (key_eq);

        if (!replaced) {
                g_strfreev (lines);
                return TRUE;
        }

        content = g_strjoinv ("\n", lines);
        g_strfreev (lines);

        retval = g_file_set_contents (filename, content, -1, &our_error);
        g_free (content);

        if (!retval) {
                g_set_error (error, READ_ERROR,
                             READ_ERROR_GENERAL,
                             "%s cannot be overwritten: %s",
                             filename, our_error->message);
                g_error_free (our_error);
        }

        return retval;
}

void component_started() {
	gchar *pidstring = NULL;
	GError *err = NULL;
	GFile *pidfile = NULL;

	pidfile = g_file_new_for_path(PIDFILE);
	pidstring = g_strdup_printf ("%lu", (gulong)getpid ());
	if (!g_file_replace_contents (pidfile, pidstring, strlen(pidstring), NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL, &err)) {
		g_critical ("Failed to write " PIDFILE ": %s", err->message);
		exit(1);
	}
}

void
check_polkit_data_free (struct check_polkit_data *data)
{
	if (data == NULL)
		return;

	if (data->subject != NULL)
		g_object_unref (data->subject);
	if (data->authority != NULL)
		g_object_unref (data->authority);
    
	g_free (data);
}

gboolean
check_polkit_finish (GAsyncResult *res,
                     GError **error)
{
	GSimpleAsyncResult *simple;

	simple = G_SIMPLE_ASYNC_RESULT (res);
	if (g_simple_async_result_propagate_error (simple, error))
		return FALSE;

	return g_simple_async_result_get_op_res_gboolean (simple);
}

void
check_polkit_async (const gchar *unique_name,
                    const gchar *action_id,
                    const gboolean user_interaction,
                    GAsyncReadyCallback callback,
                    gpointer user_data)
{
	struct check_polkit_data *data;

	data = g_new0 (struct check_polkit_data, 1);
	data->unique_name = unique_name;
	data->action_id = action_id;
	data->user_interaction = user_interaction;
	data->callback = callback;
	data->user_data = user_data;

	polkit_authority_get_async (NULL, check_polkit_authority_cb, data);
}

static void
check_polkit_authorization_cb (GObject *source_object,
				GAsyncResult *res,
				gpointer _data)
{
	struct check_polkit_data *data;
	PolkitAuthorizationResult *result;
	GSimpleAsyncResult *simple;
	GError *err = NULL;

	data = (struct check_polkit_data *) _data;
	if ((result = polkit_authority_check_authorization_finish (data->authority, res, &err)) == NULL) {
		g_simple_async_report_take_gerror_in_idle (NULL, data->callback, data->user_data, err);
		goto out;
	}
 
	if (!polkit_authorization_result_get_is_authorized (result)) {
		g_simple_async_report_error_in_idle (NULL, data->callback, data->user_data, POLKIT_ERROR, POLKIT_ERROR_NOT_AUTHORIZED, "Authorizing for '%s': not authorized", data->action_id);
		goto out;
	}
	simple = g_simple_async_result_new (NULL, data->callback, data->user_data, check_polkit_async);
	g_simple_async_result_set_op_res_gboolean (simple, TRUE);
	g_simple_async_result_complete_in_idle (simple);
	g_object_unref (simple);

	out:
		check_polkit_data_free (data);
		if (result != NULL)
			g_object_unref (result);
}

static void
check_polkit_authority_cb (GObject *source_object,
			GAsyncResult *res,
			gpointer _data)
{
	struct check_polkit_data *data;
	GError *err = NULL;

	data = (struct check_polkit_data *) _data;
	if ((data->authority = polkit_authority_get_finish (res, &err)) == NULL) {
		g_simple_async_report_take_gerror_in_idle (NULL, data->callback, data->user_data, err);
		check_polkit_data_free (data);
		return;
	}
	if (data->unique_name == NULL || data->action_id == NULL || 
		(data->subject = polkit_system_bus_name_new (data->unique_name)) == NULL) {
		g_simple_async_report_error_in_idle (NULL, data->callback, data->user_data, POLKIT_ERROR, POLKIT_ERROR_FAILED, "Authorizing for '%s': failed sanity check", data->action_id);
		check_polkit_data_free (data);
		return;
	}
	polkit_authority_check_authorization (data->authority, data->subject, data->action_id, NULL, (PolkitCheckAuthorizationFlags) data->user_interaction, NULL, check_polkit_authorization_cb, data);
}
