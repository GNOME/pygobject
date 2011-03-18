#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# setup.py - distutils configuration for pygobject


'''Python Bindings for GObject.

PyGObject is a set of bindings for the glib, gobject and gio libraries.
It provides an object oriented interface that is slightly higher level than
the C one. It automatically does all the type casting and reference
counting that you would have to do normally with the C API. You can
find out more on the official homepage, http://www.pygtk.org/'''


import os
import sys
import glob

from distutils.command.build import build
from distutils.command.build_clib import build_clib
from distutils.command.build_scripts import build_scripts
from distutils.sysconfig import get_python_inc
from distutils.extension import Extension
from distutils.core import setup

from dsextras import GLOBAL_MACROS, GLOBAL_INC, get_m4_define, getoutput, \
                     have_pkgconfig, pkgc_get_libraries, \
                     pkgc_get_library_dirs, pkgc_get_include_dirs, \
                     PkgConfigExtension, TemplateExtension, \
                     BuildExt, InstallLib, InstallData


if sys.platform != 'win32':
    msg =  '*' * 68 + '\n'
    msg += '* Building PyGObject using distutils is only supported on windows. *\n'
    msg += '* To build PyGObject in a supported way, read the INSTALL file.    *\n'
    msg += '*' * 68
    raise SystemExit(msg)

MIN_PYTHON_VERSION = (2, 6, 0)

if sys.version_info[:3] < MIN_PYTHON_VERSION:
    raise SystemExit('ERROR: Python %s or higher is required, %s found.' % (
                         '.'.join(map(str, MIN_PYTHON_VERSION)),
                         '.'.join(map(str, sys.version_info[:3]))))

if not have_pkgconfig():
    raise SystemExit('ERROR: Could not find pkg-config: '
                     'Please check your PATH environment variable.')


PYGTK_SUFFIX = '2.0'
PYGTK_SUFFIX_LONG = 'gtk-' + PYGTK_SUFFIX

GLIB_REQUIRED = get_m4_define('glib_required_version')

MAJOR_VERSION = int(get_m4_define('pygobject_major_version'))
MINOR_VERSION = int(get_m4_define('pygobject_minor_version'))
MICRO_VERSION = int(get_m4_define('pygobject_micro_version'))
VERSION       = '%d.%d.%d' % (MAJOR_VERSION, MINOR_VERSION, MICRO_VERSION)

GLOBAL_INC += ['gobject']
GLOBAL_MACROS += [('PYGOBJECT_MAJOR_VERSION', MAJOR_VERSION),
                  ('PYGOBJECT_MINOR_VERSION', MINOR_VERSION),
                  ('PYGOBJECT_MICRO_VERSION', MICRO_VERSION),
                  ('VERSION', '\\"%s\\"' % VERSION)]

BIN_DIR     = os.path.join('Scripts')
INCLUDE_DIR = os.path.join('include', 'pygtk-%s' % PYGTK_SUFFIX)
DEFS_DIR    = os.path.join('share', 'pygobject', PYGTK_SUFFIX, 'defs')
XSL_DIR     = os.path.join('share', 'pygobject','xsl')
HTML_DIR    = os.path.join('share', 'gtk-doc', 'html', 'pygobject')


class PyGObjectInstallLib(InstallLib):
    def run(self):
        # Install pygtk.pth, pygtk.py[c] and templates
        self.install_pth()
        self.install_pygtk()

        # Modify the base installation dir
        install_dir = os.path.join(self.install_dir, PYGTK_SUFFIX_LONG)
        self.set_install_dir(install_dir)

        # Install tests
        self.install_tests()

        InstallLib.run(self)

    def install_pth(self):
        '''Create the pygtk.pth file'''
        file = os.path.join(self.install_dir, 'pygtk.pth')
        self.mkpath(self.install_dir)
        open(file, 'w').write(PYGTK_SUFFIX_LONG)
        self.local_outputs.append(file)
        self.local_inputs.append('pygtk.pth')

    def install_pygtk(self):
        '''Install pygtk.py in the right place.'''
        self.copy_file('pygtk.py', self.install_dir)
        pygtk = os.path.join(self.install_dir, 'pygtk.py')
        self.byte_compile([pygtk])
        self.local_outputs.append(pygtk)
        self.local_inputs.append('pygtk.py')

    def copy_test(self, srcfile, dstfile=None):
        if dstfile is None:
            dstfile = os.path.join(self.test_dir, srcfile)
        else:
            dstfile = os.path.join(self.test_dir, dstfile)

        srcfile = os.path.join('tests', srcfile)

        self.copy_file(srcfile, os.path.abspath(dstfile))
        self.local_outputs.append(dstfile)
        self.local_inputs.append('srcfile')

    def install_tests(self):
        self.test_dir = os.path.join(self.install_dir, 'tests', 'pygobject')
        self.mkpath(self.test_dir)

        self.copy_test('runtests-windows.py', 'runtests.py')
        self.copy_test('compathelper.py')

        for testfile in glob.glob('tests/test*.py'):
            self.copy_test(os.path.basename(testfile))


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
        self.install_template('pygobject-%s.pc.in' % PYGTK_SUFFIX,
                              os.path.join(self.install_dir, 'lib', 'pkgconfig'))

        self.install_template('docs/xsl/fixxref.py.in',
                              os.path.join(self.install_dir, XSL_DIR))


class PyGObjectBuild(build):
    enable_threading = True

PyGObjectBuild.user_options.append(('enable-threading', None,
                                    'enable threading support'))


class PyGObjectBuildScripts(build_scripts):
    '''
    Overrides distutils' build_script command so we can generate
    a valid pygobject-codegen script that works on windows.
    '''

    def run(self):
        self.mkpath(self.build_dir)
        self.install_codegen_script()
        build_scripts.run(self)

    def install_codegen_script(self):
        '''Create pygobject-codegen'''
        script = ('#!/bin/sh\n\n'
                  'codegendir=`pkg-config pygobject-%s --variable=codegendir`\n\n'
                  'PYTHONPATH=$codegendir\n'
                  'export PYTHONPATH\n\n'
                  'exec pythonw.exe "$codegendir/codegen.py" "$@"\n' % PYGTK_SUFFIX)

        outfile = os.path.join(self.build_dir, 'pygobject-codegen-%s' % PYGTK_SUFFIX)
        open(outfile, 'w').write(script)


# glib
glib = PkgConfigExtension(name='glib._glib',
                          pkc_name='glib-%s' % PYGTK_SUFFIX,
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
                             pkc_name='gobject-%s' % PYGTK_SUFFIX,
                             pkc_version=GLIB_REQUIRED,
                             pygobject_pkc=None,
                             include_dirs=['glib','gi'],
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
                        pkc_name='gio-%s' % PYGTK_SUFFIX,
                        pkc_version=GLIB_REQUIRED,
                        output='gio._gio',
                        defs='gio/gio.defs',
                        include_dirs=['glib'],
                        libraries=['pyglib'],
                        sources=['gio/giomodule.c',
                                 'gio/gio.c',
                                 'gio/pygio-utils.c'],
                        register=['gio/gio-types.defs'],
                        override='gio/gio.override')

clibs = []
data_files = []
ext_modules = []

#Install dsextras and codegen so that the pygtk installer can find them
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
    clibs.append(('pyglib', {'sources': ['glib/pyglib.c'],
                             'macros': GLOBAL_MACROS,
                             'include_dirs': ['glib', get_python_inc()] +
                                              pkgc_get_include_dirs('glib-%s' % PYGTK_SUFFIX)}))
    #this library is not installed, so probably should not include its header
    #data_files.append((INCLUDE_DIR, ('glib/pyglib.h',)))

    ext_modules.append(glib)
    py_modules += ['glib.__init__', 'glib.option']
else:
    raise SystemExit('ERROR: Nothing to do, glib could not be found and is essential.')

if gobject.can_build():
    ext_modules.append(gobject)
    data_files.append((INCLUDE_DIR, ('gobject/pygobject.h',)))
    data_files.append((HTML_DIR, glob.glob('docs/html/*.html')))
    data_files.append((HTML_DIR, ['docs/style.css']))
    data_files.append((XSL_DIR,  glob.glob('docs/xsl/*.xsl')))
    py_modules += ['gobject.__init__', 'gobject.propertyhelper', 'gobject.constants']
else:
    raise SystemExit('ERROR: Nothing to do, gobject could not be found and is essential.')

if gio.can_build():
    ext_modules.append(gio)
    py_modules += ['gio.__init__']
    data_files.append((DEFS_DIR,('gio/gio.defs', 'gio/gio-types.defs',)))
else:
    raise SystemExit, 'ERROR: Nothing to do, gio could not be found and is essential.'

# Build testhelper library
testhelper = Extension(name='testhelper',
                       sources=['tests/testhelpermodule.c',
                                'tests/test-floating.c',
                                'tests/test-thread.c',
                                'tests/test-unknown.c'],
                       libraries=['pyglib'] +
                                 pkgc_get_libraries('glib-%s' % PYGTK_SUFFIX) +
                                 pkgc_get_libraries('gobject-%s' % PYGTK_SUFFIX),
                       include_dirs=['tests', 'glib',
                                     'gobject', get_python_inc()] +
                                    pkgc_get_include_dirs('glib-%s' % PYGTK_SUFFIX) +
                                    pkgc_get_include_dirs('gobject-%s' % PYGTK_SUFFIX),
                       library_dirs=pkgc_get_library_dirs('glib%s' % PYGTK_SUFFIX) +
                                    pkgc_get_library_dirs('gobject-%s' % PYGTK_SUFFIX))

ext_modules.append(testhelper)

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
        print ('* Could not import thread module, disabling threading')
        enable_threading = False
    else:
        enable_threading = True

if enable_threading:
    name = 'gthread-%s' % PYGTK_SUFFIX
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

doclines = __doc__.split('\n')
options = {'bdist_wininst': {'install_script': 'pygobject_postinstall.py',
                             'user_access_control': 'auto'}}

setup(name='pygobject',
      url='http://www.pygtk.org/',
      version=VERSION,
      license='LGPL',
      platforms=['MS Windows'],
      maintainer='Johan Dahlin',
      maintainer_email='johan@gnome.org',
      description=doclines[0],
      long_description='\n'.join(doclines[2:]),
      provides=['codegen', 'dsextras', 'gio', 'glib', 'gobject'],
      py_modules=py_modules,
      packages=packages,
      ext_modules=ext_modules,
      libraries=clibs,
      data_files=data_files,
      scripts=['pygobject_postinstall.py'],
      options=options,
      cmdclass={'install_lib': PyGObjectInstallLib,
                'install_data': PyGObjectInstallData,
                'build_scripts': PyGObjectBuildScripts,
                'build_clib' : build_clib,
                'build_ext': BuildExt,
                'build': PyGObjectBuild})
