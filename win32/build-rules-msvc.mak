# NMake Makefile portion for compilation rules
# Items in here should not need to be edited unless
# one is maintaining the NMake build files.  The format
# of NMake Makefiles here are different from the GNU
# Makefiles.  Please see the comments about these formats.

# Inference rules for compiling the .obj files.
# Used for libs and programs with more than a single source file.
# Format is as follows
# (all dirs must have a trailing '\'):
#
# {$(srcdir)}.$(srcext){$(destdir)}.obj::
# 	$(CC)|$(CXX) $(cflags) /Fo$(destdir) /c @<<
# $<
# <<
{..\gi\}.c{..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\}.obj::
	$(CC) $(CFLAGS) $(PYGI_DEFINES) $(PYGI_CFLAGS) /Fo..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\ /c @<<
$<
<<

# Inference rules for building the test programs
# Used for programs with a single source file.
# Format is as follows
# (all dirs must have a trailing '\'):
#
# {$(srcdir)}.$(srcext){$(destdir)}.exe::
# 	$(CC)|$(CXX) $(cflags) $< /Fo$*.obj  /Fe$@ [/link $(linker_flags) $(dep_libs)]< /Fo$*.obj /Fe$@ /link $(LDFLAGS) $(CFG)\$(PLAT)\deplib.lib $(TEST_DEP_LIBS)

# Rules for linking DLLs
# Format is as follows (the mt command is needed for MSVC 2005/2008 builds):
# $(dll_name_with_path): $(dependent_libs_files_objects_and_items)
#	link /DLL [$(linker_flags)] [$(dependent_libs)] [/def:$(def_file_if_used)] [/implib:$(lib_name_if_needed)] -out:$@ @<<
# $(dependent_objects)
# <<
# 	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2
..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\_gi.pyd: ..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT) $(gi_pyd_OBJS) ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi
	link /DLL $(PYGI_LDFLAGS) $(PYGI_DEP_LIBS) -out:$@ -implib:..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\$(@B).lib @<<
$(gi_pyd_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2
	@-if exist $@.manifest del $@.manifest

..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\_gi_cairo.pyd: ..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT) $(gi_cairo_pyd_OBJS) ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi
	link /DLL $(PYGI_LDFLAGS) $(PYGI_DEP_LIBS) $(PYGI_CAIRO_DEP_LIBS) -out:$@ -implib:..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\$(@B).lib @<<
$(gi_cairo_pyd_OBJS)
<<
	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2
	@-if exist $@.manifest del $@.manifest

clean:
	@-del /f /q ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\*.pdb
	@-del /f /q ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\gi\*.pyd
	@-del /f /q ..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\*.ilk
	@-del /f /q ..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)\*.obj
	@-rmdir /s /q ..\build\lib.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)
	@-rmdir /s /q ..\build\temp.$(PYTHON_PLAT)-$(PYTHONSERIESDOT)
	@if exist __pycache__ rmdir /s /q __pycache__
	@-del /f /q *.pyc
	@-del /f /q pygobject-3.0.pc
	@-del /f /q vc$(VSVER)0.pdb
