#
# dsextras.py - Extra classes and utilities
#
# TODO:
# Make it possible to import codegen from another dir
#
from commands import getoutput, getstatusoutput
from distutils.command.build_ext import build_ext
from distutils.command.install_lib import install_lib
from distutils.extension import Extension
import fnmatch
import os
import string
import sys

GLOBAL_INC = []
GLOBAL_MACROS = []

class InstallLib(install_lib):
    local_outputs = []
    local_inputs = []
    template_options = {}
    def prepare(self):
        self.prefix = os.sep.join(self.install_dir.split(os.sep)[:-4])
        self.exec_prefix = os.path.join(self.prefix, 'bin')
        self.includedir = os.path.join(self.prefix, 'include')
        self.libdir = os.path.join(self.prefix, 'lib')
        self.datadir = os.path.join(self.prefix, 'share')
        
        self.add_template_option('prefix', self.prefix)        
        self.add_template_option('exec_prefix', self.exec_prefix)        
        self.add_template_option('includedir', self.includedir)
        self.add_template_option('libdir', self.libdir)
        self.add_template_option('datadir', self.datadir)
        self.add_template_option('PYTHON', sys.executable)
        
    def set_install_dir(self, install_dir):
        self.install_dir = install_dir
        
    def add_template_option(self, name, value):
        self.template_options['@%s@'] = value
        
    def install_template(self, filename, install_dir):
        """Install template filename into target directory install_dir."""
        output_file = os.path.split(filename)[-1][:-3]

        template = open(filename).read()
        for key, value in self.template_options.items():
            template = template.replace(key, value)

        output = os.path.join(install_dir, output_file)
        self.mkpath(install_dir)
        open(output, 'w').write(template)
        self.local_inputs.append(filename)
        self.local_outputs.append(output)
        return output
    
    def get_outputs(self):
        return install_lib.get_outputs(self) + self.local_outputs

    def get_inputs(self):
        return install_lib.get_inputs(self) + self.local_inputs
    
class PkgConfigExtension(Extension):
    can_build_ok = None
    def __init__(self, **kwargs):
        name = kwargs['pkc_name']
        kwargs['include_dirs'] = self.get_include_dirs(name) + GLOBAL_INC
        kwargs['define_macros'] = GLOBAL_MACROS
        kwargs['libraries'] = self.get_libraries(name)
        kwargs['library_dirs'] = self.get_library_dirs(name) 
        self.pkc_name = kwargs['pkc_name']
        self.pkc_version = kwargs['pkc_version']
        del kwargs['pkc_name'], kwargs['pkc_version']
        Extension.__init__(self, **kwargs)

    def get_include_dirs(self, name):
        output = getoutput('pkg-config --cflags-only-I %s' % name)
        return output.replace('-I', '').split()

    def get_libraries(self, name):
        output = getoutput('pkg-config --libs-only-l %s' % name)
        return output.replace('-l', '').split()
    
    def get_library_dirs(self, name):
        output = getoutput('pkg-config --libs-only-L %s' % name)
        return output.replace('-L', '').split()

    def can_build(self):
        """If the pkg-config version found is good enough"""
        if self.can_build_ok != None: 
            return self.can_build_ok

        retval = os.system('pkg-config --exists %s' % self.pkc_name)
        if retval:
            print "* Could not find %s." % self.pkc_name
            self.can_build_ok = 0
            return 0

        orig_version = getoutput('pkg-config --modversion %s' % self.pkc_name)
        version = map(int, orig_version.split('.'))
        pkc_version = map(int, self.pkc_version.split('.'))
                      
        if version >= pkc_version:
            self.can_build_ok = 1
            return 1
        else:
            print "Warning: Too old version of %s" % self.pkc_name
            print "         Need %s, but %s is installed" % \
                  (self.pkc_version, orig_version)
            self.can_build_ok = 0
            return 0
        
    def generate(self):
        pass
       
class Template:
    def __init__(self, override, output, defs, prefix, register=[]):
        self.override = override
        self.defs = defs
        self.register = register
        self.output = output
        self.prefix = prefix

    def check_dates(self):
        if not os.path.exists(self.output):
            return 0

        files = self.register[:]
        files.append(self.override)
#        files.append('setup.py')
        files.append(self.defs)
        
        newest = 0
        for file in files:
            test = os.stat(file)[8]
            if test > newest:
                newest = test
                
        if newest < os.stat(self.output)[8]:
            return 1
        return 0
    
    def generate(self):
        # We must insert it first, otherwise python will try '.' first,
        # in which it exist a "bogus" codegen (the third import line
        # here will fail)
        sys.path.insert(0, 'codegen')
        from override import Overrides
        from defsparser import DefsParser
        from codegen import register_types, write_source, FileOutput
        
        if self.check_dates():
            return

        for item in self.register:
            dp = DefsParser(item)
            dp.startParsing()
            register_types(dp)

        dp = DefsParser(self.defs)
        dp.startParsing()
        register_types(dp)
        
        fd = open(self.output, 'w')
        write_source(dp,
                     Overrides(self.override),
                     self.prefix,
                     FileOutput(fd, self.output))
        fd.close()
        
class TemplateExtension(PkgConfigExtension):
    def __init__(self, **kwargs):
        name = kwargs['name']
        defs = kwargs['defs']
        output = defs[:-5] + '.c'
        override = kwargs['override']
        self.templates = []
        self.templates.append(Template(override, output, defs, 'py' + name,
                                       kwargs['register']))
        
        del kwargs['register'], kwargs['override'], kwargs['defs']

        if kwargs.has_key('output'):
            kwargs['name'] = kwargs['output']
            del kwargs['output']
        
        PkgConfigExtension.__init__(self, **kwargs)
        
    def generate(self):
        map(lambda x: x.generate(), self.templates)
        
class BuildExt(build_ext):
    def build_extension(self, ext):
        # Generate eventual templates before building
        if hasattr(ext, 'generate'):
            ext.generate()
        build_ext.build_extension(self, ext)
        
def list_files(dir):
    """List all files in a dir, with filename match support:
    for example: glade/*.glade will return all files in the glade directory
    that matches *.glade. It also looks up the full path"""
    if dir.find(os.sep) != -1:
        parts = dir.split(os.sep)
        dir = string.join(parts[:-1], os.sep)
        pattern = parts[-1]
    else:
        pattern = dir
        dir = '.'

    dir = os.path.abspath(dir)
    retval = []
    for file in os.listdir(dir):
        if fnmatch.fnmatch(file, pattern):
            retval.append(os.path.join(dir, file))
    return retval

def have_pkgconfig():
    """Checks for the existence of pkg-config"""
    status = getstatusoutput('pkg-config')[0]
    if status == 256:
        return 1
