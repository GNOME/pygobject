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
from email import parser

from setuptools import setup, find_packages
from distutils.core import Extension, Distribution
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

    # Add $prefix/share/pkgconfig to PKG_CONFIG_PATH so we use the right
    # pycairo in case we are in a virtualenv
    env = dict(os.environ)
    paths = env.get("PKG_CONFIG_PATH", "").split(os.pathsep)
    paths = [p for p in paths if p]
    paths.insert(0, os.path.join(sys.prefix, "share", "pkgconfig"))
    env["PKG_CONFIG_PATH"] = os.pathsep.join(paths)

    try:
        return subprocess.check_output(command, env=env)
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
    output = ret.decode()
    opt = opt[-2:]
    return [x.lstrip(opt) for x in output.split()]


du_sdist = get_command_class("sdist")


class distcheck(du_sdist):
    """Creates a tarball and does some additional sanity checks such as
    checking if the tarballs includes all files and builds.
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
        finally:
            os.chdir(old_pwd)

    def run(self):
        du_sdist.run(self)
        self._check_manifest()
        self._check_dist()


du_build_ext = get_command_class("build_ext")


class build_ext(du_build_ext):

    def initialize_options(self):
        du_build_ext.initialize_options(self)
        self.compiler_type = None

    def finalize_options(self):
        du_build_ext.finalize_options(self)
        self.compiler_type = new_compiler(compiler=self.compiler).compiler_type

    def _write_config_h(self):
        script_dir = os.path.dirname(os.path.realpath(__file__))
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
        script_dir = os.path.dirname(os.path.realpath(__file__))

        msvc_libraries = {
            "glib-2.0": ["glib-2.0"],
            "gio-2.0": ["gio-2.0", "gobject-2.0", "glib-2.0"],
            "gobject-introspection-1.0":
                ["girepository-1.0", "gobject-2.0", "glib-2.0"],
            get_pycairo_pkg_config_name(): ["cairo"],
            "cairo": ["cairo"],
            "cairo-gobject":
                ["cairo-gobject", "cairo", "gobject-2.0", "glib-2.0"],
            "libffi": ["ffi"],
        }

        def add_dependency(ext, name):
            fallback_libs = msvc_libraries[name]

            if self.compiler_type == "msvc":
                # assume that INCLUDE and LIB contains the right paths
                ext.libraries += fallback_libs

                # The PyCairo header is installed in a subdir of the
                # Python installation that we are building for, so
                # deduce that include path here, and use it
                ext.include_dirs += [
                    os.path.join(sys.prefix, "include", "pycairo")]
            else:
                min_version = get_version_requirement(script_dir, name)
                pkg_config_version_check(name, min_version)
                ext.include_dirs += pkg_config_parse("--cflags-only-I", name)
                ext.library_dirs += pkg_config_parse("--libs-only-L", name)
                ext.libraries += pkg_config_parse("--libs-only-l", name)

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
        add_dependency(gi_cairo_ext, get_pycairo_pkg_config_name())

    def run(self):
        self._write_config_h()
        self._setup_extensions()
        du_build_ext.run(self)


def main():
    script_dir = os.path.dirname(os.path.realpath(__file__))
    pkginfo = parse_pkg_info(script_dir)
    gi_dir = os.path.join(script_dir, "gi")

    sources = [
        os.path.join("gi", n) for n in os.listdir(gi_dir)
        if os.path.splitext(n)[-1] == ".c"
    ]
    cairo_sources = [os.path.join("gi", "pygi-foreign-cairo.c")]
    for s in cairo_sources:
        sources.remove(s)

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
        long_description=pkginfo["Description"],
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
        },
        install_requires=[
            "pycairo>=%s" % get_version_requirement(
                script_dir, get_pycairo_pkg_config_name()),
        ],
    )


if __name__ == "__main__":
    main()
