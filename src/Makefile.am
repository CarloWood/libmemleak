## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = rb_tree include .

EXTRA_DIST = pch-source.h ${BUILT_SOURCES}

lib_LTLIBRARIES = libmemleak.la
VERSIONINFO = @VERSIONINFO@
INCLUDES = -I$(top_builddir)/include -I$(srcdir)/include
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

if USE_PCH
@Makefilein@PREVCXXDEPMODE := $(CXXDEPMODE)
@Makefilein@CXXDEPMODE = $(PREVCXXDEPMODE) havepch=yes
@Makefilein@ifeq (./$(DEPDIR)/pch.po, $(wildcard ./$(DEPDIR)/pch.po))
@Makefilein@@am__include@ ./$(DEPDIR)/pch.po
@Makefilein@endif
PIC_FLAGS := $(shell egrep '^pic_flag=.*-f(PIC|pic)' $(top_builddir)/libtool | head -n 1 | sed -r -e 's/pic_flag=//' -e 's/^ *"//' -e 's/" *$$//')
libmemleak_la_CFLAGS += @PCHFLAGS@

APPLICATIONSUM := $(shell echo "$(CFLAGS)" | md5sum | sed -e 's/ .*//')
APPLICATIONSUM_PIC := $(shell echo "$(CFLAGS) $(PICFLAGS)" | md5sum | sed -e 's/ .*//')

BUILT_SOURCES += pch.h.gch/$(APPLICATIONSUM)
BUILT_SOURCES += pch.h.gch/$(APPLICATIONSUM_PIC)

pch.h.gch/$(APPLICATIONSUM): $(srcdir)/pch-source.h Makefile
	@rm -f $@
	@test -d pch.h.gch || mkdir pch.h.gch
	@test -d $(DEPDIR) || mkdir $(DEPDIR)
	@mkdir -p empty.pch.h
	@touch empty.pch.h/pch.h
	@if $(CC) -Iempty.pch.h $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(libmemleak_la_CFLAGS) \
	  -UUSE_PCH -M -MT pch.h.gch -MF $(DEPDIR)/pch.TTpo $(srcdir)/pch-source.h; then \
	  cat $(DEPDIR)/pch.TTpo | sed -e 's%^pch\.h\.gch:%pch.h.gch/$(APPLICATIONSUM):%' -e 's% empty\.pch\.h/pch\.h%%' > $(DEPDIR)/pch.Tpo; \
	  cmp -s $(DEPDIR)/pch.Tpo $(DEPDIR)/pch.po || mv $(DEPDIR)/pch.Tpo $(DEPDIR)/pch.po; \
	fi
	$(CC) -Iempty.pch.h $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(libmemleak_la_CFLAGS) \
	  -c -o $@ $(srcdir)/pch-source.h
	@rm empty.pch.h/pch.h
	@rmdir empty.pch.h

pch.h.gch/$(APPLICATIONSUM_PIC): $(srcdir)/pch-source.h Makefile
	@rm -f $@
	@test -d pch.h.gch || mkdir pch.h.gch
	@test -d $(DEPDIR) || mkdir $(DEPDIR)
	@mkdir -p empty.pch.h
	@touch empty.pch.h/pch.h
	$(CC) -Iempty.pch.h $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(libmemleak_la_CFLAGS) $(PIC_FLAGS) \
	  -c -o $@ $(srcdir)/pch-source.h
	@rm empty.pch.h/pch.h
	@rmdir empty.pch.h
endif

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

clean-local:
if USE_PCH
	rm -f pch.h.gch/* $(DEPDIR)/pch.*po
endif

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
