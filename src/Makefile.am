## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = rb_tree include .

EXTRA_DIST = ${BUILT_SOURCES}

lib_LTLIBRARIES = libmemleak.la
VERSIONINFO = @VERSIONINFO@
AM_CPPFLAGS = -I$(top_builddir)/include -I$(srcdir)/include
BUILT_SOURCES =
DEFS = @DEFS@
CFLAGS = -g
CXXFLAGS =

bin_PROGRAMS = memleak_control
noinst_PROGRAMS = hello

memleak_control_SOURCES = memleak_control.c
memleak_control_LDADD = @LIBREADLINE@
memleak_CXXFLAGS = @CFLAGS@

hello_SOURCES = hello.cc
hello_CXXFLAGS = -pthread @CXXFLAGS@

libmemleak_la_SOURCES = \
	memleak.c \
	addr2line.c \
	sort.c

libmemleak_la_CFLAGS = -std=c99 @CFLAGS@
libmemleak_la_LIBADD = @LIBBFD@
libmemleak_la_LDFLAGS = -version-info $(VERSIONINFO) -no-undefined -Wl,--whole-archive,rb_tree/librbtree.a,--no-whole-archive

# --------------- Maintainer's Section

#dist-hook:

all-local:
	@if test -e $(top_srcdir)/../.gdbinit-src -a ! -e .gdbinit; then \
	  echo "ln -s $(top_srcdir)/../.gdbinit-src .gdbinit"; \
	  ln -s $(top_srcdir)/../.gdbinit-src .gdbinit; \
	fi

distclean-local:
	rm -rf $(srcdir)/autom4te.cache
	@MAINTAINER_MODE_TRUE@rm -f .all_sources *.s *.i *.ii

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in tags troep* *.cgd

if MAINTAINER_MODE
# Usage: You have to set the CTAGS_ROOT_SRCDIRS environment variable yourself,
# for example: export CTAGS_ROOT_SRCDIRS="/usr/src/gtk/glib-current /usr/src/gtk/gtk+-current"
# Then run 'vim -u /some/path/.vimrc' where the .vimrc is the normal .vimrc, but with the
# additional lines: set tags=/full/path/to/top_builddir/tags
#                   nmap <C-N> :tnext<CR>
ctags:
	rm -f .all_sources
	touch .all_sources
	for d in $(CTAGS_ROOT_SRCDIRS) $(top_srcdir); do \
	  find $$d -follow -type f -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.C' -o -name '*.inl' >> .all_sources; \
	done
	ctags `cat .all_sources`
endif
