# NMake Makefile snippet for copying the built modules to
# prepare for packaging

!if "$(USE_SETUP_TOOLS)" == ""
PYGOBJECT_PREFIX = $(PREFIX)
PYGOBJECT_LIBDIR = $(PREFIX)\lib
!else
PYGOBJECT_PREFIX = $(PYTHONPREFIX)
PYGOBJECT_LIBDIR = $(PYTHONPREFIX)\libs
!endif

PYGOBJECT_HEADER_DIR = $(PYGOBJECT_PREFIX)\include\pygobject-3.0
PYGOBJECT_PKGC_DIR = $(PYGOBJECT_PREFIX)\lib\pkgconfig

pygobject-3.0.pc: ..\pygobject-3.0.pc.in
	@$(PYTHON) pygobjectpc.py --prefix=$(PYGOBJECT_PREFIX) --libdir=$(PYGOBJECT_LIBDIR) --version=$(PYGI_MAJOR_VERSION).$(PYGI_MINOR_VERSION).$(PYGI_MICRO_VERSION)

install: prep-package pygobject-3.0.pc all-build-info

prep-package: $(PYGI_MODULES)
	@for /f %d in ('dir /b /ad ..\gi') do	\
	@(if not exist ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\%d mkdir ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\%d) & \
	@(copy ..\gi\%d\*.py ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\%d\)
	@copy ..\gi\*.py ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi
	@-mkdir ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\pygtkcompat
	@copy ..\pygtkcompat\*.py ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\pygtkcompat
