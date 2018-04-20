project('pygobject', 'c',
  version : '3.27.6',
  meson_version : '>= 0.36.0',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized'])

pygobject_version = meson.project_version()
version_arr = pygobject_version.split('.')
pygobject_version_major = version_arr[0].to_int()
pygobject_version_minor = version_arr[1].to_int()
pygobject_version_micro = version_arr[2].to_int()

platform_version = '@0@.0'.format(pygobject_version_major)

pymod = import('python')
python = pymod.find_installation(get_option('python'), required: true)

python_dep = python.dependency(required : true)
python_no_warn_flags = []

gi_dep = dependency('gobject-introspection-1.0', version : '>= 1.55.0', required : true,
  fallback: ['gobject-introspection', 'girepo_dep'])
glib_dep = dependency('glib-2.0', version : '>= 2.55.0', required : true,
  fallback: ['glib', 'libglib_dep'])
gobject_dep = dependency('gobject-2.0', version : '>= 2.55.0', required : true,
  fallback: ['glib', 'libgobject_dep'])
gio_dep = dependency('gio-2.0', version : '>= 2.55.0', required : true,
  fallback: ['glib', 'libgio_dep'])
gmodule_dep = dependency('gmodule-2.0', version : '>= 2.55.0',
  fallback: ['glib', 'libgmodule_dep'])
ffi_dep = dependency('libffi', version : '>= 3.0', required : true,
  fallback : ['libffi', 'ffi_dep'])

cairo_dep = dependency('cairo', required: false)
cairo_gobject_dep = dependency('cairo-gobject', required: false)

if python.language_version().version_compare('>= 3.0')
  pycairo_dep = dependency('py3cairo', required: false)
else
  pycairo_dep = dependency('pycairo', required: false)
endif

cdata = configuration_data()

cdata.set('PYGOBJECT_MAJOR_VERSION', pygobject_version_major)
cdata.set('PYGOBJECT_MINOR_VERSION', pygobject_version_minor)
cdata.set('PYGOBJECT_MICRO_VERSION', pygobject_version_micro)

configure_file(output : 'config.h', configuration : cdata)

pkgconf = configuration_data()

pkgconf.set('prefix', join_paths(get_option('prefix')))
pkgconf.set('exec_prefix', '${prefix}')
pkgconf.set('includedir', '${prefix}/include')
pkgconf.set('datarootdir', '${prefix}/share')
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
