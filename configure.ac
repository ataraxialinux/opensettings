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

AC_CONFIG_MACRO_DIR([m4])

AC_CHECK_HEADERS(unistd.h)

DBUS_GLIB_REQUIRED_VERSION=0.74
GLIB_REQUIRED_VERSION=2.29.14
GCONF_REQUIRED_VERSION=2.6.1
GIO_REQUIRED_VERSION=2.26.0

PKG_CHECK_MODULES(SETTINGS_DAEMON,
        glib-2.0 >= $GLIB_REQUIRED_VERSION
        gio-2.0 >= $GIO_REQUIRED_VERSION
        gconf-2.0 >= $GCONF_REQUIRED_VERSION
        gmodule-2.0
        gthread-2.0
        dbus-glib-1 >= $DBUS_GLIB_REQUIRED_VERSION
	  gsettings-desktop-schemas >= 0.1.2
)

AC_CONFIG_FILES([src/datetime/org.opensettings.datetimemechanism.policy src/datetime/org.opensettings.DateTimeMechanism.service])

AC_CONFIG_FILES([Makefile src/Makefile src/datetime/Makefile])

AC_OUTPUT
