#!/usr/bin/env python
# Copyright 2017 Christoph Reiter <reiter.christoph@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

"""
ATTENTION DISTRO PACKAGERS: This is not a valid replacement for autotools.
It does not install headers, pkgconfig files and does not support running
tests. Its main use case atm is installation in virtualenvs and via pip.
"""

import io
import os
import re
import sys
import errno
import subprocess
import tarfile
import sysconfig
from email import parser

import pkg_resources
from setuptools import setup, find_packages
from distutils.core import Extension, Distribution, Command
from distutils.ccompiler import new_compiler
from distutils import dir_util


def get_command_class(name):
    # Returns the right class for either distutils or setuptools
    return Distribution({}).get_command_class(name)


def get_pycairo_pkg_config_name():
    return "py3cairo" if sys.version_info[0] == 3 else "pycairo"


def get_version_requirement(conf_dir, pkg_config_name):
    """Given a pkg-config module name gets the minimum version required"""

    if pkg_config_name in ["cairo", "cairo-gobject"]:
        return "0"

    mapping = {
        "gobject-introspection-1.0": "introspection",
        "glib-2.0": "glib",
        "gio-2.0": "gio",
        get_pycairo_pkg_config_name(): "pycairo",
        "libffi": "libffi",
    }
    assert pkg_config_name in mapping

    configure_ac = os.path.join(conf_dir, "configure.ac")
    with io.open(configure_ac, "r", encoding="utf-8") as h:
        text = h.read()
        conf_name = mapping[pkg_config_name]
        res = re.findall(
            r"%s_required_version,\s*([\d\.]+)\)" % conf_name, text)
        assert len(res) == 1
        return res[0]


def parse_versions(conf_dir):
    configure_ac = os.path.join(conf_dir, "configure.ac")
    with io.open(configure_ac, "r", encoding="utf-8") as h:
        version = re.findall(r"pygobject_[^\s]+_version,\s*(\d+)\)", h.read())
        assert len(version) == 3

    versions = {
        "PYGOBJECT_MAJOR_VERSION": version[0],
        "PYGOBJECT_MINOR_VERSION": version[1],
        "PYGOBJECT_MICRO_VERSION": version[2],
        "VERSION": ".".join(version),
    }
    return versions


def parse_pkg_info(conf_dir):
    """Returns an email.message.Message instance containing the content
    of the PKG-INFO file. The version info is parsed from configure.ac
    """

    versions = parse_versions(conf_dir)

    pkg_info = os.path.join(conf_dir, "PKG-INFO.in")
    with io.open(pkg_info, "r", encoding="utf-8") as h:
        text = h.read()
        for key, value in versions.items():
            text = text.replace("@%s@" % key, value)

    p = parser.Parser()
    message = p.parse(io.StringIO(text))
    return message


def _run_pkg_config(args):
    command = ["pkg-config"] + args

    try:
        return subprocess.check_output(command)
    except OSError as e:
        if e.errno == errno.ENOENT:
            raise SystemExit(
                "%r not found.\nArguments: %r" % (command[0], command))
        raise SystemExit(e)
    except subprocess.CalledProcessError as e:
        raise SystemExit(e)


def pkg_config_version_check(pkg, version):
    _run_pkg_config([
        "--print-errors",
        "--exists",
        '%s >= %s' % (pkg, version),
    ])


def pkg_config_parse(opt, pkg):
    ret = _run_pkg_config([opt, pkg])
    if sys.version_info[0] == 3:
        output = ret.decode()
    else:
        output = ret
    opt = opt[-2:]
    return [x.lstrip(opt) for x in output.split()]


def samefile(src, dst):
    # Python 2 on Windows doesn't have os.path.samefile, so we have to provide
    # a fallback

    if hasattr(os.path, "samefile"):
        return os.path.samefile(src, dst)

    os.stat(src)
    os.stat(dst)
    return (os.path.normcase(os.path.abspath(src)) ==
            os.path.normcase(os.path.abspath(dst)))


du_sdist = get_command_class("sdist")


class distcheck(du_sdist):
    """Creates a tarball and does some additional sanity checks such as
    checking if the tarball includes all files, builds successfully and
    the tests suite passes.
    """

    def _check_manifest(self):
        # make sure MANIFEST.in includes all tracked files
        assert self.get_archive_files()

        if subprocess.call(["git", "status"],
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE) != 0:
            return

        included_files = self.filelist.files
        assert included_files

        process = subprocess.Popen(
            ["git", "ls-tree", "-r", "HEAD", "--name-only"],
            stdout=subprocess.PIPE, universal_newlines=True)
        out, err = process.communicate()
        assert process.returncode == 0

        tracked_files = out.splitlines()
        for ignore in [".gitignore"]:
            tracked_files.remove(ignore)

        diff = set(tracked_files) - set(included_files)
        assert not diff, (
            "Not all tracked files included in tarball, check MANIFEST.in",
            diff)

    def _check_dist(self):
        # make sure the tarball builds
        assert self.get_archive_files()

        distcheck_dir = os.path.abspath(
            os.path.join(self.dist_dir, "distcheck"))
        if os.path.exists(distcheck_dir):
            dir_util.remove_tree(distcheck_dir)
        self.mkpath(distcheck_dir)

        archive = self.get_archive_files()[0]
        tfile = tarfile.open(archive, "r:gz")
        tfile.extractall(distcheck_dir)
        tfile.close()

        name = self.distribution.get_fullname()
        extract_dir = os.path.join(distcheck_dir, name)

        old_pwd = os.getcwd()
        os.chdir(extract_dir)
        try:
            self.spawn([sys.executable, "setup.py", "build"])
            self.spawn([sys.executable, "setup.py", "install",
                        "--root",
                        os.path.join(distcheck_dir, "prefix"),
                        "--record",
                        os.path.join(distcheck_dir, "log.txt"),
                        ])
            self.spawn([sys.executable, "setup.py", "test"])
        finally:
            os.chdir(old_pwd)

    def run(self):
        du_sdist.run(self)
        self._check_manifest()
        self._check_dist()


class build_tests(Command):
    description = "build test libraries and extensions"
    user_options = [
        ("force", "f", "force a rebuild"),
    ]

    def initialize_options(self):
        self.build_temp = None
        self.force = False

    def finalize_options(self):
        self.set_undefined_options(
            'build_ext',
            ('build_temp', 'build_temp'))

    def _newer_group(self, sources, target):
        from distutils.dep_util import newer_group

        if self.force:
            return True
        else:
            return newer_group(sources, target)

    def run(self):
        from distutils.ccompiler import new_compiler
        from distutils.sysconfig import customize_compiler

        gidatadir = pkg_config_parse(
            "--variable=gidatadir", "gobject-introspection-1.0")[0]
        g_ir_scanner = pkg_config_parse(
            "--variable=g_ir_scanner", "gobject-introspection-1.0")[0]
        g_ir_compiler = pkg_config_parse(
            "--variable=g_ir_compiler", "gobject-introspection-1.0")[0]

        script_dir = get_script_dir()
        tests_dir = os.path.join(script_dir, "tests")
        gi_tests_dir = os.path.join(gidatadir, "tests")

        schema_xml = os.path.join(tests_dir, "org.gnome.test.gschema.xml")
        schema_bin = os.path.join(tests_dir, "gschemas.compiled")
        if self._newer_group([schema_xml], schema_bin):
            subprocess.check_call([
                "glib-compile-schemas",
                "--targetdir=%s" % tests_dir,
                "--schema-file=%s" % schema_xml,
            ])

        compiler = new_compiler()
        customize_compiler(compiler)

        def build_ext(ext):
            ext_path = os.path.join(
                tests_dir,
                compiler.shared_object_filename(ext.name))

            if self._newer_group(ext.sources + ext.depends, ext_path):
                objects = compiler.compile(
                    ext.sources,
                    output_dir=self.build_temp,
                    include_dirs=ext.include_dirs)

                compiler.link_shared_object(
                    objects,
                    ext_path,
                    output_dir=script_dir,
                    libraries=ext.libraries,
                    library_dirs=ext.library_dirs)

        ext = Extension(
            name='libgimarshallingtests',
            sources=[
                os.path.join(gi_tests_dir, "gimarshallingtests.c"),
                os.path.join(tests_dir, "gimarshallingtestsextra.c"),
            ],
            include_dirs=[
                gi_tests_dir,
                tests_dir,
            ],
            depends=[
                os.path.join(gi_tests_dir, "gimarshallingtests.h"),
                os.path.join(tests_dir, "gimarshallingtestsextra.h"),
            ],
        )
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "glib-2.0")
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "gio-2.0")
        build_ext(ext)

        gir_path = os.path.join(tests_dir, "GIMarshallingTests-1.0.gir")
        typelib_path = os.path.join(
            tests_dir, "GIMarshallingTests-1.0.typelib")

        if self._newer_group(ext.sources + ext.depends, gir_path):
            subprocess.check_call([
                g_ir_scanner,
                "--include=Gio-2.0",
                "--namespace=GIMarshallingTests",
                "--nsversion=1.0",
                "--symbol-prefix=gi_marshalling_tests",
                "--warn-all",
                "--warn-error",
                "--library-path=%s" % tests_dir,
                "--library=gimarshallingtests",
                "--pkg=glib-2.0",
                "--pkg=gio-2.0",
                "--output=%s" % gir_path,
            ] + ext.sources + ext.depends)

        if self._newer_group([gir_path], typelib_path):
            subprocess.check_call([
                g_ir_compiler,
                gir_path,
                "--output=%s" % typelib_path,
            ])

        ext = Extension(
            name='libregress',
            sources=[
                os.path.join(gi_tests_dir, "regress.c"),
            ],
            include_dirs=[
                gi_tests_dir,
            ],
            depends=[
                os.path.join(gi_tests_dir, "regress.h"),
            ],
        )
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "glib-2.0")
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "gio-2.0")
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "cairo")
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "cairo-gobject")
        build_ext(ext)

        gir_path = os.path.join(tests_dir, "Regress-1.0.gir")
        typelib_path = os.path.join(tests_dir, "Regress-1.0.typelib")

        if self._newer_group(ext.sources + ext.depends, gir_path):
            subprocess.check_call([
                g_ir_scanner,
                "--include=cairo-1.0",
                "--include=Gio-2.0",
                "--namespace=Regress",
                "--nsversion=1.0",
                "--warn-all",
                "--warn-error",
                "--library-path=%s" % tests_dir,
                "--library=regress",
                "--pkg=glib-2.0",
                "--pkg=gio-2.0",
                "--pkg=cairo",
                "--pkg=cairo-gobject",
                "--output=%s" % gir_path,
            ] + ext.sources + ext.depends)

        if self._newer_group([gir_path], typelib_path):
            subprocess.check_call([
                g_ir_compiler,
                gir_path,
                "--output=%s" % typelib_path,
            ])

        ext = Extension(
            name='tests.testhelper',
            sources=[
                os.path.join(tests_dir, "testhelpermodule.c"),
                os.path.join(tests_dir, "test-floating.c"),
                os.path.join(tests_dir, "test-thread.c"),
                os.path.join(tests_dir, "test-unknown.c"),
            ],
            include_dirs=[
                os.path.join(script_dir, "gi"),
                tests_dir,
            ],
            depends=[
                os.path.join(tests_dir, "test-thread.h"),
                os.path.join(tests_dir, "test-unknown.h"),
                os.path.join(tests_dir, "test-floating.h"),
            ],
        )
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "glib-2.0")
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "gio-2.0")
        add_ext_pkg_config_dep(ext, compiler.compiler_type, "cairo")

        dist = Distribution({"ext_modules": [ext]})
        cmd = dist.get_command_obj("build_ext")
        cmd.inplace = True
        cmd.ensure_finalized()
        cmd.run()


class test(Command):
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        cmd = self.reinitialize_command("build_ext")
        cmd.inplace = True
        cmd.ensure_finalized()
        cmd.run()

        cmd = self.reinitialize_command("build_tests")
        cmd.ensure_finalized()
        cmd.run()

        tests_dir = os.path.join(get_script_dir(), "tests")
        subprocess.check_call([
            sys.executable,
            os.path.join(tests_dir, "runtests.py"),
        ])


def get_script_dir():
    return os.path.dirname(os.path.realpath(__file__))


def get_pycairo_include_dir():
    """Returns the best guess at where to find the pycairo headers.

    Raises if pycairo isn't found.
    """

    script_dir = get_script_dir()
    min_version = get_version_requirement(
        script_dir, get_pycairo_pkg_config_name())

    dist = pkg_resources.get_distribution("pycairo>=%s" % min_version)

    def get_sys_path(dist, name):
        """The sysconfig path for a distribution, or None"""

        location = dist.location
        for scheme in sysconfig.get_scheme_names():
            for path_type in ["platlib", "purelib"]:
                path = sysconfig.get_path(path_type, scheme)
                try:
                    if samefile(path, location):
                        return sysconfig.get_path(name, scheme)
                except EnvironmentError:
                    pass

    data_path = get_sys_path(dist, "data") or sys.prefix
    return os.path.join(data_path, "include", "pycairo")


def add_ext_pkg_config_dep(ext, compiler_type, name):
    script_dir = get_script_dir()

    msvc_libraries = {
        "glib-2.0": ["glib-2.0"],
        "gio-2.0": ["gio-2.0", "gobject-2.0", "glib-2.0"],
        "gobject-introspection-1.0":
            ["girepository-1.0", "gobject-2.0", "glib-2.0"],
        "cairo": ["cairo"],
        "cairo-gobject":
            ["cairo-gobject", "cairo", "gobject-2.0", "glib-2.0"],
        "libffi": ["ffi"],
    }

    fallback_libs = msvc_libraries[name]
    if compiler_type == "msvc":
        # assume that INCLUDE and LIB contains the right paths
        ext.libraries += fallback_libs
    else:
        min_version = get_version_requirement(script_dir, name)
        pkg_config_version_check(name, min_version)
        ext.include_dirs += pkg_config_parse("--cflags-only-I", name)
        ext.library_dirs += pkg_config_parse("--libs-only-L", name)
        ext.libraries += pkg_config_parse("--libs-only-l", name)


du_build_ext = get_command_class("build_ext")


class build_ext(du_build_ext):

    def initialize_options(self):
        du_build_ext.initialize_options(self)
        self.compiler_type = None

    def finalize_options(self):
        du_build_ext.finalize_options(self)
        self.compiler_type = new_compiler(compiler=self.compiler).compiler_type

    def _write_config_h(self):
        script_dir = get_script_dir()
        target = os.path.join(script_dir, "config.h")
        versions = parse_versions(script_dir)
        with io.open(target, 'w', encoding="utf-8") as h:
            h.write("""
/* Configuration header created by setup.py - do not edit */
#ifndef _CONFIG_H
#define _CONFIG_H 1

#define PYGOBJECT_MAJOR_VERSION %(PYGOBJECT_MAJOR_VERSION)s
#define PYGOBJECT_MINOR_VERSION %(PYGOBJECT_MINOR_VERSION)s
#define PYGOBJECT_MICRO_VERSION %(PYGOBJECT_MICRO_VERSION)s
#define VERSION "%(VERSION)s"

#endif /* _CONFIG_H */
""" % versions)

    def _setup_extensions(self):
        ext = {e.name: e for e in self.extensions}

        def add_dependency(ext, name):
            add_ext_pkg_config_dep(ext, self.compiler_type, name)

        def add_pycairo(ext):
            include_dir = get_pycairo_include_dir()
            ext.include_dirs += [include_dir]

        gi_ext = ext["gi._gi"]
        add_dependency(gi_ext, "glib-2.0")
        add_dependency(gi_ext, "gio-2.0")
        add_dependency(gi_ext, "gobject-introspection-1.0")
        add_dependency(gi_ext, "libffi")

        gi_cairo_ext = ext["gi._gi_cairo"]
        add_dependency(gi_cairo_ext, "glib-2.0")
        add_dependency(gi_cairo_ext, "gio-2.0")
        add_dependency(gi_cairo_ext, "gobject-introspection-1.0")
        add_dependency(gi_cairo_ext, "libffi")
        add_dependency(gi_cairo_ext, "cairo")
        add_dependency(gi_cairo_ext, "cairo-gobject")
        add_pycairo(gi_cairo_ext)

    def run(self):
        self._write_config_h()
        self._setup_extensions()
        du_build_ext.run(self)


def main():
    script_dir = get_script_dir()
    pkginfo = parse_pkg_info(script_dir)
    gi_dir = os.path.join(script_dir, "gi")

    sources = [
        os.path.join("gi", n) for n in os.listdir(gi_dir)
        if os.path.splitext(n)[-1] == ".c"
    ]
    cairo_sources = [os.path.join("gi", "pygi-foreign-cairo.c")]
    for s in cairo_sources:
        sources.remove(s)

    readme = os.path.join(script_dir, "README.rst")
    with io.open(readme, encoding="utf-8") as h:
        long_description = h.read()

    gi_ext = Extension(
        name='gi._gi',
        sources=sources,
        include_dirs=[script_dir, gi_dir],
        define_macros=[("HAVE_CONFIG_H", None)],
    )

    gi_cairo_ext = Extension(
        name='gi._gi_cairo',
        sources=cairo_sources,
        include_dirs=[script_dir, gi_dir],
        define_macros=[("HAVE_CONFIG_H", None)],
    )

    setup(
        name=pkginfo["Name"],
        version=pkginfo["Version"],
        description=pkginfo["Summary"],
        url=pkginfo["Home-page"],
        author=pkginfo["Author"],
        author_email=pkginfo["Author-email"],
        maintainer=pkginfo["Maintainer"],
        maintainer_email=pkginfo["Maintainer-email"],
        license=pkginfo["License"],
        long_description=long_description,
        platforms=pkginfo.get_all("Platform"),
        classifiers=pkginfo.get_all("Classifier"),
        packages=find_packages(script_dir),
        ext_modules=[
            gi_ext,
            gi_cairo_ext,
        ],
        cmdclass={
            "build_ext": build_ext,
            "distcheck": distcheck,
            "build_tests": build_tests,
            "test": test,
        },
        install_requires=[
            "pycairo>=%s" % get_version_requirement(
                script_dir, get_pycairo_pkg_config_name()),
        ],
    )


if __name__ == "__main__":
    main()
