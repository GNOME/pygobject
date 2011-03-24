REF_VERSION = $(PYGOBJECT_MAJOR_VERSION).$(PYGOBJECT_MINOR_VERSION)
FULL_VERSION = $(REF_VERSION).$(PYGOBJECT_MICRO_VERSION)

BUILDDIR = $(top_builddir)/docs

CSS_FILES = style.css

HTML_FILES =					\
	html/class-gobjectgboxed.html		\
	html/class-gobjectginterface.html	\
	html/class-gobjectgpointer.html		\
	html/class-gobject.html			\
	html/class-glibmaincontext.html		\
	html/class-glibmainloop.html		\
	html/glib-constants.html		\
	html/glib-functions.html		\
	html/gobject-class-reference.html	\
	html/glib-class-reference.html		\
	html/class-gioapplaunchcontext.html	\
	html/class-gioappinfo.html		\
	html/class-gioasyncresult.html		\
	html/class-giobufferedinputstream.html	\
	html/class-giobufferedoutputstream.html	\
	html/class-giocancellable.html		\
	html/class-giodatainputstream.html	\
	html/class-giodataoutputstream.html	\
	html/class-giodrive.html		\
	html/class-gioemblem.html		\
	html/class-gioemblemedicon.html		\
	html/class-giofile.html			\
	html/class-giofileattributeinfo.html	\
	html/class-giofileenumerator.html	\
	html/class-giofileicon.html		\
	html/class-giofileinfo.html		\
	html/class-giofileinputstream.html	\
	html/class-giofilemonitor.html		\
	html/class-giofileoutputstream.html	\
	html/class-giofilterinputstream.html	\
	html/class-giofilteroutputstream.html	\
	html/class-gioicon.html			\
	html/class-gioinputstream.html		\
	html/class-gioloadableicon.html		\
	html/class-giomemoryinputstream.html	\
	html/class-giomemoryoutputstream.html	\
	html/class-giomount.html		\
	html/class-giomountoperation.html	\
	html/class-giooutputstream.html		\
	html/class-gioseekable.html		\
	html/class-giosimpleasyncresult.html	\
	html/class-giothemedicon.html		\
	html/class-giounixinputstream.html	\
	html/class-giounixoutputstream.html	\
	html/class-giovolume.html		\
	html/class-giovolumemonitor.html	\
	html/gio-constants.html			\
	html/gio-functions.html			\
	html/gio-class-reference.html		\
	html/giounix-class-reference.html	\
	html/gobject-constants.html		\
	html/gobject-functions.html		\
	html/index.html				\
	html/index.sgml				\
	html/pygobject.devhelp			\
	html/pygobject-introduction.html


XML_FILES = 						\
	reference/pygobject.xml				\
	reference/pygobject-introduction.xml 		\
	reference/pygobject-classes.xml 		\
	reference/pygobject-constants.xml 		\
	reference/pygobject-ref.xml			\
	reference/pygobject-functions.xml		\
	reference/pygobject-gboxed.xml			\
	reference/pygobject-ginterface.xml 		\
	reference/pygobject-gpointer.xml 		\
	reference/pyglib-classes.xml 			\
	reference/pyglib-constants.xml 			\
	reference/pyglib-functions.xml			\
	reference/pyglib-maincontext.xml 		\
	reference/pyglib-mainloop.xml 			\
	reference/pygio-classes.xml 			\
	reference/pygiounix-classes.xml			\
	reference/pygio-constants.xml			\
	reference/pygio-functions.xml			\
	reference/pygio-applaunchcontext.xml 		\
	reference/pygio-appinfo.xml 			\
	reference/pygio-asyncresult.xml 		\
	reference/pygio-bufferedinputstream.xml 	\
	reference/pygio-bufferedoutputstream.xml	\
	reference/pygio-cancellable.xml 		\
	reference/pygio-datainputstream.xml 		\
	reference/pygio-dataoutputstream.xml 		\
	reference/pygio-drive.xml 			\
	reference/pygio-emblem.xml			\
	reference/pygio-emblemedicon.xml		\
	reference/pygio-file.xml			\
	reference/pygio-fileattributeinfo.xml		\
	reference/pygio-fileenumerator.xml		\
	reference/pygio-fileicon.xml 			\
	reference/pygio-fileinfo.xml 			\
	reference/pygio-fileinputstream.xml 		\
	reference/pygio-filemonitor.xml			\
	reference/pygio-fileoutputstream.xml 		\
	reference/pygio-filterinputstream.xml 		\
	reference/pygio-filteroutputstream.xml 		\
	reference/pygio-icon.xml			\
	reference/pygio-inputstream.xml			\
	reference/pygio-loadableicon.xml		\
	reference/pygio-memoryinputstream.xml 		\
	reference/pygio-memoryoutputstream.xml 		\
	reference/pygio-mount.xml			\
	reference/pygio-mountoperation.xml		\
	reference/pygio-outputstream.xml		\
	reference/pygio-seekable.xml			\
	reference/pygio-simpleasyncresult.xml		\
	reference/pygio-themedicon.xml			\
	reference/pygio-unixinputstream.xml 		\
	reference/pygio-unixoutputstream.xml 		\
	reference/pygio-volume.xml 			\
	reference/pygio-volumemonitor.xml

XSL_FILES =			\
	xsl/common.xsl 		\
	xsl/devhelp.xsl		\
	xsl/html.xsl 		\
	xsl/pdf-style.xsl 	\
	xsl/pdf.xsl		\
	xsl/ref-html-style.xsl

# fixxref cannot be included in XSL_FILES, because automake is touching it
# so the date is newer then the files in HTML_FILES
FIXXREF = xsl/fixxref.py

TARGET_DIR = $(datadir)/gtk-doc/html/pygobject
CSSdir = $(TARGET_DIR)
CSS_DATA = $(CSS_FILES)


install-data-local:
	installfiles=`echo $(srcdir)/html/*`; \
	if test "$$installfiles" = '$(srcdir)/html/*'; \
	then echo '-- Nothing to install' ; \
	else \
	  $(mkinstalldirs) $(DESTDIR)$(TARGET_DIR); \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR) || :; \
	fi

uninstall-local:
	rm -f $(DESTDIR)$(TARGET_DIR)/*


XSLdir = $(datadir)/pygobject/xsl
XSL_DATA = $(XSL_FILES) $(FIXXREF)

EXTRA_DIST = 		\
	$(HTML_FILES) 	\
	$(XML_FILES) 	\
	$(XSL_FILES) 	\
	$(FIXXREF) 	\
	$(CSS_FILES)

reference/builddate.xml:
	@$(PYTHON) -c 'import datetime; print datetime.date.today()' > $@


if ENABLE_DOCS

BUILT_SOURCES = reference/builddate.xml $(HTML_FILES)
CLEANFILES = reference/builddate.xml $(HTML_FILES)


# Do not add a builddate.xml dependency here, because the generated html 
# documentation will be included in the tarball so xsltproc is not needed to 
# install the documentation. Adding a builddate.xml dependency will force 
# the documentation to be rebuilt when it doesn't need to be.
$(HTML_FILES): $(XML_FILES) $(XSL_FILES)
	@xsltproc --nonet --xinclude -o $(BUILDDIR)/html/ \
		 --path $(BUILDDIR)/reference:$(srcdir)/reference \
                 --stringparam gtkdoc.bookname "pygobject" \
                 --stringparam gtkdoc.version ${REF_VERSION} \
                 $(srcdir)/xsl/ref-html-style.xsl \
                 $(srcdir)/reference/pygobject-ref.xml

# pdf: $(REFERENCE_DEPS) reference/builddate.xml
# 	xsltproc --nonet --xinclude -o pygobject-ref.fo xsl/pdf-style.xsl \
# 		reference/pygobject-ref.xml
# 	pdfxmltex pygobject-ref.fo >output </dev/null

endif

-include $(top_srcdir)/git.mk
