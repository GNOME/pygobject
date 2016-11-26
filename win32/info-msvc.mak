# NMake Makefile portion for displaying config info

WITH_CAIRO = no

!if "$(HAVE_PYCAIRO)" == "1"
WITH_CAIRO = yes
!endif

!if "$(PYTHONCOMPILER)" != "MSC"
WARN_MESSAGE=WARNING: Your Python installation is not built with Visual Studio
!else
!if "$(PYTHONCOMPILERVER)" == "15"
PYMSCVER = 9
!elseif "$(PYTHONCOMPILERVER)" == "16"
PYMSCVER = 10
!elseif "$(PYTHONCOMPILERVER)" == "17"
PYMSCVER = 11
!elseif "$(PYTHONCOMPILERVER)" == "18"
PYMSCVER = 12
!elseif "$(PYTHONCOMPILERVER)" == "19"
PYMSCVER = 14
!endif

!if "$(VSVER)" == "$(PYMSCVER)"
WARN_MESSAGE=No compiler version related warnings
!else
WARN_MESSAGE=WARNING: You are building PyGObject using vs$(VSVER) but Python is built with vs$(PYMSCVER).
!endif
!endif



build-info-pygobject:
	@echo.
	@echo ===========================
	@echo Configuration for PyGObject
	@echo ===========================
	@echo Build Type: $(CFG)
	@echo Python Version Series: $(PYTHONSERIESDOT)
	@echo Cairo Support: $(WITH_CAIRO)
	@echo $(WARN_MESSAGE)

all-build-info: build-info-pygobject

help:
	@echo.
	@echo =============================
	@echo Building PyGObject Using NMake
	@echo =============================
	@echo nmake /f Makefile.vc CFG=[release^|debug] ^<PREFIX=PATH^> ^<PYTHON=PATH^>
	@echo.
	@echo Where:
	@echo ------
	@echo CFG: Required, use CFG=release for an optimized build and CFG=debug
	@echo for a debug build.  PDB files are generated for all builds.
	@echo.
	@echo PYTHON: Required, unless python.exe is already in your PATH.
	@echo set to ^$(FULL_PATH_TO_PYTHON_INTERPRETOR).
	@echo.
	@echo PREFIX: Optional, the path where dependent libraries and tools may be
	@echo found, default is ^$(srcrootdir)\..\vs^$(short_vs_ver)\^$(platform),
	@echo where ^$(short_vs_ver) is 9 for VS 2008, 10 for VS 2010 and so on; and
	@echo ^$(platform) is Win32 for 32-bit builds and x64 for x64 builds.
	@echo ======
	@echo A 'clean' target is supported to remove all generated files, intermediate
	@echo object files and binaries for the specified configuration.
	@echo.
	@echo A 'tests' target is supported to build the test programs, use after building
	@echo the main PyGObject Python Module.
	@echo.
	@echo An 'install' target is supported to copy the build to appropriate
	@echo locations under ^$(srcroot)\build\ for preparation for packaging and/or
	@echo testing by setting PYTHONPATH.
	@echo ======
	@echo.
	
