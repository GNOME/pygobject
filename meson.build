project('pygobject', 'c',
  version : '3.31.4',
  meson_version : '>= 0.46.0',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized'])

pygobject_version = meson.project_version()
version_arr = pygobject_version.split('.')
pygobject_version_major = version_arr[0].to_int()
pygobject_version_minor = version_arr[1].to_int()
pygobject_version_micro = version_arr[2].to_int()

platform_version = '@0@.0'.format(pygobject_version_major)

pymod = import('python')
python = pymod.find_installation(get_option('python'))

python_dep = python.dependency()

glib_version_req = '>= 2.48.0'
gi_version_req = '>= 1.46.0'
pycairo_version_req = '>= 1.11.1'
libffi_version_req = '>= 3.0'

gi_dep = dependency('gobject-introspection-1.0', version : gi_version_req,
  fallback: ['gobject-introspection', 'girepo_dep'])
glib_dep = dependency('glib-2.0', version : glib_version_req,
  fallback: ['glib', 'libglib_dep'])
gobject_dep = dependency('gobject-2.0', version : glib_version_req,
  fallback: ['glib', 'libgobject_dep'])
gio_dep = dependency('gio-2.0', version : glib_version_req,
  fallback: ['glib', 'libgio_dep'])
gmodule_dep = dependency('gmodule-2.0', version : glib_version_req,
  fallback: ['glib', 'libgmodule_dep'])
ffi_dep = dependency('libffi', version : '>= 3.0',
  fallback : ['libffi', 'ffi_dep'])

with_pycairo = get_option('pycairo')

cc = meson.get_compiler('c')

if with_pycairo
  cairo_dep = dependency('cairo', required: cc.get_id() != 'msvc')
  cairo_gobject_dep = dependency('cairo-gobject', required: cc.get_id() != 'msvc')

  if cc.get_id() == 'msvc' and (not cairo_gobject_dep.found() or not cairo_dep.found())
    if cc.has_header('cairo.h') and cc.has_header ('cairo-gobject.h')
      cairo_dep = cc.find_library ('cairo')
      cairo_gobject_dep = cc.find_library ('cairo-gobject')
    endif
  endif

  if python.language_version().version_compare('>= 3.0')
    pycairo_name = 'py3cairo'
  else
    pycairo_name = 'pycairo'
  endif

  pycairo_dep = dependency(
    pycairo_name,
    version: pycairo_version_req,
    fallback: ['pycairo', 'pycairo_dep'],
    default_options: ['python=' + get_option('python')],
  )
endif

main_c_args = []
if cc.get_id() == 'msvc'
  main_c_args += [ '-FImsvc_recommended_pragmas.h' ]
else
  main_c_args += [
    '-Wall',
    '-Warray-bounds',
    '-Wcast-align',
    '-Wdeclaration-after-statement',
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
    '-Wnested-externs',
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

  if not ['3.3', '3.4'].contains(python.language_version())
    main_c_args += [
      '-Wswitch-default',
    ]
  endif

  main_c_args = cc.get_supported_arguments(main_c_args)
endif

pyext_c_args = ['-DPY_SSIZE_T_CLEAN']

cdata = configuration_data()

cdata.set('PYGOBJECT_MAJOR_VERSION', pygobject_version_major)
cdata.set('PYGOBJECT_MINOR_VERSION', pygobject_version_minor)
cdata.set('PYGOBJECT_MICRO_VERSION', pygobject_version_micro)

configure_file(output : 'config.h', configuration : cdata)

pkgconf = configuration_data()

pkgconf.set('prefix', join_paths(get_option('prefix')))
pkgconf.set('exec_prefix', '${prefix}')
pkgconf.set('includedir', join_paths('${prefix}', get_option('includedir')))
pkgconf.set('datarootdir', join_paths('${prefix}', get_option('datadir')))
pkgconf.set('datadir', '${datarootdir}')
pkgconf.set('VERSION', pygobject_version)

pkg_install_dir = '@0@/pkgconfig'.format(get_option('libdir'))

configure_file(input : 'pygobject-@0@.pc.in'.format(platform_version),
  output : 'pygobject-@0@.pc'.format(platform_version),
  configuration : pkgconf,
  install_dir : pkg_install_dir)

pygobject_dep = declare_dependency(
  include_directories: include_directories('gi'),
  dependencies: [gobject_dep, ffi_dep],
  version: meson.project_version(),
)

if pygobject_version_minor.is_odd()
  py_version = '@0@.dev0'.format(pygobject_version)
else
  py_version = pygobject_version
endif

pkginfo_conf = configuration_data()
pkginfo_conf.set('VERSION', py_version)
configure_file(input : 'PKG-INFO.in',
  output : 'PyGObject-@0@.egg-info'.format(py_version),
  configuration : pkginfo_conf,
  install_dir : python.get_install_dir(pure : false))

subdir('gi')
subdir('pygtkcompat')
subdir('tests')
