project('pygobject', 'c',
  version : '3.29.2',
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

glib_version_req = '>= 2.38.0'
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

if with_pycairo
  cairo_dep = dependency('cairo')
  cairo_gobject_dep = dependency('cairo-gobject')

  if python.language_version().version_compare('>= 3.0')
    pycairo_dep = dependency('py3cairo', version : pycairo_version_req)
  else
    pycairo_dep = dependency('pycairo', version : pycairo_version_req)
  endif
endif

cc = meson.get_compiler('c')

main_c_args = [
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
  '-Winline',
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
  '-Wconversion',
]

main_c_args += [
  '-Wno-incompatible-pointer-types-discards-qualifiers',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wno-discarded-qualifiers',
  '-Wno-sign-conversion',
  '-Wno-cast-function-type',
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

configure_file(input : 'PKG-INFO.in',
  output : 'pygobject-@0@-py@1@.egg-info'.format(pygobject_version, python.language_version()),
  configuration : pkgconf,
  install_dir : python.get_install_dir(subdir : 'gi'))

subdir('gi')
subdir('pygtkcompat')
subdir('tests')
