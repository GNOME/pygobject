REF_VERSION = $(PYGOBJECT_MAJOR_VERSION).$(PYGOBJECT_MINOR_VERSION)
FULL_VERSION = $(REF_VERSION).$(PYGOBJECT_MICRO_VERSION)

BUILDDIR = $(top_builddir)/docs

CSS_FILES = style.css

HTML_FILES =					\
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
	html/pygobject.devhelp			\
	html/pygobject-introduction.html

if ENABLE_DOCS
BUILT_HTML_FILES = $(HTML_FILES)
else
BUILT_HTML_FILES =
endif

XML_FILES = 					\
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

CSSdir = $(HTMLdir)
CSS_DATA = $(CSS_FILES)

HTMLdir = $(datadir)/gtk-doc/html/pygobject
HTML_DATA = $(HTML_FILES)

XSLdir = $(datadir)/pygobject/xsl
XSL_DATA = $(XSL_FILES) $(FIXXREF)

BUILT_SOURCES = reference/builddate.xml $(BUILT_HTML_FILES)

CLEANFILES = reference/builddate.xml $(BUILT_HTML_FILES)

EXTRA_DIST = 		\
	$(HTML_FILES) 	\
	$(XML_FILES) 	\
	$(XSL_FILES) 	\
	$(FIXXREF) 	\
	$(CSS_FILES)

reference/builddate.xml:
	@$(PYTHON) -c 'import datetime; print datetime.date.today()' > $@

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

