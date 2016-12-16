## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = .

EXTRA_DIST = CHANGES LICENSE rb_tree.diff README simple_test.sh unfreed.txt ${BUILT_SOURCES}

lib_LIBRARIES = librbtree.a
BUILT_SOURCES =
DEFS = @DEFS@
ARFLAGS = @AR_FLAGS@
CFLAGS = -fPIC @CFLAGS@

librbtree_a_SOURCES = \
	red_black_tree.c \
	red_black_tree.h \
	stack.c \
	stack.h \
	misc.c \
	misc.h

librbtree_a_CFLAGS =

distclean-local:
	rm -rf $(srcdir)/autom4te.cache
	@MAINTAINER_MODE_TRUE@rm -f .all_sources *.s *.i *.ii

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in troep* *.cgd

