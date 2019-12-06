dbus_servicesdir = $(datadir)/dbus-1/system-services
dbus_confdir = $(datadir)/dbus-1/system.d
polkitdir = $(datadir)/polkit-1/actions
autorundir = $(sysconfdir)/xdg/autostart

dbus_services_in_files = org.opensettings.DateTimeMechanism.service.in
polkit_in_files = org.opensettings.datetimemechanism.policy.in

datetime-glue.h: $(srcdir)/datetime.xml
	$(AM_V_GEN) dbus-binding-tool \
			--prefix=gsd_datetime_mechanism --mode=glib-server	\
			--output=datetime-glue.h						\
			$(srcdir)/datetime.xml


bin_PROGRAMS = opensettings-datetime

opensettings_datetime_CFLAGS = \
        @CFLAGS@ \
        @GLIB_CFLAGS@ \
        @DBUSGLIB_CFLAGS@ \
        @POLKIT_CFLAGS@

opensettings_datetime_LDADD = \
        @GLIB_LIBS@ \
        @DBUSGLIB_LIBS@ \
        @POLKIT_LIBS@

opensettings_datetime_SOURCES = \
	datetime.c			\
	datetime.h			\
	datetime-ataraxia.c	\
	datetime-ataraxia.h	\
	datetime-devuan.c		\
	datetime-devuan.h		\
	datetime-main.c		\
	system-timezone.c		\
	system-timezone.h

BUILT_SOURCES = datetime-glue.h

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(autorundir)
	$(mkinstalldirs) $(DESTDIR)$(dbus_servicesdir)
	$(mkinstalldirs) $(DESTDIR)$(dbus_confdir)
	$(mkinstalldirs) $(DESTDIR)$(polkitdir)
	install -m644 org.opensettings.DateTimeMechanism.desktop $(DESTDIR)$(autorundir)
	install -m644 org.opensettings.DateTimeMechanism.service $(DESTDIR)$(dbus_servicesdir)
	install -m644 org.opensettings.DateTimeMechanism.conf $(DESTDIR)$(dbus_confdir)
	install -m644 org.opensettings.datetimemechanism.policy $(DESTDIR)$(polkitdir)

CLEANFILES = 		\
	org.opensettings.DateTimeMechanism.service	\
	org.opensettings.datetimemechanism.policy	\
	org.opensettings.DateTimeMechanism.desktop \
	$(BUILT_SOURCES)
