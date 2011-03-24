PLATFORM_VERSION = 2.0

bin_SCRIPTS = pygobject-codegen-$(PLATFORM_VERSION)

codegendir = $(pkgdatadir)/$(PLATFORM_VERSION)/codegen

codegen_SCRIPTS = \
	code-coverage.py \
	codegen.py \
	createdefs.py \
	defsconvert.py \
	defsgen.py \
	docextract_to_xml.py \
	docgen.py \
	h2def.py \
	mergedefs.py \
	missingdefs.py \
	mkskel.py \
	scanvirtuals.py \
	scmexpr.py

codegen_PYTHON = \
	__init__.py \
	argtypes.py \
	definitions.py \
	defsparser.py \
	docextract.py \
	override.py \
	reversewrapper.py

EXTRA_DIST = $(codegen_SCRIPTS) README.defs pygobject-codegen-$(PLATFORM_VERSION).in

-include $(top_srcdir)/git.mk
