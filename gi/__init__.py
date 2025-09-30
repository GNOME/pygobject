# Copyright (C) 2005-2009 Johan Dahlin <johan@gnome.org>
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

# support overrides in different directories than our gi module
from pkgutil import extend_path

__path__ = extend_path(__path__, __name__)

import sys
import os
import importlib
import types

_static_binding_error = (
    "When using gi.repository you must not import static "
    'modules like "gobject". Please change all occurrences '
    'of "import gobject" to "from gi.repository import GObject". '
    "See: https://bugzilla.gnome.org/show_bug.cgi?id=709183"
)

# we can't have pygobject 2 loaded at the same time we load the internal _gobject
if "gobject" in sys.modules:
    raise ImportError(_static_binding_error)


from . import _gi
from ._gi import _API as _API
from ._gi import Repository
from ._gi import PyGIDeprecationWarning as PyGIDeprecationWarning
from ._gi import PyGIWarning as PyGIWarning

_versions = {}
_overridesdir = os.path.join(os.path.dirname(__file__), "overrides")

# Needed for compatibility with "pygobject.h"/pygobject_init()
_gobject = types.ModuleType("gi._gobject")
sys.modules[_gobject.__name__] = _gobject
_gobject._PyGObject_API = _gi._PyGObject_API
_gobject.pygobject_version = _gi.pygobject_version

version_info = _gi.pygobject_version[:]
__version__ = "{}.{}.{}".format(*version_info)

_gi.register_foreign()

# Options which can be enabled or disabled after importing 'gi'. This may affect
# repository imports or binding machinery in a backwards incompatible way.
_options = {
    # When True, importing Gtk or Gdk will call Gtk.init() or Gdk.init() respectively.
    "legacy_autoinit": True,
}


class OverrideImport:
    def __init__(self, overrides_path):
        self.overrides_path = overrides_path

    def find_spec(self, fullname, path, target=None):
        if not fullname.startswith("gi.overrides"):
            return None
        finder = importlib.machinery.PathFinder()
        # From find_spec the docs:
        # If name is for a submodule (contains a dot), the parent module is automatically imported.
        return finder.find_spec(fullname, self.overrides_path)


_pgi_overrides_path = os.environ.get("PYGI_OVERRIDES_PATH", "")
if _pgi_overrides_path:
    sys.meta_path.insert(0, OverrideImport(_pgi_overrides_path.split(os.pathsep)))


class _DummyStaticModule(types.ModuleType):
    __path__ = None

    def __getattr__(self, name):
        raise AttributeError(_static_binding_error)


sys.modules["glib"] = _DummyStaticModule("glib", _static_binding_error)
sys.modules["gobject"] = _DummyStaticModule("gobject", _static_binding_error)
sys.modules["gio"] = _DummyStaticModule("gio", _static_binding_error)
sys.modules["gtk"] = _DummyStaticModule("gtk", _static_binding_error)
sys.modules["gtk.gdk"] = _DummyStaticModule("gtk.gdk", _static_binding_error)


def check_version(version):
    if isinstance(version, str):
        version_list = tuple(map(int, version.split(".")))
    else:
        version_list = version

    if version_list > version_info:
        raise ValueError(
            f"pygobject's version {version} required, and available version "
            f"{__version__} is not recent enough"
        )


def require_version(namespace, version):
    """Ensures the correct versions are loaded when importing `gi` modules.

    :param namespace: The name of module to require.
    :type namespace: str
    :param version: The version of module to require.
    :type version: str
    :raises ValueError: If module/version is already loaded, already required, or unavailable.

    :Example:

    .. code-block:: python

        import gi

        gi.require_version("Gtk", "3.0")

    """
    repository = Repository.get_default()

    if not isinstance(version, str):
        raise ValueError("Namespace version needs to be a string.")

    if namespace in repository.get_loaded_namespaces():
        loaded_version = repository.get_version(namespace)
        if loaded_version != version:
            raise ValueError(
                f"Namespace {namespace} is already loaded with version {loaded_version}"
            )

    if namespace in _versions and _versions[namespace] != version:
        raise ValueError(
            f"Namespace {namespace} already requires version {_versions[namespace]}"
        )

    available_versions = repository.enumerate_versions(namespace)
    if not available_versions:
        raise ValueError(f"Namespace {namespace} not available")

    if version not in available_versions:
        raise ValueError(f"Namespace {namespace} not available for version {version}")

    _versions[namespace] = version


def require_versions(requires):
    """Utility function for consolidating multiple `gi.require_version()` calls.

    :param requires: The names and versions of modules to require.
    :type requires: dict

    :Example:

    .. code-block:: python

        import gi

        gi.require_versions({"Gtk": "3.0", "GLib": "2.0", "Gio": "2.0"})
    """
    for module_name, module_version in requires.items():
        require_version(module_name, module_version)


def get_required_version(namespace):
    return _versions.get(namespace)


def require_foreign(namespace, symbol=None):
    """Ensure the given foreign marshaling module is available and loaded.

    :param str namespace:
        Introspection namespace of the foreign module (e.g. "cairo")
    :param symbol:
        Optional symbol typename to ensure a converter exists.
    :type symbol: str or None
    :raises: ImportError

    :Example:

    .. code-block:: python

        import gi
        import cairo

        gi.require_foreign("cairo")

    """
    try:
        _gi.require_foreign(namespace, symbol)
    except Exception as e:
        raise ImportError(str(e))
    importlib.import_module("gi.repository", namespace)


def get_option(name):
    """Get option by name.

    :param str name:
        Name of the option.
    """
    return _options.get(name)


def disable_legacy_autoinit():
    """Disable the legacy initialization of Gdk and Gtk.

    These modules are automatically initialized on import from `gi.repository`.
    This is not required, and was only kept for backwards compatibility reason.
    This behavior may eventually be dropped for future Gtk major releases.

    Usually, users should not manually perform these initializations but let
    e.g. `Gtk.Application` manage it when needed.
    """
    _options["legacy_autoinit"] = False
