AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = main.css mainpage.dox images/silvrhed.png

if REAL_MAINTAINER
HTMLDIR=@OUTPUT_DIRECTORY@/libmemleak
else
HTMLDIR=libmemleak

distclean-local:
	rm -rf $(HTMLDIR)
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

maintainer-clean-local:
	rm -rf html latex

all-local: doxygen.config html.header html.footer
	@REAL_MAINTAINER_TRUE@rm -rf $(HTMLDIR)
	doxygen doxygen.config
	mkdir -p $(HTMLDIR)/images
	if test -n "`ls $(srcdir)/images/*.png 2>/dev/null`"; then \
	  $(INSTALL) -m 644 $(srcdir)/images/*.png $(HTMLDIR)/images; \
	fi
	mv $(HTMLDIR)/doxygen.svg $(HTMLDIR)/images
	test -f $(srcdir)/main.css && $(INSTALL) -m 644 $(srcdir)/main.css $(HTMLDIR)

