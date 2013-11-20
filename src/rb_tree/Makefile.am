## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = .

EXTRA_DIST = ${BUILT_SOURCES}

lib_LIBRARIES = librbtree.a
BUILT_SOURCES =
DEFS = @DEFS@
CFLAGS = -fPIC @CFLAGS@

librbtree_a_SOURCES = \
	red_black_tree.c \
	stack.c \
	misc.c

librbtree_a_CFLAGS =

distclean-local:
	rm -rf $(srcdir)/autom4te.cache
	@MAINTAINER_MODE_TRUE@rm -f .all_sources *.s *.i *.ii

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in troep* *.cgd

