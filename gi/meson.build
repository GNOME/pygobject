gi_c_sources = files(
  'pygboxed.c',
  'pygenum.c',
  'pygflags.c',
  'pyginterface.c',
  'pygobject-class.c',
  'pygobject-object.c',
  'pygobject-props.c',
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
  'pygi-async.c',
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
  'pygi-hashtable.c')

gi_cairo_c_sources = files('pygi-foreign-cairo.c')

headers = files(
  'pygobject.h'
)

install_headers(headers, subdir : 'pygobject-@0@'.format(platform_version))

python_sources = files(
  'events.py',
  '_constants.py',
  'docstring.py',
  '_enum.py',
  '_error.py',
  '_gi.py',
  '_gtktemplate.py',
  'importer.py',
  '__init__.py',
  'module.py',
  '_ossighelper.py',
  '_propertyhelper.py',
  'pygtkcompat.py',
  '_signalhelper.py',
  '_signature.py',
  'types.py',
)

python.install_sources(python_sources,
  pure : false,
  subdir : 'gi'
)

configure_file(output : 'config.h', configuration : cdata)

subdir('overrides')
subdir('repository')
