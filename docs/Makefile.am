REF_VERSION = $(PYGOBJECT_MAJOR_VERSION).$(PYGOBJECT_MINOR_VERSION)
FULL_VERSION = $(REF_VERSION).$(PYGOBJECT_MICRO_VERSION)

XMLFILES = 					\
	reference/pygobject.xml			\
	reference/pygobject-introduction.xml 	\
	reference/pygobject-classes.xml 	\
	reference/pygobject-constants.xml 	\
	reference/pygobject-ref.xml		\
	reference/pygobject-functions.xml	\
	reference/pygobject-gboxed.xml		\
	reference/pygobject-ginterface.xml 	\
	reference/pygobject-gpointer.xml 	\
	reference/pygobject-maincontext.xml 	\
	reference/pygobject-mainloop.xml

XSLFILES =			\
	xsl/common.xsl 		\
	xsl/devhelp.xsl		\
	xsl/fixxref.py		\
	xsl/html.xsl 		\
	xsl/pdf-style.xsl 	\
	xsl/pdf.xsl		\
	xsl/ref-html-style.xsl

XSLdir = $(datadir)/pygobject/xsl
XSL_DATA = $(XSLFILES)

BUILDDIR = $(top_builddir)/docs

HTMLdir = $(datadir)/gtk-doc/html/pygobject
HTML_DATA = 					\
	html/pygobject-introduction.html	\
	html/pygobject-reference-copyright.html	\
	html/class-gobjectgboxed.html		\
	html/class-gobjectginterface.html	\
	html/class-gobjectgpointer.html		\
	html/class-gobject.html			\
	html/class-gobjectmaincontext.html	\
	html/class-gobjectmainloop.html		\
	html/gobject-class-reference.html	\
	html/gobject-constants.html		\
	html/gobject-functions.html		\
	html/index.html				\
	html/index.sgml				\
	html/pygobject.devhelp

BUILT_SOURCES = 			\
	build_stamp 			\
	$(HTML_DATA)

CLEANFILES = 				\
	build_stamp 			\
	pygobject-ref.*			\
	$(HTML_DATA)

EXTRA_DIST = 				\
	$(XMLFILES) 			\
	$(XSLFILES)

REFERENCE_DEPS = 			\
	reference 			\
	$(XMLFILES)			\
	$(XSLFILES)

$(HTML_DATA): build_stamp

build_stamp: $(REFERENCE_DEPS)
	xsltproc --nonet --xinclude -o $(BUILDDIR)/html/ \
		 --path $(BUILDDIR)/reference:$(srcdir)/reference \
                 --stringparam gtkdoc.bookname "pygobject" \
                 --stringparam gtkdoc.version ${REF_VERSION} \
                 $(srcdir)/xsl/ref-html-style.xsl \
                 $(srcdir)/reference/pygobject-ref.xml
	touch $@

pdf: $(REFERENCE_DEPS)
	xsltproc --nonet --xinclude -o pygobject-ref.fo xsl/pdf-style.xsl \
		reference/pygobject-ref.xml
	pdfxmltex pygobject-ref.fo >output </dev/null

