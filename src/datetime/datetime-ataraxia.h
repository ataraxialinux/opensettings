/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
 * Copyright (C) 2011 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2011 Vincent Untz <vuntz@gnome.org>
 * Copyright (C) 2019 protonesso <nagakamira@gmail.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <glib.h>
#include <dbus/dbus-glib.h>

gboolean _get_using_ntp_ataraxia  (DBusGMethodInvocation   *context);
gboolean _set_using_ntp_ataraxia  (DBusGMethodInvocation   *context,
                               gboolean                 using_ntp);
gboolean _update_etc_rcd_ntp_ataraxia
                                (DBusGMethodInvocation *context,
                                 const char            *key,
                                 const char            *value);
