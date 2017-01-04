include $(srcdir)/cwm4/root_makefile_top.am

SUBDIRS = @CW_SUBDIRS@ src doc

EXTRA_DIST += LICENSE.GPL3 NEWS INSTALL.md README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmemleak.pc

# --------------- Maintainer's Section

distclean-local:
	rm -rf *~ $(srcdir)/autom4te.cache

DISTCLEANFILES = stamp-h.in

include $(srcdir)/cwm4/root_makefile_bottom.am
