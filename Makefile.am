AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = @ACLOCAL_CWFLAGS@

SUBDIRS = src doc

EXTRA_DIST = LICENSE.GPL3 NEWS INSTALL README README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmemleak.pc

# --------------- Maintainer's Section

distclean-local:
	rm -rf *~ $(srcdir)/autom4te.cache

DISTCLEANFILES = stamp-h.in
MAINTAINERCLEANFILES = \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/configure \
	$(srcdir)/Makefile.in \
	$(srcdir)/install-sh \
	$(srcdir)/config.h.in \
	$(srcdir)/compile \
	$(srcdir)/depcomp \
	$(srcdir)/missing \
	$(srcdir)/config.guess \
	$(srcdir)/config.sub \ \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/ltmain.sh \
	$(srcdir)/stamp-h.in \
	$(srcdir)/ChangeLog
