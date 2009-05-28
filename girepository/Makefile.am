AUTOMAKE_OPTIONS = 1.7
PLATFORM_VERSION = 2.0

INCLUDES = \
	-I$(top_srcdir)/gobject \
	$(PYTHON_INCLUDES) \
	$(PYGOBJECT_CFLAGS) \
	$(GOBJECT_INTROSPECTION_CFLAGS)

pkginclude_HEADERS = bank.h

# girepository extension modules
pkgpyexecdir = $(pyexecdir)/gtk-2.0/girepository

# girepository python scripts
pygirepositorydir = $(pkgpyexecdir)
pygirepository_PYTHON = \
	__init__.py \
	importer.py \
	module.py \
	repository.py \
	btypes.py

# linker flags
common_ldflags = -module -avoid-version
if PLATFORM_WIN32
common_ldflags += -no-undefined
endif

CLEANFILES =
EXTRA_DIST = 

# repo module
repo_la_CFLAGS = -O0
repo_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initrepo
repo_la_LIBADD = $(GOBJECT_INTROSPECTION_LIBS)
repo_la_SOURCES = \
	bank-repository.c \
	bank-argument.c \
	bank-info.c \
	bank.c \
	bank.h

if BUILD_GOBJECT_INTROSPECTION
pygirepository_LTLIBRARIES = repo.la

all: $(pygirepository_LTLIBRARIES:.la=.so)
clean-local:
	rm -f $(pygirepository_LTLIBRARIES:.la=.so)
.la.so:
	$(LN_S) .libs/$@ $@ || true
endif
