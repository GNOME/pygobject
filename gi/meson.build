sources = [
  'pygboxed.c',
  'pygenum.c',
  'pygflags.c',
  'pyginterface.c',
  'pygobject-object.c',
  'pygparamspec.c',
  'pygpointer.c',
  'pygoptioncontext.c',
  'pygoptiongroup.c',
  'pygspawn.c',
  'gimodule.c',
  'pygi-repository.c',
  'pygi-info.c',
  'pygi-foreign.c',
  'pygi-struct.c',
  'pygi-source.c',
  'pygi-argument.c',
  'pygi-resulttuple.c',
  'pygi-type.c',
  'pygi-boxed.c',
  'pygi-closure.c',
  'pygi-ccallback.c',
  'pygi-util.c',
  'pygi-property.c',
  'pygi-signal-closure.c',
  'pygi-invoke.c',
  'pygi-cache.c',
  'pygi-marshal-cleanup.c',
  'pygi-basictype.c',
  'pygi-list.c',
  'pygi-array.c',
  'pygi-error.c',
  'pygi-object.c',
  'pygi-value.c',
  'pygi-enum-marshal.c',
  'pygi-struct-marshal.c',
  'pygi-hashtable.c']

headers = [
  'pygobject.h'
]

install_headers(headers, subdir : 'pygobject-@0@'.format(platform_version))

python_sources = [
  '_constants.py',
  'docstring.py',
  '_error.py',
  'importer.py',
  '__init__.py',
  'module.py',
  '_option.py',
  '_ossighelper.py',
  '_propertyhelper.py',
  'pygtkcompat.py',
  '_signalhelper.py',
  'types.py',
  '_compat.py']

python.install_sources(python_sources,
  pure : false,
  subdir : 'gi'
)

giext = python.extension_module('_gi', sources,
  dependencies : [python_dep, glib_dep, gi_dep, ffi_dep],
  include_directories: include_directories('..'),
  install: true,
  subdir : 'gi',
  c_args: ['-DHAVE_CONFIG_H', '-DPY_SSIZE_T_CLEAN'] + python_no_warn_flags
)

if pycairo_dep.found()
  gicairoext = python.extension_module('_gi_cairo', ['pygi-foreign-cairo.c'],
    dependencies : [python_dep, glib_dep, gi_dep, ffi_dep, pycairo_dep],
    c_args: ['-DHAVE_CONFIG_H', '-DPY_SSIZE_T_CLEAN'] + python_no_warn_flags)
endif

subdir('overrides')
subdir('repository')
