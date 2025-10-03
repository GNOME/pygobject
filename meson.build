project(
  'pygobject',
  'c',
  version : '3.55.0',
  meson_version : '>= 0.64.0',
  default_options : ['warning_level=1', 'buildtype=debugoptimized']
)

pygobject_version = meson.project_version()
version_arr = pygobject_version.split('.')
pygobject_version_major = version_arr[0].to_int()
pygobject_version_minor = version_arr[1].to_int()
pygobject_version_micro = version_arr[2].to_int()

platform_version = '@0@.0'.format(pygobject_version_major)

pymod = import('python')
python = pymod.find_installation(get_option('python'), pure: false)

if python.language_version().version_compare('< 3.9')
  error('Requires Python >= 3.9')
endif

python_dep = python.dependency()

glib_version_req = '>= 2.80.0'
gi_version_req = '>= 1.64.0'
pycairo_version_req = '>= 1.16.0'
libffi_version_req = '>= 3.0'

gi_dep = dependency('girepository-2.0', version : glib_version_req,
  fallback: ['glib', 'libgirepository_dep'])
glib_dep = dependency('glib-2.0', version : glib_version_req,
  fallback: ['glib', 'libglib_dep'], default_options: ['-Dintrospection=enabled'])
gobject_dep = dependency('gobject-2.0', version : glib_version_req,
  fallback: ['glib', 'libgobject_dep'])
gio_dep = dependency('gio-2.0', version : glib_version_req,
  fallback: ['glib', 'libgio_dep'])
ffi_dep = dependency('libffi', version : libffi_version_req,
  fallback : ['libffi', 'ffi_dep'])

with_pycairo = get_option('pycairo')

cc = meson.get_compiler('c')

pythoncapi_compat = subproject('pythoncapi-compat')
pythoncapi_compat_dep = declare_dependency(
    include_directories: pythoncapi_compat.get_variable('incdir')
)

if not with_pycairo.disabled()
  cairo_dep = dependency('cairo', required: with_pycairo.enabled() and cc.get_id() != 'msvc')
  cairo_gobject_dep = dependency('cairo-gobject', required: with_pycairo.enabled() and cc.get_id() != 'msvc')

  if cc.get_id() == 'msvc' and (not cairo_gobject_dep.found() or not cairo_dep.found())
    if cc.has_header('cairo.h') and cc.has_header ('cairo-gobject.h')
      cairo_dep = cc.find_library ('cairo', required: with_pycairo)
      cairo_gobject_dep = cc.find_library ('cairo-gobject', required: with_pycairo)
    endif
  endif

  # Find pycairo with target Python (TODO: extract version as well, see setup.py)
  r = run_command(
    python,
    '-c',
    '''
import pathlib, sys
from importlib.util import find_spec
spec = find_spec("cairo")
if spec is None:
  sys.stderr.write("cairo module spec not found")
  sys.exit(1)
src_path = pathlib.Path(r"@0@")
inc_path = pathlib.Path(spec.origin).parent / "include"
try:
  sys.stdout.write(str(inc_path.relative_to(src_path)))
except ValueError:
  sys.stdout.write(str(inc_path))
'''.format(meson.current_source_dir()),
    check: false,
  )

  if r.returncode() == 0
    message('Found pycairo with target Python: ' + r.stdout())
    pycairo_inc_dir = include_directories(r.stdout())
    pycairo_dep = declare_dependency(
      include_directories: pycairo_inc_dir,
    )
  else
    message('pycairo not found via target Python, falling back to pkg-config (@0@)'.format(r.stderr()))
    # Find pycairo with pkg-config
    pycairo_dep = dependency(
      'py3cairo',
      version: pycairo_version_req,
      fallback: ['pycairo', 'pycairo_dep'],
      default_options: ['python=' + get_option('python')],
      required: with_pycairo,
    )
  endif
else
  cairo_dep = dependency('', required: false)
  pycairo_dep = dependency('', required: false)
endif

main_c_args = []
if cc.get_id() == 'msvc'
  main_c_args += [ '-FImsvc_recommended_pragmas.h' ]
else
  main_c_args += [
    '-Wall',
    '-Warray-bounds',
    '-Wcast-align',
    '-Wduplicated-branches',
    '-Wextra',
    '-Wformat=2',
    '-Wformat-nonliteral',
    '-Wformat-security',
    '-Wimplicit-function-declaration',
    '-Winit-self',
    '-Wjump-misses-init',
    '-Wlogical-op',
    '-Wmissing-declarations',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-prototypes',
    '-Wnull-dereference',
    '-Wold-style-definition',
    '-Wpacked',
    '-Wpointer-arith',
    '-Wrestrict',
    '-Wreturn-type',
    '-Wshadow',
    '-Wsign-compare',
    '-Wstrict-aliasing',
    '-Wstrict-prototypes',
    '-Wswitch-default',
    '-Wundef',
    '-Wunused-but-set-variable',
    '-Wwrite-strings',
  ]

  main_c_args += [
    '-Wno-incompatible-pointer-types-discards-qualifiers',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wno-discarded-qualifiers',
    '-Wno-sign-conversion',
    '-Wno-cast-function-type',
    '-Wno-int-conversion',
  ]

  main_c_args += [
    '-fno-strict-aliasing',
    '-fvisibility=hidden',
  ]

  # On MSys2, the compiler errors 'cause it wants a `gnu_printf` format attributes
  if host_machine.system() == 'windows' and cc.get_id() == 'gcc'
    main_c_args += ['-Wno-suggest-attribute=format']
  endif

  main_c_args = cc.get_supported_arguments(main_c_args)
endif

pyext_c_args = ['-DPY_SSIZE_T_CLEAN']

cdata = configuration_data()

cdata.set('PYGOBJECT_MAJOR_VERSION', pygobject_version_major)
cdata.set('PYGOBJECT_MINOR_VERSION', pygobject_version_minor)
cdata.set('PYGOBJECT_MICRO_VERSION', pygobject_version_micro)

if gio_dep.version().version_compare('< 2.67.4')
  cdata.set('g_memdup2(ptr,sz)', '(G_LIKELY(((guint64)(sz)) < G_MAXUINT)) ? g_memdup(ptr,sz) : (g_abort(),NULL)')
endif

configure_file(output : 'config.h', configuration : cdata)

pkgconf = configuration_data()

pkgconf.set('prefix', join_paths(get_option('prefix')))
pkgconf.set('exec_prefix', '${prefix}')
pkgconf.set('includedir', join_paths('${prefix}', get_option('includedir')))
pkgconf.set('datarootdir', join_paths('${prefix}', get_option('datadir')))
pkgconf.set('datadir', '${datarootdir}')
pkgconf.set('VERSION', pygobject_version)

for_wheel = get_option('wheel')
if not for_wheel
  pkg_install_dir = '@0@/pkgconfig'.format(get_option('libdir'))

  configure_file(input : 'pygobject-@0@.pc.in'.format(platform_version),
    output : 'pygobject-@0@.pc'.format(platform_version),
    configuration : pkgconf,
    install_dir : pkg_install_dir)

  if pygobject_version_minor.is_odd()
    py_version = '@0@.dev0'.format(pygobject_version)
  else
    py_version = pygobject_version
  endif

  pkginfo_conf = configuration_data()
  pkginfo_conf.set('VERSION', py_version)
  configure_file(input : 'METADATA.in',
  output : 'METADATA',
  configuration : pkginfo_conf,
  install_dir : python.get_install_dir(pure : false) / 'PyGObject-@0@.dist-info'.format(py_version))
endif

pygobject_dep = declare_dependency(
  include_directories: include_directories('gi'),
  dependencies: [gobject_dep, ffi_dep],
  version: meson.project_version(),
)


subdir('gi')
subdir('pygtkcompat')

with_tests = get_option('tests')
if with_tests
  gi_tests = subproject('gobject-introspection-tests', default_options : ['cairo=' + cairo_dep.found().to_string()])

  # We cannot depend on gobject-introspection-1.0 fallback if we also need GLib to build.
  # All we need is the g-ir-scanner and friends
  if glib_dep.type_name() != 'internal'
    dependency('gobject-introspection-1.0', version : gi_version_req, fallback: ['gobject-introspection', 'girepo_dep'])
  endif
  subdir('tests')
endif

devenv = environment()
devenv.prepend('PYTHONPATH', [
    meson.current_source_dir(),
    meson.current_build_dir(),
])
meson.add_devenv(devenv)
