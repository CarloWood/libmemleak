AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = CHANGES LICENSE rb_tree.diff README simple_test.sh unfreed.txt

noinst_LTLIBRARIES = librbtree.la

librbtree_la_SOURCES = \
	red_black_tree.c \
	red_black_tree.h \
	stack.c \
	stack.h \
	misc.c \
	misc.h

distclean-local:
	rm -rf $(srcdir)/autom4te.cache
	@MAINTAINER_MODE_TRUE@rm -f .all_sources *.s *.i *.ii

DISTCLEANFILES = librbtree.a
MAINTAINERCLEANFILES = $(srcdir)/Makefile.in troep* *.cgd
