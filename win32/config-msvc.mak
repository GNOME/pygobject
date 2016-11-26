# NMake Makefile portion for enabling features for Windows builds

# You may change these lines to customize the .lib files that will be linked to

# gobject-introspection, GLib and libffi are required for everything
PYGI_DEP_LIBS = girepository-1.0.lib gobject-2.0.lib glib-2.0.lib ffi.lib
PYGI_CAIRO_DEP_LIBS = cairo-gobject.lib cairo.lib

# Please do not change anything beneath this line unless maintaining the NMake Makefiles

!if "$(PLAT)" == "x64"
PYTHON_PLAT = win-amd64
!else
PYTHON_PLAT = win32
!endif

!if "$(PYCAIRO)" == "1"
!if ![echo PYCAIRO_CONFIG= > pycairoconfig.mak]
!endif

!if "$(PYTHONMAJ)" == "2"
!if ![if exist $(PYTHONPREFIX)\include\pycairo\pycairo.h echo HAVE_PYCAIRO=1 >> pycairoconfig.mak]
!endif
!else
!if ![if exist $(PYTHONPREFIX)\include\pycairo\py3cairo.h echo HAVE_PYCAIRO=1 >> pycairoconfig.mak]
!endif
!endif

!include pycairoconfig.mak

!if ![del /f pycairoconfig.mak]
!endif
!endif

!include pygobject-version.mak

PYGI_DEFINES =						\
	/DPY_SSIZE_T_CLEAN				\
	/DPYGOBJECT_MAJOR_VERSION=$(PYGI_MAJOR_VERSION)	\
	/DPYGOBJECT_MINOR_VERSION=$(PYGI_MINOR_VERSION)	\
	/DPYGOBJECT_MICRO_VERSION=$(PYGI_MICRO_VERSION)

PYGI_CFLAGS =				\
	/I..\gi 			\
	/I$(PREFIX)\include 		\
	/I$(PYTHONPREFIX)\include

PYGI_LDFLAGS =						\
	$(LDFLAGS)						\
	/libpath:$(PYTHONPREFIX)\libs

!if "$(HAVE_PYCAIRO)" == "1"
PYGI_CFLAGS = $(PYGI_CFLAGS) /I$(PYTHONPREFIX)\include\pycairo
!endif

!if "$(ADDITIONAL_LIB_DIR)" != ""
PYGI_LDFLAGS = /libpath:$(ADDITIONAL_LIB_DIR) $(PYGI_LDFLAGS)
!endif

PYGI_SOURCES =	$(pygi_module_sources)
PYGI_CAIRO_SOURCES = $(pygi_cairo_module_sources)

# We build the PyGI module at least
PYGI_MODULES = ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\_gi.pyd

!if "$(HAVE_PYCAIRO)" == "1"
PYGI_MODULES = $(PYGI_MODULES) ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\_gi_cairo.pyd
!endif
