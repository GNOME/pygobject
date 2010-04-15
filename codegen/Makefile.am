PLATFORM_VERSION = 2.0

SUBDIRS = libcodegen

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

EXTRA_DIST = $(codegen_SCRIPTS) README.defs pygobject-codegen-$(PLATFORM_VERSION).in
