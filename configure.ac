AC_PREREQ([2.64])
AC_INIT([opensettings], [0.1], [ataraxialinux@protonmail.ch])
AC_CONFIG_AUX_DIR(aux-dist)
AM_INIT_AUTOMAKE
AM_SANITY_CHECK
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_MAKE_SET
AC_GNU_SOURCE
AC_STDC_HEADERS
AC_PROG_CC([gcc])
AC_LANG(C)

CFLAGS="-fPIC"

PKG_CHECK_MODULES(GLIB, glib-2.0)
PKG_CHECK_MODULES(GIO, gio-2.0)
PKG_CHECK_MODULES(GMODULE, gmodule-2.0)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)
PKG_CHECK_MODULES(DBUSGLIB, dbus-glib-1)
PKG_CHECK_MODULES(POLKIT, polkit-gobject-1 dbus-1)

AC_CONFIG_FILES([src/datetime/org.opensettings.datetimemechanism.policy src/datetime/org.opensettings.DateTimeMechanism.service src/datetime/org.opensettings.DateTimeMechanism.desktop])

AC_CONFIG_FILES([Makefile src/Makefile src/datetime/Makefile])

AC_OUTPUT
