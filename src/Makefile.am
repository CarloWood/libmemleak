AUTOMAKE_OPTIONS = foreign
SUBDIRS = rb_tree include

AM_CPPFLAGS = -I$(top_builddir)/include -I$(srcdir)/include

VERSIONINFO = @VERSIONINFO@

lib_LTLIBRARIES = libmemleak.la

libmemleak_la_SOURCES = \
	memleak.c \
	addr2line.c \
	sort.c \
	sort.hc

libmemleak_la_LIBADD = rb_tree/librbtree.la @LIBBFD@
libmemleak_la_LDFLAGS = -version-info $(VERSIONINFO) -no-undefined

bin_PROGRAMS = memleak_control

memleak_control_SOURCES = memleak_control.c
memleak_control_LDADD = @LIBREADLINE@

noinst_PROGRAMS = hello

hello_SOURCES = hello.cc
hello_CXXFLAGS = -pthread @CXXFLAGS@

# --------------- Maintainer's Section

#dist-hook:

all-local:
	@if test -e $(top_srcdir)/../.gdbinit-src -a ! -e .gdbinit; then \
	  echo "ln -s $(top_srcdir)/../.gdbinit-src .gdbinit"; \
	  ln -s $(top_srcdir)/../.gdbinit-src .gdbinit; \
	fi

distclean-local:
	rm -rf $(srcdir)/autom4te.cache
	@test -L .gdbinit && rm .gdbinit
	@MAINTAINER_MODE_TRUE@rm -f .all_sources *.s *.i *.ii

DISTCLEANFILES = memleak_backtraces memleak_sock
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
