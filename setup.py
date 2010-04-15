#!/usr/bin/env python
#
# setup.py - distutils configuration for pygobject
#
"""Python Bindings for GObject."""

from distutils.command.build import build
from distutils.command.build_clib import build_clib
from distutils.sysconfig import get_python_inc
from distutils.core import setup
import glob
import os
import sys

from dsextras import get_m4_define, getoutput, have_pkgconfig, \
     GLOBAL_INC, GLOBAL_MACROS, InstallLib, InstallData, BuildExt, \
     PkgConfigExtension, TemplateExtension, \
     pkgc_get_libraries, pkgc_get_library_dirs, pkgc_get_include_dirs

if '--yes-i-know-its-not-supported' in sys.argv:
    sys.argv.remove('--yes-i-know-its-not-supported')
else:
    print '*'*70
    print 'Building PyGObject using distutils is NOT SUPPORTED.'
    print "It's mainly included to be able to easily build win32 installers"
    print "You may continue, but only if you agree to not ask any questions"
    print "To build PyGObject in a supported way, read the INSTALL file"
    print
    print "Build fixes are of course welcome and should be filed in bugzilla"
    print '*'*70
    input = raw_input('Not supported, ok [y/N]? ')
    if not input.startswith('y'):
        raise SystemExit("Aborted")

MIN_PYTHON_VERSION = (2, 3, 5)

MAJOR_VERSION = int(get_m4_define('pygobject_major_version'))
MINOR_VERSION = int(get_m4_define('pygobject_minor_version'))
MICRO_VERSION = int(get_m4_define('pygobject_micro_version'))

VERSION = "%d.%d.%d" % (MAJOR_VERSION, MINOR_VERSION, MICRO_VERSION)

GLIB_REQUIRED  = get_m4_define('glib_required_version')

PYGOBJECT_SUFFIX = '2.0'
PYGOBJECT_SUFFIX_LONG = 'gtk-' + PYGOBJECT_SUFFIX

GLOBAL_INC += ['gobject']
GLOBAL_MACROS += [('PYGOBJECT_MAJOR_VERSION', MAJOR_VERSION),
                  ('PYGOBJECT_MINOR_VERSION', MINOR_VERSION),
                  ('PYGOBJECT_MICRO_VERSION', MICRO_VERSION)]

if sys.platform == 'win32':
    GLOBAL_MACROS.append(('VERSION', '"""%s"""' % VERSION))
else:
    raise SystemExit("Error: distutils build only supported on windows")

if sys.version_info[:3] < MIN_PYTHON_VERSION:
    raise SystemExit("Python %s or higher is required, %s found" % (
        ".".join(map(str,MIN_PYTHON_VERSION)),
                     ".".join(map(str,sys.version_info[:3]))))

if not have_pkgconfig():
    raise SystemExit("Error, could not find pkg-config")

DEFS_DIR    = os.path.join('share', 'pygobject', PYGOBJECT_SUFFIX, 'defs')
INCLUDE_DIR = os.path.join('include', 'pygtk-%s' % PYGOBJECT_SUFFIX)

class PyGObjectInstallLib(InstallLib):
    def run(self):

        # Install pygtk.pth, pygtk.py[c] and templates
        self.install_pth()
        self.install_pygtk()

        # Modify the base installation dir
        install_dir = os.path.join(self.install_dir, PYGOBJECT_SUFFIX_LONG)
        self.set_install_dir(install_dir)

        InstallLib.run(self)

    def install_pth(self):
        """Write the pygtk.pth file"""
        file = os.path.join(self.install_dir, 'pygtk.pth')
        self.mkpath(self.install_dir)
        open(file, 'w').write(PYGOBJECT_SUFFIX_LONG)
        self.local_outputs.append(file)
        self.local_inputs.append('pygtk.pth')

    def install_pygtk(self):
        """install pygtk.py in the right place."""
        self.copy_file('pygtk.py', self.install_dir)
        pygtk = os.path.join(self.install_dir, 'pygtk.py')
        self.byte_compile([pygtk])
        self.local_outputs.append(pygtk)
        self.local_inputs.append('pygtk.py')

class PyGObjectInstallData(InstallData):
    def run(self):
        self.add_template_option('VERSION', VERSION)
        self.add_template_option('FFI_LIBS', '')
        self.add_template_option('LIBFFI_PC', '')
        self.prepare()

        # Install templates
        self.install_templates()

        InstallData.run(self)

    def install_templates(self):
        self.install_template('pygobject-2.0.pc.in',
                              os.path.join(self.install_dir,
                                           'lib', 'pkgconfig'))

class PyGObjectBuild(build):
    enable_threading = 1
PyGObjectBuild.user_options.append(('enable-threading', None,
                                'enable threading support'))

# glib
glib = PkgConfigExtension(name='glib._glib',
                          pkc_name='glib-2.0',
                          pkc_version=GLIB_REQUIRED,
                          pygobject_pkc=None,
                          include_dirs=['glib'],
                          libraries=['pyglib'],
                          sources=['glib/glibmodule.c',
                                   'glib/pygiochannel.c',
                                   'glib/pygmaincontext.c',
                                   'glib/pygmainloop.c',
                                   'glib/pygoptioncontext.c',
                                   'glib/pygoptiongroup.c',
                                   'glib/pygsource.c',
                                   'glib/pygspawn.c',
                                   ])

# GObject
gobject = PkgConfigExtension(name='gobject._gobject',
                             pkc_name='gobject-2.0',
                             pkc_version=GLIB_REQUIRED,
                             pygobject_pkc=None,
                             include_dirs=['glib'],
                             libraries=['pyglib'],
                             sources=['gobject/gobjectmodule.c',
                                      'gobject/pygboxed.c',
                                      'gobject/pygenum.c',
                                      'gobject/pygflags.c',
                                      'gobject/pyginterface.c',
                                      'gobject/pygobject.c',
                                      'gobject/pygparamspec.c',
                                      'gobject/pygpointer.c',
                                      'gobject/pygtype.c',
                                      ])

# gio
gio = TemplateExtension(name='gio',
                        pkc_name='gio-2.0',
                        pkc_version=GLIB_REQUIRED,
                        output='gio._gio',
                        defs=('gio/gio.defs', ['gio/gio-types.defs']),
                        include_dirs=['glib'],
                        libraries=['pyglib'],
                        sources=['gio/giomodule.c',
                                 'gio/gio.c',
                                 'gio/pygio-utils.c'],
                        register=[('gio/gio.defs', ['gio/gio-types.defs'])],
                        override='gio/gio.override')

clibs = []
data_files = []
ext_modules = []

#Install dsextras and codegen so that the pygtk installer
#can find them
py_modules = ['dsextras']
packages = ['codegen']

if glib.can_build():
    #It would have been nice to create another class, such as PkgConfigCLib to
    #encapsulate this dictionary, but it is impossible. build_clib.py does
    #a dumb check to see if its only arguments are a 2-tuple containing a
    #string and a Dictionary type - which makes it impossible to hide behind a
    #subclass
    #
    #So we are stuck with this ugly thing
    clibs.append((
        'pyglib',{
            'sources':['glib/pyglib.c'],
            'macros':GLOBAL_MACROS,
            'include_dirs':
                ['glib', get_python_inc()]+pkgc_get_include_dirs('glib-2.0')}))
    #this library is not installed, so probbably should not include its header
    #data_files.append((INCLUDE_DIR, ('glib/pyglib.h',)))
        
    ext_modules.append(glib)
    py_modules += ['glib.__init__', 'glib.option']
else:
    raise SystemExit("ERROR: Nothing to do, glib could not be found and is essential.")

if gobject.can_build():
    ext_modules.append(gobject)
    data_files.append((INCLUDE_DIR, ('gobject/pygobject.h',)))
    py_modules += ['gobject.__init__', 'gobject.propertyhelper', 'gobject.constants']
else:
    raise SystemExit("ERROR: Nothing to do, gobject could not be found and is essential.")

if gio.can_build():
    ext_modules.append(gio)
    py_modules += ['gio.__init__']
    data_files.append((DEFS_DIR,('gio/gio-types.defs',)))
else:
    raise SystemExit("ERROR: Nothing to do, gio could not be found and is essential.")

# Threading support
if '--disable-threading' in sys.argv:
    sys.argv.remove('--disable-threading')
    enable_threading = False
else:
    if '--enable-threading' in sys.argv:
        sys.argv.remove('--enable-threading')
    try:
        import thread
    except ImportError:
        print "Warning: Could not import thread module, disabling threading"
        enable_threading = False
    else:
        enable_threading = True

if enable_threading:
    name = 'gthread-2.0'
    for module in ext_modules:
        raw = getoutput('pkg-config --libs-only-l %s' % name)
        for arg in raw.split():
            if arg.startswith('-l'):
                module.libraries.append(arg[2:])
            else:
                module.extra_link_args.append(arg)
        raw = getoutput('pkg-config --cflags-only-I %s' % name)
        for arg in raw.split():
            if arg.startswith('-I'):
                module.include_dirs.append(arg[2:])
            else:
                module.extra_compile_args.append(arg)
else:
    GLOBAL_MACROS.append(('DISABLE_THREADING', 1))


doclines = __doc__.split("\n")

options = {"bdist_wininst": {"install_script": "pygobject_postinstall.py"}}

setup(name="pygobject",
      url='http://www.pygtk.org/',
      version=VERSION,
      license='LGPL',
      platforms=['yes'],
      maintainer="Johan Dahlin",
      maintainer_email="johan@gnome.org",
      description = doclines[0],
      long_description = "\n".join(doclines[2:]),
      py_modules=py_modules,
      packages=packages,
      ext_modules=ext_modules,
      libraries=clibs,
      data_files=data_files,
      scripts = ["pygobject_postinstall.py"],
      options=options,
      cmdclass={'install_lib': PyGObjectInstallLib,
                'install_data': PyGObjectInstallData,
                'build_clib' : build_clib,
                'build_ext': BuildExt,
                'build': PyGObjectBuild})
