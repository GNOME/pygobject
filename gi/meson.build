sources = [
  'pygboxed.c',
  'pygenum.c',
  'pygflags.c',
  'pyginterface.c',
  'pygobject-object.c',
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
  'pygi-fundamental.c',
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
  '_gtktemplate.py',
  'importer.py',
  '__init__.py',
  'module.py',
  '_option.py',
  '_ossighelper.py',
  '_propertyhelper.py',
  'pygtkcompat.py',
  '_signalhelper.py',
  'types.py',
]

python.install_sources(python_sources,
  pure : false,
  subdir : 'gi'
)

# https://github.com/mesonbuild/meson/issues/4117
if host_machine.system() == 'windows'
  python_ext_dep = python_dep
else
  python_ext_dep = python_dep.partial_dependency(compile_args: true)
endif

giext = python.extension_module('_gi', sources,
  dependencies : [python_ext_dep, glib_dep, gi_dep, ffi_dep],
  include_directories: include_directories('..'),
  install: true,
  subdir : 'gi',
  c_args: pyext_c_args + main_c_args
)

if cairo_dep.found()
  gicairoext = python.extension_module('_gi_cairo', ['pygi-foreign-cairo.c'],
    dependencies : [python_ext_dep, glib_dep, gi_dep, ffi_dep, pycairo_dep, cairo_dep, cairo_gobject_dep],
    install: true,
    subdir : 'gi',
    c_args: pyext_c_args + main_c_args)
endif

subdir('overrides')
subdir('repository')
