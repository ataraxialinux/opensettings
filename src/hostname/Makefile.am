dbus_servicesdir = $(datadir)/dbus-1/system-services
dbus_confdir = $(datadir)/dbus-1/system.d
polkitdir = $(datadir)/polkit-1/actions
autorundir = $(sysconfdir)/xdg/autostart

dbus_services_in_files = org.freedesktop.hostname1.service.in
polkit_in_files = org.freedesktop.hostname1.policy.in

hostname-glue.h: $(srcdir)/org.freedesktop.hostname1.xml
	$(AM_V_GEN) gdbus-codegen \
			--interface-prefix org.freedesktop. \
			--c-namespace OpenSettings --generate-c-code hostname-glue \
			org.freedesktop.hostname1.xml

bin_PROGRAMS = opensettings-hostname

opensettings_hostname_CFLAGS = \
        @CFLAGS@ \
        @GLIB_CFLAGS@ \
        @GIO_CFLAGS@ \
        @DBUSGLIB_CFLAGS@ \
        @POLKIT_CFLAGS@

opensettings_hostname_LDADD = \
        @GLIB_LIBS@ \
        @GIO_LIBS@ \
        @DBUSGLIB_LIBS@ \
        @POLKIT_LIBS@

opensettings_hostname_SOURCES = \
	main.c \
	hostname-glue.c \
	common.c

BUILT_SOURCES = hostname-glue.h

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(autorundir)
	$(mkinstalldirs) $(DESTDIR)$(dbus_servicesdir)
	$(mkinstalldirs) $(DESTDIR)$(dbus_confdir)
	$(mkinstalldirs) $(DESTDIR)$(polkitdir)
	install -m644 org.freedesktop.hostname1.desktop $(DESTDIR)$(autorundir)
	install -m644 org.freedesktop.hostname1.service $(DESTDIR)$(dbus_servicesdir)
	install -m644 org.freedesktop.hostname1.policy $(DESTDIR)$(polkitdir)

CLEANFILES = 		\
	org.freedesktop.hostname1.service	\
	org.freedesktop.hostname1.policy	\
	org.freedesktop.hostname1.desktop \
	$(BUILT_SOURCES)
