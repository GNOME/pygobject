!if ![$(PYTHON) configpy.py]
!endif

!if !exists(configpy.mak)
!error The path '$(PYTHON)' specified for PYTHON is invalid.
!endif

!include configpy.mak

!if "$(PYTHONPREFIX)" == ""
!error The path '$(PYTHON)' specified for PYTHON is not a valid Python installation.
!endif

!if ![del /f configpy.mak]
!endif
