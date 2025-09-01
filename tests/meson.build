gnome = import('gnome')

host_system = host_machine.system()

cc = meson.get_compiler('c')

helper_sources = [
  'testhelpermodule.c',
  'test-floating.c',
  'test-thread.c',
  'test-unknown.c']

helperext = python.extension_module('testhelper', helper_sources,
  dependencies : [python_dep, glib_dep, gobject_dep, pythoncapi_compat_dep],
  c_args: pyext_c_args + main_c_args,
  include_directories: include_directories(join_paths('..', 'gi'))
)

schemas = gnome.compile_schemas(build_by_default: true)

envdata = environment()
if host_machine.system() == 'windows'
    envdata.prepend('PATH', join_paths(get_option('prefix'), get_option('bindir')))
endif

test_deps = [
    giext,
    helperext,
    schemas,
    gi_tests.get_variable('gimarshallingtests_typelib'),
    gi_tests.get_variable('regress_typelib'),
    gi_tests.get_variable('utility_typelib')]

python_paths = [join_paths(meson.current_build_dir(), '..')]
if pycairo_dep.found() and pycairo_dep.type_name() == 'internal'
  python_paths += [join_paths(meson.project_build_root(), 'subprojects', 'pycairo')]
  test_deps += [gicairoext]
endif
envdata.append('PYTHONPATH', python_paths)

gi_tests_builddir = meson.project_build_root() / 'subprojects' / 'gobject-introspection-tests'
envdata.prepend('GI_TYPELIB_PATH', gi_tests_builddir)

test('pygobject-test-suite',
  python,
  args: ['-m', 'pytest'],
  workdir: meson.project_source_root(),
  env: envdata,
  timeout: 90,
  depends: test_deps)
